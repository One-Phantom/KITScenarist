#ifndef SCENARIOCARDSVIEW_H
#define SCENARIOCARDSVIEW_H

#include <QWidget>

class ActivityEdit;
class FlatButton;
class QLabel;

namespace UserInterface {
	class CardsResizer;


	/**
	 * @brief Представление редактора карт
	 */
	class ScenarioCardsView : public QWidget
	{
		Q_OBJECT

	public:
		explicit ScenarioCardsView(QWidget* _parent = 0);

		/**
		 * @brief Очистить схему
		 */
		void clear();

		/**
		 * @brief Отменить последнее действие
		 */
		void undo();

		/**
		 * @brief Повторить последнее действие
		 */
		void redo();

		/**
		 * @brief Необходимо ли использовать в качестве фона пробковую доску
		 */
		void setUseCorkboardBackground(bool _use);

		/**
		 * @brief Установить цвет фона
		 */
		void setBackgroundColor(const QColor& _color);

		/**
		 * @brief Загрузить схему из xml-строки
		 */
		void load(const QString& _xml);

		/**
		 * @brief Получить xml-строку текущей схемы
		 */
		QString save() const;

		/**
		 * @brief Сохранить изменения схемы
		 */
		void saveChanges(bool _hasChangesInText);

		/**
		 * @brief Добавить карточку
		 */
		void addCard(const QString& _uuid, int _cardType, const QString& _title,
			const QString& _description, const QString& _colors, bool _isCardFirstInParent);

		/**
		 * @brief Обновить карточку с заданным uuid
		 */
		void updateCard(const QString& _uuid, int _type, const QString& _title,
			const QString& _description, const QString& _colors);

		/**
		 * @brief Удалить карточку с заданным uuid
		 */
		void removeCard(const QString& _uuid);

		/**
		 * @brief Сделать активной карточку с заданным номером
		 */
		void selectCard(const QString& _uuid);

		/**
		 * @brief Получить номер выделенной карточки, если нет выделенных, или выделено больше одной, возвращается -1
		 */
		QString selectedCardUuid() const;

		/**
		 * @brief Добавить заметку
		 */
		void addNote(const QString& _text);

		/**
		 * @brief Изменить заметку
		 */
		void editNote(const QString& _text);

		/**
		 * @brief Добавить текст на связь
		 */
		void addFlowText(const QString& _text);

		/**
		 * @brief Изменить текст на связи
		 */
		void editFlowText(const QString& _text);

		/**
		 * @brief Установить режим работы со сценарием
		 */
		void setCommentOnly(bool _isCommentOnly);

	signals:
		/**
		 * @brief Не удалось загрузить схему
		 */
		void schemeNotLoaded();

		/**
		 * @brief Запрос на отмену последнего действия
		 */
		void undoRequest();

		/**
		 * @brief Запрос на повтор последнего действия
		 */
		void redoRequest();

		/**
		 * @brief Нажата кнопка добавления карточки
		 */
		void addCardClicked();

		/**
		 * @brief Запросы на изменение выделенной фигуры
		 */
		/** @{ */
		void editCardRequest(const QString& _uuid, int _cardType, const QString& _title, const QString& _color, const QString& _description);
		void editNoteRequest(const QString& _text);
		void editFlowTextRequest(const QString& _text);
		/** @} */

		/**
		 * @brief Нажата кнопка удаления карточки
		 */
		void removeCardRequest(const QString& _uuid);

		/**
		 * @brief Карточка была перемещена
		 */
		void cardMoved(const QString& _parentUuid, const QString& _previousUuid, const QString& _movedUuid);

		/**
		 * @brief Изменились цвета карточки
		 */
		void cardColorsChanged(const QString& _uuid, const QString& _colors);

		/**
		 * @brief Запрос на изменение типа карточки
		 */
		void itemTypeChanged(const QString& _uuid, int _cardType);

		/**
		 * @brief Нажата кнопка добавления карточки
		 */
		void addNoteClicked();

		/**
		 * @brief Запрос на добавление текста на связь
		 */
		void addFlowTextRequest();

		/**
		 * @brief Запрос на переход в полноэкранный режим, или выход из него
		 */
		void fullscreenRequest();

		/**
		 * @brief Схема карточек изменена
		 */
		void schemeChanged();

	private:
		/**
		 * @brief Упорядочить карточки по сетке
		 */
		void resortCards();

	private:
		/**
		 * @brief Настроить представление
		 */
		void initView();

		/**
		 * @brief Настроить соединения
		 */
		void initConnections();

		/**
		 * @brief Настроить горячие клавиши
		 */
		void initShortcuts();

		/**
		 * @brief Настроить внешний вид
		 */
		void initStyleSheet();

	private:
		/**
		 * @brief Редактор карточек
		 */
		ActivityEdit* m_cardsEdit;

		/**
		 * @brief Кнопка добавления карточки
		 */
		FlatButton* m_addCard;

		/**
		 * @brief Кнопка добавления заметки
		 */
		FlatButton* m_addNote;

		/**
		 * @brief Кнопка добавления горизонтальной линии
		 */
		FlatButton* m_addHLine;

		/**
		 * @brief Кнопка добавления вертикальной линии
		 */
		FlatButton* m_addVLine;

		/**
		 * @brief Кнопка упорядочивания по таблице
		 */
		FlatButton* m_sort;

		/**
		 * @brief Виджет настройки размера и упорядочивания карточек
		 */
		CardsResizer* m_resizer;

		/**
		 * @brief Перейти в полноэкранный режим
		 */
		FlatButton* m_fullscreen;

		/**
		 * @brief Метка, для закрашивания пространства в панели инструментов
		 */
		QLabel* m_toolbarSpacer;
	};
}

#endif // SCENARIOCARDSVIEW_H
