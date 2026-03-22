#include "clearspace/core/room.hpp"

namespace clearspace::core {

Room::Room(RoomId id, std::shared_ptr<ITimeProvider> timeProvider)
    : id_(id), timeProvider_(std::move(timeProvider)) {}

RoomId Room::id() const noexcept {
  return id_;
}

std::size_t Room::memberCount() const {
  std::scoped_lock lock(mutex_);
  return members_.size();
}

Seq Room::lastSeq() const {
  std::scoped_lock lock(mutex_);
  return nextSeq_ == 0 ? 0 : (nextSeq_ - 1);
}

PersistedBoardSnapshot Room::makePersistedSnapshot() const {
  std::scoped_lock lock(mutex_);
  PersistedBoardSnapshot s;
  s.room = id_;
  s.lastSeq = nextSeq_ == 0 ? 0 : (nextSeq_ - 1);
  s.strokes = board_.strokes;  // no cursors
  return s;
}

Expected<void, Error> Room::ensureMember_(ClientId client) const {
  if (members_.find(client) == members_.end()) {
    return Error::notInRoom();
  }
  return Expected<void, Error>();
}

std::vector<OutgoingEvent> Room::broadcast_(ClientId from, Seq seq, const ServerEventPayload& payload) const {
  std::vector<OutgoingEvent> out;
  out.reserve(members_.size() > 0 ? members_.size() - 1 : 0);
  for (ClientId m : members_) {
    if (m == from) {
      continue;
    }
    out.push_back(OutgoingEvent{.room = id_, .to = m, .seq = seq, .payload = payload});
  }
  return out;
}

Expected<RoomSnapshot, Error> Room::join(ClientId client) {
  std::scoped_lock lock(mutex_);
  members_.insert(client);

  std::vector<ClientId> members;
  members.reserve(members_.size());
  for (ClientId m : members_) {
    members.push_back(m);
  }

  RoomSnapshot snap;
  snap.room = id_;
  snap.lastSeq = nextSeq_ == 0 ? 0 : (nextSeq_ - 1);
  snap.state = board_;
  snap.members = std::move(members);
  return snap;
}

Expected<std::vector<OutgoingEvent>, Error> Room::leave(ClientId client) {
  std::scoped_lock lock(mutex_);
  if (members_.find(client) == members_.end()) {
    return Error::notInRoom();
  }

  std::vector<OutgoingEvent> out;

  auto itActive = activeStrokeByClient_.find(client);
  if (itActive != activeStrokeByClient_.end()) {
    StrokeId sid = itActive->second;
    auto itIdx = strokeIndexById_.find(sid);
    if (itIdx != strokeIndexById_.end()) {
      board_.strokes[itIdx->second].finished = true;
    }
    activeStrokeByClient_.erase(itActive);

    Seq seqEnd = nextSeq_++;
    auto extra = broadcast_(client, seqEnd, StrokeEndOut{.author = client, .id = sid});
    out.insert(out.end(), extra.begin(), extra.end());
  }

  board_.cursors.erase(client);
  lastCursorUpdate_.erase(client);

  members_.erase(client);

  Seq seqLeft = nextSeq_++;
  auto left = broadcast_(client, seqLeft, MemberLeftOut{.client = client});
  out.insert(out.end(), left.begin(), left.end());
  return out;
}

Expected<std::vector<OutgoingEvent>, Error> Room::apply(ClientId client, const EventPayload& ev) {
  std::scoped_lock lock(mutex_);
  auto ok = ensureMember_(client);
  if (!ok) {
    return ok.error();
  }

  if (std::holds_alternative<StrokeBegin>(ev)) {
    return applyStrokeBegin_(client, std::get<StrokeBegin>(ev));
  }
  if (std::holds_alternative<StrokePoint>(ev)) {
    return applyStrokePoint_(client, std::get<StrokePoint>(ev));
  }
  if (std::holds_alternative<StrokeEnd>(ev)) {
    return applyStrokeEnd_(client, std::get<StrokeEnd>(ev));
  }
  if (std::holds_alternative<CursorMove>(ev)) {
    return applyCursorMove_(client, std::get<CursorMove>(ev));
  }
  return applyClear_(client, std::get<Clear>(ev));
}

Expected<std::vector<OutgoingEvent>, Error> Room::applyStrokeBegin_(ClientId client, const StrokeBegin& e) {
  if (activeStrokeByClient_.find(client) != activeStrokeByClient_.end()) {
    return Error::protocolViolation("stroke already active");
  }

  Seq seq = nextSeq_++;
  StrokeId sid = nextStrokeId_++;

  Stroke s;
  s.id = sid;
  s.author = client;
  s.color = e.color;
  s.thickness = e.thickness;
  s.strokeOrder = seq;
  s.points.push_back(e.start);
  s.finished = false;

  std::size_t idx = board_.strokes.size();
  board_.strokes.push_back(std::move(s));
  strokeIndexById_[sid] = idx;
  activeStrokeByClient_[client] = sid;

  return broadcast_(client, seq, StrokeBeginOut{.author = client, .id = sid, .strokeOrder = seq, .color = e.color, .thickness = e.thickness, .start = e.start});
}

Expected<std::vector<OutgoingEvent>, Error> Room::applyStrokePoint_(ClientId client, const StrokePoint& e) {
  auto itActive = activeStrokeByClient_.find(client);
  if (itActive == activeStrokeByClient_.end()) {
    return Error::protocolViolation("stroke point without begin");
  }

  StrokeId sid = itActive->second;
  auto itIdx = strokeIndexById_.find(sid);
  if (itIdx == strokeIndexById_.end()) {
    return Error::protocolViolation("unknown stroke id");
  }

  board_.strokes[itIdx->second].points.push_back(e.p);

  Seq seq = nextSeq_++;
  return broadcast_(client, seq, StrokePointOut{.author = client, .id = sid, .p = e.p});
}

Expected<std::vector<OutgoingEvent>, Error> Room::applyStrokeEnd_(ClientId client, const StrokeEnd&) {
  auto itActive = activeStrokeByClient_.find(client);
  if (itActive == activeStrokeByClient_.end()) {
    return Error::protocolViolation("stroke end without begin");
  }

  StrokeId sid = itActive->second;
  auto itIdx = strokeIndexById_.find(sid);
  if (itIdx == strokeIndexById_.end()) {
    return Error::protocolViolation("unknown stroke id");
  }

  board_.strokes[itIdx->second].finished = true;
  activeStrokeByClient_.erase(itActive);

  Seq seq = nextSeq_++;
  return broadcast_(client, seq, StrokeEndOut{.author = client, .id = sid});
}

Expected<std::vector<OutgoingEvent>, Error> Room::applyCursorMove_(ClientId client, const CursorMove& e) {
  TimePoint now = timeProvider_->now();
  auto itLast = lastCursorUpdate_.find(client);
  if (itLast != lastCursorUpdate_.end()) {
    if (now - itLast->second < config::kCursorMinInterval) {
      return std::vector<OutgoingEvent>{};
    }
  }

  lastCursorUpdate_[client] = now;
  board_.cursors[client] = e.p;

  Seq seq = nextSeq_++;
  return broadcast_(client, seq, CursorMoveOut{.author = client, .p = e.p});
}

Expected<std::vector<OutgoingEvent>, Error> Room::applyClear_(ClientId client, const Clear&) {
  board_.strokes.clear();
  activeStrokeByClient_.clear();
  strokeIndexById_.clear();

  Seq seq = nextSeq_++;
  return broadcast_(client, seq, ClearOut{.author = client});
}

}
