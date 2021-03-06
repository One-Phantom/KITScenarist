#include "verticalline.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>
#include <QPalette>


namespace {
	/**
	 * @brief Ширина линии
	 */
	const int LINE_WIDTH = 3;

	/**
	 * @brief Минимальная высота линии
	 * @brief Используется, если ещё не задана сцена
	 */
	const int LINE_MIN_HEIGHT = 10000;
}


VerticalLineShape::VerticalLineShape(QGraphicsItem* _parent) :
	Shape(_parent),
	m_inBoundingRect(false)
{
	setCursor(Qt::SizeHorCursor);
}

VerticalLineShape::VerticalLineShape(const QPointF& _pos, QGraphicsItem* _parent) :
	Shape(_parent)
{
	setPos(_pos.x(), 0);
	setCursor(Qt::SizeHorCursor);
}

QRectF VerticalLineShape::boundingRect() const
{
	QRectF result(0, 0, LINE_WIDTH, LINE_MIN_HEIGHT);
	if (!m_inBoundingRect) {
		m_inBoundingRect = true;
		if (scene() != nullptr) {
			result.setHeight(scene()->height() - LINE_WIDTH);
		}
		m_inBoundingRect = false;
	}

	return result;
}

void VerticalLineShape::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
	Q_UNUSED(_option);
	Q_UNUSED(_widget);

	//
	// Скорректируем позицию, если нужно
	//
	if (scene() != nullptr
		&& pos().y() != scene()->sceneRect().y()) {
		setPos(pos().x(), scene()->sceneRect().y());
	}

	//
	// Рисуем линию
	//
	_painter->save();
	if (isSelected()) {
		setPenAndBrushForSelection(_painter);
		_painter->setBrush(_painter->pen().color());
	} else {
        _painter->setPen(QApplication::palette().text().color());
        _painter->setBrush(QApplication::palette().text());
	}
	_painter->drawRect(boundingRect());
	_painter->restore();
}

QVariant VerticalLineShape::itemChange(QGraphicsItem::GraphicsItemChange _change, const QVariant& _value)
{
	switch (_change) {
		case ItemPositionChange: {
			QPointF newPosition = _value.toPointF();
			newPosition.setY(scene() != nullptr ? scene()->sceneRect().y() : 0);
			emit moving();
			return newPosition;
		}

		default: {
			break;
		}
	}

	return Shape::itemChange(_change, _value);
}
