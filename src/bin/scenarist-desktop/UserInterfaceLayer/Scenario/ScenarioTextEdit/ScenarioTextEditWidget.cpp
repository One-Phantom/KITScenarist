#include "ScenarioTextEditWidget.h"

#include "ScenarioTextEdit.h"
#include "ScenarioTextEditHelpers.h"
#include "ScenarioFastFormatWidget.h"
#include "ScenarioReviewPanel.h"
#include "ScenarioReviewView.h"

#include <3rd_party/Helpers/ShortcutHelper.h>
#include <3rd_party/Widgets/FlatButton/FlatButton.h>
#include <3rd_party/Widgets/ScalableWrapper/ScalableWrapper.h>
#include <3rd_party/Widgets/SearchWidget/SearchWidget.h>

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>
#include <BusinessLayer/Chronometry/ChronometerFacade.h>

#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QTreeView>
#include <QHeaderView>
#include <QStandardItemModel>

using UserInterface::ScenarioTextEditWidget;
using UserInterface::ScenarioReviewPanel;
using UserInterface::ScenarioReviewView;
using UserInterface::ScenarioTextEdit;
using BusinessLogic::ScenarioTemplateFacade;
using BusinessLogic::ScenarioTemplate;
using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScenarioTextBlockInfo;

namespace {
	const int SCROLL_DELTA = 140;

	/**
	 * @brief Получить хэш текста
	 */
	static QByteArray textMd5Hash(const QString& _text) {
		QCryptographicHash hash(QCryptographicHash::Md5);
		hash.addData(_text.toUtf8());
		return hash.result();
	}
}


ScenarioTextEditWidget::ScenarioTextEditWidget(QWidget* _parent) :
	QFrame(_parent),
	m_editor(new ScenarioTextEdit(this)),
	m_editorWrapper(new ScalableWrapper(m_editor, this)),
	m_toolbar(new QWidget(this)),
	m_textStyles(new QComboBox(this)),
	m_undo(new FlatButton(this)),
	m_redo(new FlatButton(this)),
	m_search(new FlatButton(this)),
	m_fastFormat(new FlatButton(this)),
	m_review(new ScenarioReviewPanel(m_editor, this)),
	m_duration(new QLabel(this)),
	m_countersInfo(new QLabel(this)),
	m_searchLine(new SearchWidget(this)),
	m_fastFormatWidget(new ScenarioFastFormatWidget(this)),
	m_reviewView(new ScenarioReviewView(this))
{
	initView();
	initConnections();
	initStyleSheet();
}

QWidget* ScenarioTextEditWidget::toolbar() const
{
	return m_toolbar;
}

BusinessLogic::ScenarioTextDocument* ScenarioTextEditWidget::scenarioDocument() const
{
	return qobject_cast<BusinessLogic::ScenarioTextDocument*>(m_editor->document());
}

void ScenarioTextEditWidget::setScenarioDocument(BusinessLogic::ScenarioTextDocument* _document, bool _isDraft)
{
	removeEditorConnections();

	m_editor->setScenarioDocument(_document);
	m_editor->setWatermark(_isDraft ? tr("DRAFT") : QString::null);
	if (_document != 0) {
		m_lastTextMd5Hash = ::textMd5Hash(_document->toPlainText());
	}

	initEditorConnections();
}

void ScenarioTextEditWidget::setDuration(const QString& _duration)
{
	m_duration->setText(_duration);
}

void ScenarioTextEditWidget::setCountersInfo(const QString& _counters)
{
	m_countersInfo->setText(_counters);
}

void ScenarioTextEditWidget::setShowScenesNumbers(bool _show)
{
	m_editor->setShowSceneNumbers(_show);
}

void ScenarioTextEditWidget::setHighlightCurrentLine(bool _highlight)
{
	m_editor->setHighlightCurrentLine(_highlight);
}

void ScenarioTextEditWidget::setAutoReplacing(bool _replacing)
{
	m_editor->setAutoReplacing(_replacing);
}

void ScenarioTextEditWidget::setUsePageView(bool _use)
{
	//
	// Установка постраничного режима так же тянет за собой ряд настроек
	//
	QMarginsF pageMargins(15, 5, 5, 5);
	Qt::Alignment pageNumbersAlign;
	if (_use) {
		pageMargins = ScenarioTemplateFacade::getTemplate().pageMargins();
		pageNumbersAlign = ScenarioTemplateFacade::getTemplate().numberingAlignment();
	}

	m_editor->setUsePageMode(_use);
	m_editor->setPageMargins(pageMargins);
	m_editor->setPageNumbersAlignment(pageNumbersAlign);

	//
	// В дополнение установим шрифт по умолчанию для документа (шрифтом будет рисоваться нумерация)
	//
	m_editor->document()->setDefaultFont(
		ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Action).font());
}

void ScenarioTextEditWidget::setUseSpellChecker(bool _use)
{
	m_editor->setUseSpellChecker(_use);
}

void ScenarioTextEditWidget::setSpellCheckLanguage(int _language)
{
	m_editor->setSpellCheckLanguage((SpellChecker::Language)_language);
}

void ScenarioTextEditWidget::setTextEditColors(const QColor& _textColor, const QColor& _backgroundColor)
{
	m_editor->viewport()->setStyleSheet(QString("color: %1; background-color: %2;").arg(_textColor.name(), _backgroundColor.name()));
	m_editor->setStyleSheet(QString("#scenarioEditor { color: %1; }").arg(_textColor.name()));
}

void ScenarioTextEditWidget::setTextEditZoomRange(qreal _zoomRange)
{
	m_editorWrapper->setZoomRange(_zoomRange);
}

int ScenarioTextEditWidget::cursorPosition() const
{
	return m_editor->textCursor().position();
}

void ScenarioTextEditWidget::setCursorPosition(int _position)
{
	//
	// Устанавливаем позицию курсора
	//
	QTextCursor cursor = m_editor->textCursor();

	//
	// Если это новая позиция
	//
	if (cursor.position() != _position) {
		//
		// Устанавливаем реальную позицию
		//
		if (_position < m_editor->document()->characterCount()) {
			cursor.setPosition(_position);
		} else {
			cursor.movePosition(QTextCursor::End);
		}
		m_editor->setTextCursor(cursor);
		m_editor->ensureCursorVisibleReimpl();
		//
		// Прокручиваем вниз, чтобы курсор стоял в верху экрана
		//
		m_editor->verticalScrollBar()->setValue(m_editor->verticalScrollBar()->maximum());

		//
		// Возвращаем курсор в поле зрения
		//
		m_editor->ensureCursorVisibleReimpl();
		m_editorWrapper->setFocus();
	}
	//
	// Если нужно обновить в текущей позиции курсора просто имитируем отправку сигнала
	//
	else {
		emit m_editor->cursorPositionChanged();
	}
}

void ScenarioTextEditWidget::addItem(int _position, int _type, const QString& _header,
	const QColor& _color, const QString& _description)
{
	QTextCursor cursor = m_editor->textCursor();
	cursor.beginEditBlock();

	cursor.setPosition(_position);
	m_editor->setTextCursor(cursor);
	ScenarioBlockStyle::Type type = (ScenarioBlockStyle::Type)_type;
	//
	// Если в позиции пустой блок, изменим его
	//
	if (cursor.block().text().isEmpty()) {
		m_editor->changeScenarioBlockType(type);
	}
	//
	// В противном случае добавим новый
	//
	else {
		m_editor->addScenarioBlock(type);
	}

	//
	// Устанавливаем текст в блок
	//
	m_editor->insertPlainText(_header);

	//
	// Устанавливаем цвет и описание
	//
	cursor = m_editor->textCursor();
	QTextBlockUserData* textBlockData = cursor.block().userData();
	ScenarioTextBlockInfo* info = dynamic_cast<ScenarioTextBlockInfo*>(textBlockData);
	if (info == 0) {
		info = new ScenarioTextBlockInfo;
	}
	info->setColors(_color.name());
	info->setDescription(_description);
	cursor.block().setUserData(info);

	//
	// Если это группирующий блок, то вставим и закрывающий текст
	//
	if (ScenarioTemplateFacade::getTemplate().blockStyle(type).isEmbeddableHeader()) {
		cursor = m_editor->textCursor();
		cursor.movePosition(QTextCursor::NextBlock);
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.insertText(Helpers::footerText(_header));
	}

	cursor.endEditBlock();
}

void ScenarioTextEditWidget::removeText(int _from, int _to)
{
	QTextCursor cursor = m_editor->textCursor();
	cursor.beginEditBlock();

	//
	// Стираем текст
	//
	cursor.setPosition(_from);
	cursor.setPosition(_to, QTextCursor::KeepAnchor);
	cursor.removeSelectedText();

	//
	// Если остаётся пустой блок, стираем его тоже
	//
	if (cursor.block().text().isEmpty()) {
		//
		// Стирать необходимо через имитацию удаления редактором,
		// для корректного обновления модели сцен
		//
		QKeyEvent* event = 0;
		if (cursor.atStart()) {
			event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
		} else {
			event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
		}
		QApplication::sendEvent(m_editor, event);
	}

	cursor.endEditBlock();
}

void ScenarioTextEditWidget::updateStylesElements()
{
	//
	// Обновить выпадающий список стилей сценария
	//
	m_textStyles->clear();
	initStylesCombo();

	//
	// Обновить виджет быстрого форматирования
	//
	m_fastFormatWidget->reinitBlockStyles();
}

void ScenarioTextEditWidget::updateShortcuts()
{
	m_editor->updateShortcuts();
	updateStylesCombo();
}

void ScenarioTextEditWidget::setAdditionalCursors(const QMap<QString, int>& _cursors)
{
	m_editor->setAdditionalCursors(_cursors);
}

void ScenarioTextEditWidget::setCommentOnly(bool _isCommentOnly)
{
	m_textStyles->setEnabled(!_isCommentOnly);
	m_undo->setVisible(!_isCommentOnly);
	m_redo->setVisible(!_isCommentOnly);
	m_fastFormat->setVisible(!_isCommentOnly);
	m_editor->setReadOnly(_isCommentOnly);
	m_searchLine->setSearchOnly(_isCommentOnly);

	if (_isCommentOnly) {
		//
		// Закроем панель быстрого форматирования, если она была открыта
		//
		m_fastFormat->setChecked(false);
		m_fastFormatWidget->hide();
	}
}

void ScenarioTextEditWidget::aboutUndo()
{
	m_editor->undoReimpl();
}

void ScenarioTextEditWidget::aboutRedo()
{
	m_editor->redoReimpl();
}

void ScenarioTextEditWidget::aboutShowSearch()
{
	m_searchLine->setVisible(m_search->isChecked());
	if (m_searchLine->isVisible()) {
		m_searchLine->selectText();
		m_searchLine->setFocus();
	}
}

void ScenarioTextEditWidget::aboutShowFastFormat()
{
	m_fastFormatWidget->setVisible(m_fastFormat->isChecked());
	if (m_fastFormatWidget->isVisible()) {
		m_fastFormatWidget->setFocus();
		m_fastFormatWidget->selectCurrentBlock();
	}
}

void ScenarioTextEditWidget::aboutUpdateTextStyle()
{
	ScenarioBlockStyle::Type currentType = m_editor->scenarioBlockType();
	if (currentType == ScenarioBlockStyle::TitleHeader) {
		currentType = ScenarioBlockStyle::Title;
	} else if (currentType == ScenarioBlockStyle::SceneGroupFooter) {
		currentType = ScenarioBlockStyle::SceneGroupHeader;
	} else if (currentType == ScenarioBlockStyle::FolderFooter) {
		currentType = ScenarioBlockStyle::FolderHeader;
	}

	for (int itemIndex = 0; itemIndex < m_textStyles->count(); ++itemIndex) {
		ScenarioBlockStyle::Type itemType =
				(ScenarioBlockStyle::Type)m_textStyles->itemData(itemIndex).toInt();
		if (itemType == currentType) {
			m_textStyles->setCurrentIndex(itemIndex);
			break;
		}
	}
}

void ScenarioTextEditWidget::aboutChangeTextStyle()
{
	ScenarioBlockStyle::Type type =
			(ScenarioBlockStyle::Type)m_textStyles->itemData(m_textStyles->currentIndex()).toInt();

	//
	// Меняем стиль блока, если это возможно
	//
	m_editor->changeScenarioBlockType(type);
	m_editorWrapper->setFocus();
}

void ScenarioTextEditWidget::aboutCursorPositionChanged()
{
	emit cursorPositionChanged(m_editor->textCursor().position());
}

void ScenarioTextEditWidget::aboutTextChanged()
{
	QByteArray currentTextMd5Hash = textMd5Hash(m_editor->document()->toPlainText());
	if (m_lastTextMd5Hash != currentTextMd5Hash) {
		emit textChanged();

		m_lastTextMd5Hash = currentTextMd5Hash;
	}
}

void ScenarioTextEditWidget::aboutStyleChanged()
{
	emit textChanged();
}

void ScenarioTextEditWidget::initView()
{
	m_textStyles->setToolTip(tr("Current Text Block Style"));
	m_textStyles->setSizePolicy(m_textStyles->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);

	initStylesCombo();

	m_undo->setIcons(QIcon(":/Graphics/Icons/Editing/undo.png"));
	m_undo->setToolTip(ShortcutHelper::makeToolTip(tr("Undo last action"), "Ctrl+Z"));

	m_redo->setIcons(QIcon(":/Graphics/Icons/Editing/redo.png"));
	m_redo->setToolTip(ShortcutHelper::makeToolTip(tr("Redo last action"), "Shift+Ctrl+Z"));

	m_search->setObjectName("scenarioSearch");
	m_search->setIcons(QIcon(":/Graphics/Icons/Editing/search.png"));
	m_search->setToolTip(ShortcutHelper::makeToolTip(tr("Search and Replace"), "Ctrl+F"));
	m_search->setCheckable(true);
	m_search->setShortcut(QKeySequence("Ctrl+F"));

	m_fastFormat->setObjectName("scenarioFastFormat");
	m_fastFormat->setIcons(QIcon(":/Graphics/Icons/Editing/format.png"));
	m_fastFormat->setToolTip(tr("Text Fast Format"));
	m_fastFormat->setCheckable(true);

	m_review->setObjectName("scenarioReview");

	m_duration->setToolTip(tr("Duration from Start to Cursor Position | Full Duration"));
	m_duration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_duration->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	m_editor->setObjectName("scenarioEditor");
	m_editor->setPageFormat(ScenarioTemplateFacade::getTemplate().pageSizeId());

	m_searchLine->setEditor(m_editor);
	m_searchLine->hide();

	m_fastFormatWidget->setEditor(m_editor);
	m_fastFormatWidget->hide();

	m_reviewView->setObjectName("reviewView");
	m_reviewView->setEditor(m_editor);
	m_reviewView->hide();

	QHBoxLayout* topLayout = new QHBoxLayout(m_toolbar);
	topLayout->setContentsMargins(QMargins());
	topLayout->setSpacing(0);
	topLayout->addWidget(m_textStyles);
	topLayout->addWidget(m_undo);
	topLayout->addWidget(m_redo);
	topLayout->addWidget(m_search);
	topLayout->addWidget(m_fastFormat);
	topLayout->addWidget(m_review);
	topLayout->addWidget(m_duration);
	topLayout->addWidget(m_countersInfo);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(QMargins());
	mainLayout->addWidget(m_toolbar);
	mainLayout->addWidget(m_editorWrapper);
	mainLayout->addWidget(m_searchLine);

	QSplitter* mainSplitter = new QSplitter(this);
	mainSplitter->setObjectName("mainScenarioSplitter");
	mainSplitter->setHandleWidth(1);
	mainSplitter->setOpaqueResize(false);
	QWidget* mainLayoutWidget = new QWidget(this);
	mainLayoutWidget->setObjectName("mainLayoutWidget");
	mainLayoutWidget->setLayout(mainLayout);
	mainSplitter->addWidget(mainLayoutWidget);
	mainSplitter->addWidget(m_reviewView);
	mainSplitter->setSizes(QList<int>() << 3 << 1);

	QHBoxLayout* layout = new QHBoxLayout;
	layout->setContentsMargins(QMargins());
	layout->setSpacing(0);
	layout->addWidget(mainSplitter);
	layout->addWidget(m_fastFormatWidget);

	setLayout(layout);
}

void ScenarioTextEditWidget::initStylesCombo()
{
	ScenarioTemplate style = ScenarioTemplateFacade::getTemplate();
	const bool BEAUTIFY_NAME = true;

	QList<ScenarioBlockStyle::Type> types;
	types << ScenarioBlockStyle::SceneHeading
		  << ScenarioBlockStyle::SceneCharacters
		  << ScenarioBlockStyle::Action
		  << ScenarioBlockStyle::Character
		  << ScenarioBlockStyle::Dialogue
		  << ScenarioBlockStyle::Parenthetical
		  << ScenarioBlockStyle::Title
		  << ScenarioBlockStyle::Note
		  << ScenarioBlockStyle::Transition
		  << ScenarioBlockStyle::NoprintableText
		  << ScenarioBlockStyle::SceneGroupHeader
		  << ScenarioBlockStyle::FolderHeader;

	foreach (ScenarioBlockStyle::Type type, types) {
		if (style.blockStyle(type).isActive()) {
			m_textStyles->addItem(ScenarioBlockStyle::typeName(type, BEAUTIFY_NAME), type);
		}
	}

	updateStylesCombo();
}

void ScenarioTextEditWidget::updateStylesCombo()
{
	for (int index = 0; index < m_textStyles->count(); ++index) {
		ScenarioBlockStyle::Type blockType =
				(ScenarioBlockStyle::Type)m_textStyles->itemData(index).toInt();
		m_textStyles->setItemData(index, m_editor->shortcut(blockType), Qt::ToolTipRole);
	}
}

void ScenarioTextEditWidget::initConnections()
{
	connect(m_textStyles, SIGNAL(activated(int)), this, SLOT(aboutChangeTextStyle()));
	connect(m_undo, SIGNAL(clicked()), this, SLOT(aboutUndo()));
	connect(m_redo, SIGNAL(clicked()), this, SLOT(aboutRedo()));
	connect(m_search, SIGNAL(toggled(bool)), this, SLOT(aboutShowSearch()));
	connect(m_fastFormat, SIGNAL(toggled(bool)), this, SLOT(aboutShowFastFormat()));
	connect(m_review, SIGNAL(toggled(bool)), m_reviewView, SLOT(setVisible(bool)));
	connect(m_reviewView, SIGNAL(undoPressed()), this, SLOT(aboutUndo()));
	connect(m_reviewView, SIGNAL(redoPressed()), this, SLOT(aboutRedo()));

	initEditorConnections();
}

void ScenarioTextEditWidget::initEditorConnections()
{
	connect(m_editor, SIGNAL(currentStyleChanged()), this, SLOT(aboutUpdateTextStyle()));
	connect(m_editor, SIGNAL(cursorPositionChanged()), this, SLOT(aboutUpdateTextStyle()));
	connect(m_editor, SIGNAL(cursorPositionChanged()), this, SLOT(aboutCursorPositionChanged()));
	connect(m_editor, SIGNAL(textChanged()), this, SLOT(aboutTextChanged()));
	connect(m_editor, SIGNAL(styleChanged()), this, SLOT(aboutStyleChanged()));
	connect(m_editor, SIGNAL(reviewChanged()), this, SIGNAL(textChanged()));
	connect(m_editorWrapper, SIGNAL(zoomRangeChanged(qreal)), this, SIGNAL(zoomRangeChanged(qreal)));
}

void ScenarioTextEditWidget::removeEditorConnections()
{
	disconnect(m_editor, SIGNAL(currentStyleChanged()), this, SLOT(aboutUpdateTextStyle()));
	disconnect(m_editor, SIGNAL(cursorPositionChanged()), this, SLOT(aboutUpdateTextStyle()));
	disconnect(m_editor, SIGNAL(cursorPositionChanged()), this, SLOT(aboutCursorPositionChanged()));
	disconnect(m_editor, SIGNAL(textChanged()), this, SLOT(aboutTextChanged()));
	disconnect(m_editor, SIGNAL(styleChanged()), this, SLOT(aboutStyleChanged()));
	disconnect(m_editor, SIGNAL(reviewChanged()), this, SIGNAL(textChanged()));
	disconnect(m_editorWrapper, SIGNAL(zoomRangeChanged(qreal)), this, SIGNAL(zoomRangeChanged(qreal)));
}

void ScenarioTextEditWidget::initStyleSheet()
{
	m_textStyles->setProperty("inTopPanel", true);
	m_textStyles->setProperty("topPanelTopBordered", true);
	m_textStyles->setProperty("topPanelRightBordered", true);

	m_undo->setProperty("inTopPanel", true);
	m_redo->setProperty("inTopPanel", true);
	m_search->setProperty("inTopPanel", true);
	m_fastFormat->setProperty("inTopPanel", true);

	m_duration->setProperty("inTopPanel", true);
	m_duration->setProperty("topPanelTopBordered", true);

	m_countersInfo->setProperty("inTopPanel", true);
	m_countersInfo->setProperty("topPanelTopBordered", true);
	m_countersInfo->setProperty("topPanelRightBordered", true);

	m_editorWrapper->setProperty("mainContainer", true);
}