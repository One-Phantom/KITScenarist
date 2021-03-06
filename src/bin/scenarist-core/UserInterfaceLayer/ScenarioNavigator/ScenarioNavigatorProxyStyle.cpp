#include "ScenarioNavigatorProxyStyle.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>

using UserInterface::ScenarioNavigatorProxyStyle;


ScenarioNavigatorProxyStyle::ScenarioNavigatorProxyStyle(QStyle* _style)
	: QProxyStyle(_style)
{
}

void ScenarioNavigatorProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	if(element == QStyle::PE_IndicatorItemViewItemDrop)
	{
		painter->setRenderHint(QPainter::Antialiasing, true);

		QPalette palette(QApplication::palette());
		QColor indicatorColor(palette.text().color());

		//
		// Кисть для рисования линий
		//
		QPen pen(indicatorColor);
		pen.setWidth(2);
		QBrush brush(indicatorColor);

		//
		// Настраиваем рисовальщика
		//
		painter->setPen(pen);
		painter->setBrush(brush);

		//
		// Элемент вставляется в конец списка
		//
		if (option->rect.topLeft().isNull() && option->rect.size().isEmpty()) {
			//
			// Рисуем вспомогательный треугольник внизу виджета
			//
			int x = widget->width() / 2;
			int y = widget->height() - 16;
			QPolygonF treangle;
			treangle <<	QPointF(x, y)
					 << QPointF(x + 7,  y + 10)
					 << QPointF(x - 7,  y + 10);
			painter->drawPolygon(treangle);
		}
		//
		// Элемент вставляется между двух соседних
		//
		else if (!option->rect.topLeft().isNull() && option->rect.size().isEmpty()) {
			//
			// Рисуем линию между элементов
			//
			painter->drawLine(QPoint(option->rect.topLeft().x() - 10,
									 option->rect.topLeft().y()),
							  option->rect.topRight());
			//
			// Вспомогательный треугольник
			//
			QPolygonF treangle;
			treangle <<	QPointF(option->rect.topLeft().x() - 10, option->rect.topLeft().y() - 4)
					 << QPointF(option->rect.topLeft().x() - 5,  option->rect.topLeft().y())
					 << QPointF(option->rect.topLeft().x() - 10, option->rect.topLeft().y() + 4);
			painter->drawPolygon(treangle);
		}
		//
		// Элемент вставляется в группирующий
		//
		else {
			//
			// Заливку делаем полупрозрачной
			//
			indicatorColor.setAlpha(50);
			QBrush brush(indicatorColor);
			painter->setBrush(brush);

			//
			// Расширим немного область, чтобы не перекрывать иконку
			//
			QRect rect = option->rect;
			rect.setX(rect.topLeft().x() - 4);
			painter->drawRoundedRect(rect, 2, 2);
		}
	} else {
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
}
