#include "customgraphicsview.h"

#include <QApplication>
#include <QCursor>
#include <QGestureEvent>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>


CustomGraphicsView::CustomGraphicsView (QWidget* _parent) :
	QGraphicsView(_parent),
	m_useCorkboardBackground(true),
	m_rubberRect(nullptr),
	m_inRubberBanding(false),
	m_inScrolling(false),
	m_gestureZoomInertionBreak(0)
{
	//
	// Отслеживаем жесты
	//
	grabGesture(Qt::PinchGesture);
	grabGesture(Qt::SwipeGesture);
}

void CustomGraphicsView::setUseCorkboardBackground(bool _use)
{
	if (m_useCorkboardBackground != _use) {
		m_useCorkboardBackground = _use;
		updateBackgroundBrush();
	}
}

void CustomGraphicsView::setBackgroundColor(const QColor& _color)
{
	if (m_backgroundColor != _color) {
		m_backgroundColor = _color;
		updateBackgroundBrush();
	}
}

void CustomGraphicsView::zoomIn()
{
	scaleView(qreal(0.1));
}

void CustomGraphicsView::zoomOut()
{
	scaleView(qreal(-0.1));
}

bool CustomGraphicsView::event(QEvent* _event)
{
	//
	// Определяем особый обработчик для жестов
	//
	if (_event->type() == QEvent::Gesture) {
		gestureEvent(static_cast<QGestureEvent*>(_event));
		return true;
	}

	//
	// Прочие стандартные обработчики событий
	//
	return QGraphicsView::event(_event);
}

void CustomGraphicsView::gestureEvent(QGestureEvent* _event)
{
	//
	// Жест масштабирования
	//
	if (QGesture* gesture = _event->gesture(Qt::PinchGesture)) {
		if (QPinchGesture* pinch = qobject_cast<QPinchGesture *>(gesture)) {
			//
			// При масштабировании за счёт жестов приходится немного притормаживать
			// т.к. события приходят слишком часто и при обработке каждого события
			// пользователю просто невозможно корректно настроить масштаб
			//

			const int INERTION_BREAK_STOP = 4;
			qreal zoomDelta = 0;
			if (pinch->scaleFactor() > 1) {
				if (m_gestureZoomInertionBreak < 0) {
					m_gestureZoomInertionBreak = 0;
				} else if (m_gestureZoomInertionBreak >= INERTION_BREAK_STOP) {
					m_gestureZoomInertionBreak = 0;
					zoomDelta = 0.1;
				} else {
					++m_gestureZoomInertionBreak;
				}
			} else if (pinch->scaleFactor() < 1) {
				if (m_gestureZoomInertionBreak > 0) {
					m_gestureZoomInertionBreak = 0;
				} else if (m_gestureZoomInertionBreak <= -INERTION_BREAK_STOP) {
					m_gestureZoomInertionBreak = 0;
					zoomDelta = -0.1;
				} else {
					--m_gestureZoomInertionBreak;
				}
			} else {
				//
				// При обычной прокрутке часто приходит событие с scaledFactor == 1,
				// так что просто игнорируем их
				//
			}

			//
			// Если необходимо масштабируем и перерисовываем представление
			//
			const bool needZoomIn = pinch->scaleFactor() > 1;
			const bool needZoomOut = pinch->scaleFactor() < 1;
			if (zoomDelta != 0 && needZoomIn) {
				zoomIn();
			} else if (zoomDelta != 0 && needZoomOut) {
				zoomOut();
			}

			_event->accept();
		}
	}
}

void CustomGraphicsView::wheelEvent(QWheelEvent* _event)
{
#ifdef Q_OS_MAC
	const qreal ANGLE_DIVIDER = 2.;
#else
	const qreal ANGLE_DIVIDER = 120.;
#endif
	//
	// Собственно масштабирование
	//
	if (_event->modifiers() & Qt::ControlModifier) {
		if (_event->orientation() == Qt::Vertical) {
			//
			// zoomRange > 0 - масштаб увеличивается
			// zoomRange < 0 - масштаб уменьшается
			//
			const qreal zoom = _event->angleDelta().y() / ANGLE_DIVIDER;
			if (zoom > 0) {
				zoomIn();
			} else if (zoom < 0) {
				zoomOut();
			}

			_event->accept();
		}
	}
	//
	// В противном случае прокручиваем редактор
	//
	else {
		QGraphicsView::wheelEvent(_event);
	}
}

void CustomGraphicsView::keyPressEvent(QKeyEvent* _event)
{
	if ((_event->key() == Qt::Key_Plus || _event->key() == Qt::Key_Equal)
		&& _event->modifiers() & Qt::ControlModifier)
	{
		zoomIn();
		return;
	}

	if (_event->key() == Qt::Key_Minus
		&& _event->modifiers() & Qt::ControlModifier)
	{
		zoomOut();
		return;
	}

	if (_event->key() == Qt::Key_Delete
		|| _event->key() == Qt::Key_Backspace) {
		emit deletePressed();
		return;
	}

	if (_event->key() == Qt::Key_Space) {
		m_inScrolling = true;
		return;
	}

	QGraphicsView::keyPressEvent(_event);
}

void CustomGraphicsView::keyReleaseEvent(QKeyEvent* _event)
{
	if (_event->key() == Qt::Key_Space) {
		m_inScrolling = false;
	}

	QGraphicsView::keyReleaseEvent(_event);
}

void CustomGraphicsView::mousePressEvent(QMouseEvent* _event)
{
	if (m_inScrolling
		&& _event->buttons() & Qt::LeftButton) {
		m_scrollingLastPos = _event->globalPos();
		QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
		return;
	}

	QList<QGraphicsItem*> items = scene()->items();
	prevSelState.clear();
	for (int i = 0; i < items.count(); ++i) {
		if (items[i]->flags() & QGraphicsItem::ItemIsSelectable) {
			prevSelState[items[i]] = items[i]->isSelected();
		}
	}

	QGraphicsView::mousePressEvent(_event);

	//
	// Если в точке нет элемента, начинаем выделение
	//
	if (scene()->itemAt(mapToScene(_event->pos()), QTransform()) == nullptr) {
		m_startPos = _event->pos();
		m_inRubberBanding = true;

		m_rubberRect = new QGraphicsRectItem;
		m_rubberRect->setPen(QPen(Qt::gray));
		m_rubberRect->setBrush(QColor(255, 255, 255, 120));
		m_rubberRect->setRect(QRectF());
		if (_event->modifiers() & Qt::ControlModifier) {
			//
			// Выделение было снято методом родителя,
			// теперь его нужно выставить обратно
			//
			for (int i=0; i<items.count(); ++i) {
				if (prevSelState.contains(items[i])) {
					items[i]->setSelected(prevSelState[items[i]]);
				}
			}
		}
		scene()->addItem(m_rubberRect);
	}
	//
	// В противном случае, если это клик правой кнопкой мышки, выделим элемент под ней
	//
	else if (_event->button() == Qt::RightButton) {
		scene()->clearSelection();
		scene()->itemAt(mapToScene(_event->pos()), QTransform())->setSelected(true);
	}
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent* _event)
{
	//
	// Если в данный момент происходит прокрутка полотна
	//
	if (m_inScrolling) {
		if (_event->buttons() & Qt::LeftButton) {
			const QPoint prevPos = m_scrollingLastPos;
			m_scrollingLastPos = _event->globalPos();
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (prevPos.x() - m_scrollingLastPos.x()));
			verticalScrollBar()->setValue(verticalScrollBar()->value() + (prevPos.y() - m_scrollingLastPos.y()));
		}
		return;
	}

	//
	// Если в данным момент происходит не выделение области, то убираем элемент области со сцены
	//
	if (m_inRubberBanding
		&& (!_event->buttons()
			|| !m_rubberRect)) {
		m_inRubberBanding = false;
		delete m_rubberRect;
		m_rubberRect = nullptr;
		return;
	}

	//
	// Если происходит изменение размера области выделения, то корректируем её
	//
	if (m_inRubberBanding) {
		if ((m_startPos-_event->pos()).manhattanLength() < QApplication::startDragDistance()) {
			return;
		}

		QPointF mp = mapToScene(m_startPos);
		QPointF ep = mapToScene(_event->pos());
		m_rubberRect->setRect(
			qMin(mp.x(), ep.x()),
			qMin(mp.y(), ep.y()),
			qAbs(mp.x()-ep.x()) + 1,
			qAbs(mp.y()-ep.y())+1);
		m_rubberRect->setVisible(true);

		//
		// Выделяем элементы входящие в выделенную область
		//
		QPainterPath selectionArea;
		selectionArea.addPolygon(m_rubberRect->rect());
		selectionArea.closeSubpath();
		if (_event->modifiers() & Qt::ControlModifier) {
			selectItemsWithCtrl(selectionArea);
		} else if (scene() != nullptr) {
			scene()->setSelectionArea(selectionArea, rubberBandSelectionMode(), viewportTransform());
		}
	}
	//
	// В противном случае используем стандартный обработчик события
	//
	else {
		QGraphicsView::mouseMoveEvent(_event);
	}
}


void CustomGraphicsView::mouseReleaseEvent(QMouseEvent* _event)
{
	if (m_inScrolling) {
		QApplication::restoreOverrideCursor();
	}
	if (m_rubberRect && m_inRubberBanding) {
		scene()->removeItem(m_rubberRect);
		delete m_rubberRect;
		m_rubberRect = nullptr;
		m_inRubberBanding = false;
	} else if (_event->button() == Qt::RightButton) {
		emit contextMenuRequest(_event->pos());
	}

	QGraphicsView::mouseReleaseEvent(_event);
}

void CustomGraphicsView::updateBackgroundBrush()
{
	if (m_useCorkboardBackground) {
		QBrush brush(QImage(":/Graphics/Images/corkboard.jpg"));
		brush.setTransform(QTransform().scale(0.4, 0.4));
		setBackgroundBrush(brush);
		setCacheMode(QGraphicsView::CacheBackground);
	} else {
		setBackgroundBrush(m_backgroundColor);
	}
}

void CustomGraphicsView::scaleView(qreal _factor)
{
	scale(1 + _factor, 1 + _factor);
}

void CustomGraphicsView::selectItemsWithCtrl(QPainterPath& _area)
{
	QList<QGraphicsItem*> items = scene()->items(_area, rubberBandSelectionMode(), Qt::AscendingOrder, viewportTransform());
	for (int i = 0; i < items.count(); ++i) {
		items[i]->setSelected(true);
	}
}
