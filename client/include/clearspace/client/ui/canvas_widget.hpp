#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QWidget>

#include "clearspace/client/model/board_model.hpp"

namespace clearspace::client {

class CanvasWidget : public QWidget {
  Q_OBJECT

 public:
  explicit CanvasWidget(QWidget* parent = nullptr);

  void setBoardModel(BoardModel* boardModel);
  void setRoomId(std::uint64_t roomId);
  void setTool(std::uint8_t color, std::uint8_t thickness);
  void clearCanvasSurface();

 signals:
  void localStrokeBegan(std::uint64_t roomId, std::uint8_t color, std::uint8_t thickness, const QPointF& point);
  void localStrokePoint(std::uint64_t roomId, const QPointF& point);
  void localStrokeEnded(std::uint64_t roomId);
  void cursorMoved(std::uint64_t roomId, const QPointF& point);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 private slots:
  void redrawBuffer();

 private:
  void ensureBuffer();
  void drawStrokeToPainter(QPainter& painter, const RenderStroke& stroke) const;

  BoardModel* boardModel_{nullptr};
  QImage buffer_;
  std::uint64_t roomId_{0};
  std::uint8_t currentColor_{0};
  std::uint8_t currentThickness_{0};
  bool drawing_{false};
  QPointF lastPoint_;
  QElapsedTimer cursorTimer_;
};

}
