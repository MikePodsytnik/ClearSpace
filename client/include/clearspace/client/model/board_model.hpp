#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include <QObject>
#include <QColor>
#include <QPointF>

#include "clearspace/client/network/client_protocol.hpp"

namespace clearspace::client {

struct RenderStroke {
  std::uint64_t id{};
  std::uint64_t author{};
  QColor color;
  std::uint64_t strokeOrder{};
  int thickness{2};
  std::vector<QPointF> points;
  bool finished{false};
  bool local{false};
};

class BoardModel : public QObject {
  Q_OBJECT

 public:
  explicit BoardModel(QObject* parent = nullptr);

  void resetFromSnapshot(const SnapshotData& snapshot);
  void clearBoard();

  void beginLocalStroke(std::uint8_t color, std::uint8_t thickness, const QPointF& start);
  void appendLocalPoint(const QPointF& point);
  void endLocalStroke();
  bool hasLocalActiveStroke() const;

  void applyRemoteStrokeBegin(const StrokeBeginData& data);
  void applyRemoteStrokePoint(const StrokePointData& data);
  void applyRemoteStrokeEnd(const StrokeEndData& data);
  void applyRemoteCursorMove(const CursorMoveData& data);
  void removeCursor(std::uint64_t clientId);

  const std::vector<RenderStroke>& strokes() const;
  const std::map<std::uint64_t, QPointF>& cursors() const;

  static QColor mapColor(std::uint8_t color);
  static int mapThickness(std::uint8_t thickness);

 signals:
  void changed();

 private:
  std::optional<std::size_t> findStrokeIndex(std::uint64_t id);
  void sortStrokes();

  std::vector<RenderStroke> strokes_;
  std::map<std::uint64_t, QPointF> cursors_;
  std::optional<std::uint64_t> localActiveStrokeId_;
  std::uint64_t nextLocalStrokeId_{(1ULL << 63)};
};

}
