#include "clearspace/client/ui/canvas_widget.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

namespace clearspace::client {

CanvasWidget::CanvasWidget(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_StaticContents);
  setMouseTracking(true);
  cursorTimer_.start();
  ensureBuffer();
}

void CanvasWidget::setBoardModel(BoardModel* boardModel) {
  if (boardModel_ == boardModel) {
    return;
  }
  if (boardModel_) {
    disconnect(boardModel_, nullptr, this, nullptr);
  }
  boardModel_ = boardModel;
  if (boardModel_) {
    connect(boardModel_, &BoardModel::changed, this, &CanvasWidget::redrawBuffer);
  }
  redrawBuffer();
}

void CanvasWidget::setRoomId(std::uint64_t roomId) {
  roomId_ = roomId;
}

void CanvasWidget::setTool(std::uint8_t color, std::uint8_t thickness) {
  currentColor_ = color;
  currentThickness_ = thickness;
}

void CanvasWidget::clearCanvasSurface() {
  if (boardModel_) {
    boardModel_->clearBoard();
  }
  redrawBuffer();
}

void CanvasWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.fillRect(rect(), Qt::white);
  painter.drawImage(QPoint(0, 0), buffer_);

  if (!boardModel_) {
    return;
  }

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(QPen(Qt::darkGray, 1));
  painter.setBrush(Qt::yellow);

  for (const auto& [clientId, point] : boardModel_->cursors()) {
    painter.drawEllipse(point, 5.0, 5.0);
    painter.drawText(point + QPointF(8.0, -8.0), QStringLiteral("#%1").arg(clientId));
  }
}

void CanvasWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  ensureBuffer();
  redrawBuffer();
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton || roomId_ == 0 || !boardModel_) {
    QWidget::mousePressEvent(event);
    return;
  }

  const QPointF point = event->position();
  drawing_ = true;
  lastPoint_ = point;
  boardModel_->beginLocalStroke(currentColor_, currentThickness_, point);
  emit localStrokeBegan(roomId_, currentColor_, currentThickness_, point);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
  const QPointF point = event->position();

  if (roomId_ != 0 && cursorTimer_.elapsed() >= 40) {
    cursorTimer_.restart();
    emit cursorMoved(roomId_, point);
  }

  if (!drawing_ || !boardModel_) {
    QWidget::mouseMoveEvent(event);
    return;
  }

  if ((point - lastPoint_).manhattanLength() < 1.0) {
    return;
  }

  lastPoint_ = point;
  boardModel_->appendLocalPoint(point);
  emit localStrokePoint(roomId_, point);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton || !drawing_ || !boardModel_) {
    QWidget::mouseReleaseEvent(event);
    return;
  }

  drawing_ = false;
  boardModel_->endLocalStroke();
  emit localStrokeEnded(roomId_);
}

void CanvasWidget::redrawBuffer() {
  ensureBuffer();
  buffer_.fill(Qt::white);

  if (!boardModel_) {
    update();
    return;
  }

  QPainter painter(&buffer_);
  painter.setRenderHint(QPainter::Antialiasing, true);

  for (const auto& stroke : boardModel_->strokes()) {
    drawStrokeToPainter(painter, stroke);
  }

  update();
}

void CanvasWidget::ensureBuffer() {
  if (size().isEmpty()) {
    return;
  }
  if (buffer_.size() == size()) {
    return;
  }
  buffer_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
  buffer_.fill(Qt::white);
}

void CanvasWidget::drawStrokeToPainter(QPainter& painter, const RenderStroke& stroke) const {
  if (stroke.points.empty()) {
    return;
  }

  QPen pen(stroke.color, stroke.thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);

  if (stroke.points.size() == 1) {
    painter.drawPoint(stroke.points.front());
    return;
  }

  for (std::size_t i = 1; i < stroke.points.size(); ++i) {
    painter.drawLine(stroke.points[i - 1], stroke.points[i]);
  }
}

}
