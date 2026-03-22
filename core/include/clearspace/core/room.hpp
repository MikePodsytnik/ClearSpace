#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "errors.hpp"
#include "events.hpp"
#include "expected.hpp"
#include "snapshot.hpp"
#include "snapshot_sink.hpp"
#include "time.hpp"
#include "types.hpp"

namespace clearspace::core {

class Room final {
 public:
  Room(RoomId id, std::shared_ptr<ITimeProvider> timeProvider);

  RoomId id() const noexcept;

  Expected<RoomSnapshot, Error> join(ClientId client);
  Expected<std::vector<OutgoingEvent>, Error> leave(ClientId client);
  Expected<std::vector<OutgoingEvent>, Error> apply(ClientId client, const EventPayload& ev);

  std::size_t memberCount() const;
  Seq lastSeq() const;
  PersistedBoardSnapshot makePersistedSnapshot() const;

 private:
  Expected<void, Error> ensureMember_(ClientId client) const;
  std::vector<OutgoingEvent> broadcast_(ClientId from, Seq seq, const ServerEventPayload& payload) const;

  Expected<std::vector<OutgoingEvent>, Error> applyStrokeBegin_(ClientId client, const StrokeBegin& e);
  Expected<std::vector<OutgoingEvent>, Error> applyStrokePoint_(ClientId client, const StrokePoint& e);
  Expected<std::vector<OutgoingEvent>, Error> applyStrokeEnd_(ClientId client, const StrokeEnd& e);
  Expected<std::vector<OutgoingEvent>, Error> applyCursorMove_(ClientId client, const CursorMove& e);
  Expected<std::vector<OutgoingEvent>, Error> applyClear_(ClientId client, const Clear& e);

  mutable std::mutex mutex_;
  RoomId id_{};

  std::shared_ptr<ITimeProvider> timeProvider_;

  std::unordered_set<ClientId> members_;
  BoardState board_;

  Seq nextSeq_{1};
  StrokeId nextStrokeId_{1};

  std::unordered_map<ClientId, TimePoint> lastCursorUpdate_;
  std::unordered_map<ClientId, StrokeId> activeStrokeByClient_;
  std::unordered_map<StrokeId, std::size_t> strokeIndexById_;
};

}
