#ifndef SIMPLETEXTEDITOR_H
#define SIMPLETEXTEDITOR_H

#include <3rd_party/Widgets/SpellCheckTextEdit/SpellCheckTextEdit.h>

class QGestureEvent;


/**
 * @brief Простейший редактор текста
 */
class SimpleTextEditor : public SpellCheckTextEdit
{
	Q_OBJECT

public:
    /**
     * @brief Включить/выключить проверку правописания
     */
    static void enableSpellCheck(bool _enable, SpellChecker::Language _language = SpellChecker::Undefined);

public:
	explicit SimpleTextEditor(QWidget *parent = 0);
	~SimpleTextEditor();

    /**
     * @brief Методы настройки формата
     */
    /** @{ */
    void setTextBold();
    void setTextUnderline();
    void setTextItalic();
    void setTextColor(const QColor& _color);
    void setTextBackgroundColor(const QColor& _color);
    void setTextFont(const QFont& _font);
    /** @} */

protected:
	/**
	 * @brief Переопределяется для добавления пунктов форматирования текста
	 */
    void contextMenuEvent(QContextMenuEvent* _event);

	/**
     * @brief Вставляется только простой текст
	 */
    void insertFromMimeData(const QMimeData* _source);

private:
	void setupMenu();
	void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void mergeFormatOnParagraphOrSelection(const QTextCharFormat &format);

	/**
	 * @brief Установить масштабирование
	 */
	void setZoomRange(int _zoomRange);

private:
	QAction* actionTextBold;
	QAction* actionTextUnderline;
    QAction* actionTextItalic;

	/**
	 * @brief Синхронизация масштабирования всех редакторов данного типа
	 */
    static QList<SimpleTextEditor*> s_editors;

    /**
     * @brief Даём виджету доступ к защищённым членам класса
     */
    friend class SimpleTextEditorWidget;
};

#endif // SIMPLETEXTEDITOR_H
