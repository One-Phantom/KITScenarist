#include "SimpleTextEditor.h"

#include <QAction>
#include <QApplication>
#include <QGestureEvent>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QMenu>
#include <QMimeData>
#include <QContextMenuEvent>
#include <QSettings>
#include <QShortcut>


void SimpleTextEditor::enableSpellCheck(bool _enable, SpellChecker::Language _language)
{
    //
    // Для каждого редактора
    //
    foreach (SimpleTextEditor* editor, s_editors) {
        editor->setUseSpellChecker(_enable);
        editor->setSpellCheckLanguage(_language);
        editor->setHighlighterDocument(editor->document());
    }
}

SimpleTextEditor::SimpleTextEditor(QWidget *parent) :
    SpellCheckTextEdit(parent)
{
	setAddSpaceToBottom(false);
	setTabChangesFocus(true);
	setUsePageMode(false);
    setPageMargins(QMarginsF(2, 2, 2, 2));

	setupMenu();

	//
	// Обновляем пункты меню
	//
    connect(this, &SimpleTextEditor::currentCharFormatChanged,
            [=] (const QTextCharFormat& _format) {
        QFont formatFont = _format.font();
        actionTextBold->setChecked(formatFont.bold());
        actionTextItalic->setChecked(formatFont.italic());
        actionTextUnderline->setChecked(formatFont.underline());
    });

	//
	// Подготовить редактор к синхронизации
	//
    s_editors.append(this);
}

SimpleTextEditor::~SimpleTextEditor()
{
	s_editors.removeOne(this);
}

void SimpleTextEditor::setTextBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void SimpleTextEditor::setTextUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void SimpleTextEditor::setTextItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void SimpleTextEditor::setTextColor(const QColor& _color)
{
    QTextCharFormat fmt;
    fmt.setForeground(_color);
    mergeFormatOnWordOrSelection(fmt);
}

void SimpleTextEditor::setTextBackgroundColor(const QColor& _color)
{
    QTextCharFormat fmt;
    fmt.setBackground(_color);
    mergeFormatOnWordOrSelection(fmt);
}

void SimpleTextEditor::setTextFont(const QFont& _font)
{
    QTextCharFormat fmt;
    fmt.setFont(_font);
    mergeFormatOnParagraphOrSelection(fmt);
}

void SimpleTextEditor::contextMenuEvent(QContextMenuEvent* _event)
{
	//
	// Сформируем  контекстное меню
	//
    QMenu* menu = createContextMenu(_event->globalPos());

	if (!isReadOnly()) {
		//
		// Добавим действия настройки стиля
		//
		QAction* actionInsertBefore = 0;
		if (menu->actions().count() > 0) {
			actionInsertBefore = menu->actions().first();
		}
		menu->insertAction(actionInsertBefore, actionTextBold);
		menu->insertAction(actionInsertBefore, actionTextItalic);
		menu->insertAction(actionInsertBefore, actionTextUnderline);
		menu->insertSeparator(actionInsertBefore);
	}

	//
	// Покажем меню, а после очистим от него память
	//
	menu->exec(_event->globalPos());
	delete menu;
}

void SimpleTextEditor::insertFromMimeData(const QMimeData* _source)
{
	if (_source->hasText()) {
		textCursor().insertText(_source->text());
	}
}

void SimpleTextEditor::setupMenu()
{
	actionTextBold = new QAction(tr("Bold"), this);
	actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
	actionTextBold->setShortcutContext(Qt::WidgetShortcut);
	actionTextBold->setPriority(QAction::LowPriority);
	QFont bold;
	bold.setBold(true);
	actionTextBold->setFont(bold);
    connect(actionTextBold, &QAction::triggered, this, &SimpleTextEditor::setTextBold);
	actionTextBold->setCheckable(true);

	actionTextItalic = new QAction(tr("Italic"), this);
	actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
	actionTextItalic->setShortcutContext(Qt::WidgetShortcut);
	actionTextItalic->setPriority(QAction::LowPriority);
	QFont italic;
	italic.setItalic(true);
	actionTextItalic->setFont(italic);
    connect(actionTextItalic, &QAction::triggered, this, &SimpleTextEditor::setTextItalic);
	actionTextItalic->setCheckable(true);

	actionTextUnderline = new QAction(tr("Underline"), this);
	actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
	actionTextUnderline->setShortcutContext(Qt::WidgetShortcut);
	actionTextUnderline->setPriority(QAction::LowPriority);
	QFont underline;
	underline.setUnderline(true);
	actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, &QAction::triggered, this, &SimpleTextEditor::setTextUnderline);
	actionTextUnderline->setCheckable(true);

	//
	// Добавим действия виджету
	//
	addAction(actionTextBold);
	addAction(actionTextItalic);
	addAction(actionTextUnderline);
}

void SimpleTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat& format)
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection())
		cursor.select(QTextCursor::WordUnderCursor);
	cursor.mergeCharFormat(format);
    mergeCurrentCharFormat(format);
}

void SimpleTextEditor::mergeFormatOnParagraphOrSelection(const QTextCharFormat& format)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::BlockUnderCursor);
    cursor.mergeCharFormat(format);
    mergeCurrentCharFormat(format);
}

QList<SimpleTextEditor*> SimpleTextEditor::s_editors;
