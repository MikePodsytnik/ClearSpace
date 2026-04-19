#include "clearspace/client/model/board_model.hpp"

#include <algorithm>

#include <Qt>

namespace clearspace::client {

BoardModel::BoardModel(QObject* parent) : QObject(parent) {}

void BoardModel::resetFromSnapshot(const SnapshotData& snapshot) {
  strokes_.clear();
  cursors_.clear();
  localActiveStrokeId_.reset();
  nextLocalStrokeId_ = (1ULL << 63);

  for (const auto& stroke : snapshot.strokes) {
    RenderStroke renderStroke;
    renderStroke.id = stroke.id;
    renderStroke.author = stroke.author;
    renderStroke.color = mapColor(stroke.color);
    renderStroke.strokeOrder = stroke.strokeOrder;
    renderStroke.thickness = mapThickness(stroke.thickness);
    renderStroke.points = stroke.points;
    renderStroke.finished = stroke.finished;
    renderStroke.local = false;
    strokes_.push_back(std::move(renderStroke));
    nextLocalStrokeId_ = std::max(nextLocalStrokeId_, stroke.id + 1);
  }

  for (const auto& cursor : snapshot.cursors) {
    cursors_[cursor.client] = cursor.point;
  }

  sortStrokes();
  emit changed();
}

void BoardModel::clearBoard() {
  strokes_.clear();
  cursors_.clear();
  localActiveStrokeId_.reset();
  emit changed();
}

void BoardModel::beginLocalStroke(std::uint8_t color, std::uint8_t thickness, const QPointF& start) {
  if (localActiveStrokeId_.has_value()) {
    return;
  }

  RenderStroke stroke;
  stroke.id = nextLocalStrokeId_++;
  stroke.color = mapColor(color);
  stroke.thickness = mapThickness(thickness);
  stroke.strokeOrder = stroke.id;
  stroke.points.push_back(start);
  stroke.finished = false;
  stroke.local = true;
  strokes_.push_back(std::move(stroke));
  localActiveStrokeId_ = strokes_.back().id;
  emit changed();
}

void BoardModel::appendLocalPoint(const QPointF& point) {
  if (!localActiveStrokeId_) {
    return;
  }
  auto index = findStrokeIndex(*localActiveStrokeId_);
  if (!index) {
    return;
  }
  strokes_[*index].points.push_back(point);
  emit changed();
}

void BoardModel::endLocalStroke() {
  if (!localActiveStrokeId_) {
    return;
  }
  auto index = findStrokeIndex(*localActiveStrokeId_);
  if (index) {
    strokes_[*index].finished = true;
  }
  localActiveStrokeId_.reset();
  emit changed();
}

bool BoardModel::hasLocalActiveStroke() const {
  return localActiveStrokeId_.has_value();
}

void BoardModel::applyRemoteStrokeBegin(const StrokeBeginData& data) {
  RenderStroke stroke;
  stroke.id = data.id;
  stroke.author = data.author;
  stroke.strokeOrder = data.strokeOrder;
  stroke.color = mapColor(data.color);
  stroke.thickness = mapThickness(data.thickness);
  stroke.points.push_back(data.start);
  stroke.finished = false;
  stroke.local = false;
  strokes_.push_back(std::move(stroke));
  sortStrokes();
  emit changed();
}

void BoardModel::applyRemoteStrokePoint(const StrokePointData& data) {
  auto index = findStrokeIndex(data.id);
  if (!index) {
    return;
  }
  strokes_[*index].points.push_back(data.point);
  emit changed();
}

void BoardModel::applyRemoteStrokeEnd(const StrokeEndData& data) {
  auto index = findStrokeIndex(data.id);
  if (!index) {
    return;
  }
  strokes_[*index].finished = true;
  emit changed();
}

void BoardModel::applyRemoteCursorMove(const CursorMoveData& data) {
  cursors_[data.author] = data.point;
  emit changed();
}

void BoardModel::removeCursor(std::uint64_t clientId) {
  cursors_.erase(clientId);
  emit changed();
}

const std::vector<RenderStroke>& BoardModel::strokes() const {
  return strokes_;
}

const std::map<std::uint64_t, QPointF>& BoardModel::cursors() const {
  return cursors_;
}

QColor BoardModel::mapColor(std::uint8_t color) {
  switch (color) {
    case 1:
      return Qt::red;
    case 2:
      return QColor(0, 150, 0);
    case 3:
      return Qt::blue;
    default:
      return Qt::black;
  }
}

int BoardModel::mapThickness(std::uint8_t thickness) {
  switch (thickness) {
    case 1:
      return 4;
    case 2:
      return 7;
    default:
      return 2;
  }
}

std::optional<std::size_t> BoardModel::findStrokeIndex(std::uint64_t id) {
  for (std::size_t i = 0; i < strokes_.size(); ++i) {
    if (strokes_[i].id == id) {
      return i;
    }
  }
  return std::nullopt;
}

void BoardModel::sortStrokes() {
  std::stable_sort(strokes_.begin(), strokes_.end(), [](const RenderStroke& lhs, const RenderStroke& rhs) {
    if (lhs.strokeOrder != rhs.strokeOrder) {
      return lhs.strokeOrder < rhs.strokeOrder;
    }
    return lhs.id < rhs.id;
  });
}

}
