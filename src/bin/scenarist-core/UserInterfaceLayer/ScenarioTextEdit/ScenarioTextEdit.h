#ifndef SCENARIOTEXTEDIT_H
#define SCENARIOTEXTEDIT_H

#include <3rd_party/Widgets/CompletableTextEdit/CompletableTextEdit.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

namespace BusinessLogic {
	class ScenarioTextDocument;
}

class QCompleter;

namespace UserInterface
{
	class ShortcutsManager;


	/**
	 * @brief Текстовый редактор сценария
	 */
	class ScenarioTextEdit : public CompletableTextEdit
	{
		Q_OBJECT

	public:
		explicit ScenarioTextEdit(QWidget* _parent);

		/**
		 * @brief Установить документ для редактирования
		 */
		void setScenarioDocument(BusinessLogic::ScenarioTextDocument* _document);

		/**
		 * @brief Вставить новый блок
		 * @param Тип блока
		 */
		void addScenarioBlock(BusinessLogic::ScenarioBlockStyle::Type _blockType);

		/**
		 * @brief Установить вид текущего блока
		 * @param Тип блока
		 */
		void changeScenarioBlockType(BusinessLogic::ScenarioBlockStyle::Type _blockType, bool _forced = false);

		/**
		 * @brief Применить тип блока ко всему тексту в блоке
		 * @param Тип для применения
		 */
		void applyScenarioTypeToBlockText(BusinessLogic::ScenarioBlockStyle::Type _blockType);

		/**
		 * @brief Получить вид блока в котором находится курсор
		 */
		BusinessLogic::ScenarioBlockStyle::Type scenarioBlockType() const;

		/**
		 * @brief Своя реализация установки курсора
		 */
		void setTextCursorReimpl(const QTextCursor& _cursor);

		/**
		 * @brief Получить значение флага сигнализирующего сохранять ли данные во время редактирования
		 */
		bool storeDataWhenEditing() const;

		/**
		 * @brief Установить значение флага сигнализирующего сохранять ли данные во время редактирования
		 */
		void setStoreDataWhenEditing(bool _store);

		/**
		 * @brief Показываются ли в редакторе номера сцен
		 */
		bool showSceneNumbers() const;

		/**
		 * @brief Установить значение необходимости отображать номера сцен
		 */
		void setShowSceneNumbers(bool _show);

		/**
		 * @brief Подсвечивается ли текущая строка в редакторе
		 */
		bool highlightCurrentLine() const;

		/**
		 * @brief Установить значение необходимости подсвечивать текущую строку
		 */
		void setHighlightCurrentLine(bool _highlight);

		/**
		 * @brief Установить необходимость автоматических замен
		 */
		void setAutoReplacing(bool _capitalizeFirstWord, bool _correctDoubleCapitals,
			bool _replaceThreeDots, bool _smartQuotes);

		/**
		 * @brief Показывать ли автодополнения в пустых блоках
		 */
		void setShowSuggestionsInEmptyBlocks(bool _show);

		/**
		 * @brief Установить доступность выделения текста мышью
		 */
		void setTextSelectionEnable(bool _enable);

		/**
		 * @brief Редактор в режиме отображения поэпизодника или сценария
		 */
		bool outlineMode() const;

		/**
		 * @brief Установить режим отображения поэпизодника или сценария
		 */
		void setOutlineMode(bool _outlineMode);

		/**
		 * @brief Получить список видимых блоков в зависимости от режима отображения поэпизодника или сценария
		 */
		QList<BusinessLogic::ScenarioBlockStyle::Type> visibleBlocksTypes() const;

		/**
		 * @brief Обновить сочетания клавиш для переходов между блоками
		 */
		void updateShortcuts();

		/**
		 * @brief Получить сочетание смены для блока
		 */
		QString shortcut(BusinessLogic::ScenarioBlockStyle::Type _forType) const;

		/**
		 * @brief Переопределяем для корректировки вызова отмены/повтора последнего действия
		 */
		QMenu* createContextMenu(const QPoint &_pos, QWidget* _parent = 0);

		/**
		 * @brief Установить список дополнительных курсоров для отрисовки
		 */
		void setAdditionalCursors(const QMap<QString, int>& _cursors);

	public slots:
		/**
		 * @brief Собственные реализации отмены/повтора последнего действия
		 */
		/** @{ */
		void undoReimpl();
		void redoReimpl();
		/** @} */

	signals:
		/**
		 * @brief Сменился стиль под курсором
		 */
		void currentStyleChanged();

		/**
		 * @brief Изменён стиль блока
		 */
		void styleChanged();

		/**
		 * @brief В документ были внесены редакторские примечания
		 */
		void reviewChanged();

	protected:
		/**
		 * @brief Нажатия многих клавиш обрабатываются вручную
		 */
		void keyPressEvent(QKeyEvent* _event);

		/**
		 * @brief Переопределяем, чтобы самостоятельно обрабатывать вводимый пользователем текст
		 */
		void inputMethodEvent(QInputMethodEvent* _event);

		/**
		 * @brief Дополнительная функция для обработки нажатий самим редактором
		 * @return Обработано ли событие
		 */
		bool keyPressEventReimpl(QKeyEvent* _event);

		/**
		 * @brief Переопределяется для корректной загрузки больших документов
		 */
		void paintEvent(QPaintEvent* _event);

		/**
		 * @brief Переопределяется для обработки тройного клика
		 */
		void mousePressEvent(QMouseEvent* _event);

		/**
		 * @brief Переопределяется, для того, чтобы блокировать выделение текста мышкой
		 */
		void mouseMoveEvent(QMouseEvent* _event);

		/**
		 * @brief Переопределяется для того, чтобы корректировать позицию курсора, если пользователь
		 *		  установил курсор в блок с декорацией
		 */
		void mouseReleaseEvent(QMouseEvent* _event);

		/**
		 * @brief Переопределяем работу с буфером обмена для использования собственного майм типа данных
		 */
		/** @{ */
		bool canInsertFromMimeData(const QMimeData* _source) const;
		QMimeData* createMimeDataFromSelection() const;
		void insertFromMimeData(const QMimeData* _source);
		/** @} */

		/**
		 * @brief Переопределяем, чтобы запрещать дополнять в пустых блоках при установке
		 *		  соответствующего флага
		 */
		bool canComplete() const;

	private slots:
		/**
		 * @brief Вспомогательный слот для корректировки отображения больших документов
		 */
		void aboutCorrectRepaint();

		/**
		 * @brief Скорректировать позиции курсоров соавторов
		 */
		void aboutCorrectAdditionalCursors(int _position, int _charsRemoved, int _charsAdded);

		/**
		 * @brief Обработки изменения выделения
		 */
		void aboutSelectionChanged();

		/**
		 * @brief Сохранить состояние редактора
		 */
		void aboutSaveEditorState();

		/**
		 * @brief Загрузить состояние редактора
		 */
		void aboutLoadEditorState();

	private:
		/**
		 * @brief Очистить текущий блок от установленного в нём типа
		 */
		void cleanScenarioTypeFromBlock();

		/**
		 * @brief Применить заданный тип к текущему блоку редактора
		 * @param Тип блока
		 */
		void applyScenarioTypeToBlock(BusinessLogic::ScenarioBlockStyle::Type _blockType);

		/**
		 * @brief Применить стиль группы к блоку группе
		 * @param Тип для применения
		 */
		void applyScenarioGroupTypeToGroupBlock(BusinessLogic::ScenarioBlockStyle::Type _blockType);


		/**
		 * @brief Скорректировать введённый текст
		 *
		 * - изменить регистр текста, если это необходимо
		 * - убрать лишние пробелы
		 */
		void updateEnteredText(QKeyEvent* _event);

		/**
		 * @brief Оканчивается ли строка сокращением
		 */
		bool stringEndsWithAbbrev(const QString& _text);

	private:
		void initEditor();
		void initEditorConnections();
		void removeEditorConnections();
		void initView();
		void initConnections();

	private:
		/**
		 * @brief Документ
		 */
		BusinessLogic::ScenarioTextDocument* m_document;

		/**
		 * @brief Количеств
		 */
		int m_mouseClicks;

		/**
		 * @brief Время последнего клика мышки, мс
		 */
		qint64 m_lastMouseClickTime;

		/**
		 * @brief Необходимо ли сохранять данные при вводе
		 */
		bool m_storeDataWhenEditing;

		/**
		 * @brief Отображать ли номер сцен
		 */
		bool m_showSceneNumbers;

		/**
		 * @brief Подсвечивать текущую линию
		 */
		bool m_highlightCurrentLine;

		/**
		 * @brief Использовать автозамены для особых случаев
		 */
		/** @{ */
		bool m_capitalizeFirstWord;
		bool m_correctDoubleCapitals;
		bool m_replaceThreeDots;
		bool m_smartQuotes;
		/** @} */

		/**
		 * @brief Показывать автодополения в пустых блоках
		 */
		bool m_showSuggestionsInEmptyBlocks;

		/**
		 * @brief Включена ли возможность выделения текста
		 */
		bool m_textSelectionEnable;

		/**
		 * @brief Курсоры соавторов
		 */
		QMap<QString, int> m_additionalCursors;

		/**
		 * @brief Скорректированные позиции курсоров, после локальных изменений текста
		 */
		QMap<QString, int> m_additionalCursorsCorrected;

		/**
		 * @brief Управляющий шорткатами
		 */
		ShortcutsManager* m_shortcutsManager;
	};
}

#endif // SCENARIOTEXTEDIT_H
