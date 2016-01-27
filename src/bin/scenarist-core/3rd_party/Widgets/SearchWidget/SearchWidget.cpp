#include "SearchWidget.h"

#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBlock>


SearchWidget::SearchWidget(QWidget* _parent) :
	QFrame(_parent),
	m_editor(0),
	m_searchText(new QLineEdit(this)),
	m_caseSensitive(new QPushButton(this)),
	m_prevMatch(new QPushButton(this)),
	m_nextMatch(new QPushButton(this)),
	m_replaceText(new QLineEdit(this)),
	m_replaceOne(new QPushButton(this)),
	m_replaceAll(new QPushButton(this))
{
	setFrameShape(QFrame::Box);
	setStyleSheet("*[searchWidget=\"true\"] {"
				  "  border: 0px solid black; "
				  "  border-top-width: 1px; "
				  "  border-top-style: solid; "
				  "  border-top-color: palette(dark);"
				  "}"
				  "*[middle=\"true\"] { border-left: 0; border-radius: 0; min-width: 20px; }"
				  "*[last=\"true\"] { border-left: 0; border-top-left-radius: 0; border-bottom-left-radius: 0; min-width: 20px; }"
				  );
	setProperty("searchWidget", true);
	m_caseSensitive->setProperty("middle", true);
	m_prevMatch->setProperty("middle", true);
	m_nextMatch->setProperty("last", true);
	m_replaceOne->setProperty("middle", true);
	m_replaceAll->setProperty("last", true);

	setFocusProxy(m_searchText);
	connect(m_searchText, SIGNAL(textChanged(QString)), this, SLOT(aboutFindNext()));
	connect(m_searchText, SIGNAL(returnPressed()), this, SLOT(aboutFindNext()));
	m_searchText->setPlaceholderText(tr("Find..."));

	m_caseSensitive->setFixedWidth(20);
	m_caseSensitive->setCheckable(true);
	m_caseSensitive->setText("Aa");
	QFont caseSensitiveFont = m_caseSensitive->font();
	caseSensitiveFont.setItalic(true);
	m_caseSensitive->setFont(caseSensitiveFont);
	m_caseSensitive->setToolTip(tr("Case Sensitive"));

	m_prevMatch->setFixedWidth(20);
	m_prevMatch->setText(
#ifdef Q_OS_WIN
				QSysInfo::windowsVersion() == QSysInfo::WV_XP ? "◄" : "◀"
#else
				"◀"
#endif
				);
	m_prevMatch->setShortcut(QKeySequence("Shift+F3"));
	m_prevMatch->setToolTip(tr("Find Prev"));
	connect(m_prevMatch, SIGNAL(clicked()), this, SLOT(aboutFindPrev()));

	m_nextMatch->setFixedWidth(20);
	m_nextMatch->setText(
#ifdef Q_OS_WIN
				QSysInfo::windowsVersion() == QSysInfo::WV_XP ? "►" : "▶"
#else
				"▶"
#endif
				);
	m_nextMatch->setShortcut(QKeySequence("F3"));
	m_nextMatch->setToolTip(tr("Find Next"));
	connect(m_nextMatch, SIGNAL(clicked()), this, SLOT(aboutFindNext()));

	m_replaceText->setPlaceholderText(tr("Replace with..."));

	m_replaceOne->setText(tr("Replace"));
	connect(m_replaceOne, SIGNAL(clicked()), this, SLOT(aboutReplaceOne()));

	m_replaceAll->setFixedWidth(50);
	m_replaceAll->setText(tr("All"));
	connect(m_replaceAll, SIGNAL(clicked()), this, SLOT(aboutReplaceAll()));

	QHBoxLayout* layout = new QHBoxLayout;
	layout->setSpacing(0);
	layout->addWidget(m_searchText);
	layout->addWidget(m_caseSensitive);
	layout->addWidget(m_prevMatch);
	layout->addWidget(m_nextMatch);
	layout->addSpacing(16);
	layout->addWidget(m_replaceText);
	layout->addWidget(m_replaceOne);
	layout->addWidget(m_replaceAll);

	setLayout(layout);
}

void SearchWidget::setEditor(PageTextEdit* _editor)
{
	if (m_editor != _editor) {
		m_editor = _editor;
	}
}

void SearchWidget::selectText()
{
	m_searchText->selectAll();
}

void SearchWidget::setSearchOnly(bool _isSearchOnly)
{
	m_replaceText->setVisible(!_isSearchOnly);
	m_replaceOne->setVisible(!_isSearchOnly);
	m_replaceAll->setVisible(!_isSearchOnly);
}

void SearchWidget::aboutFindNext()
{
	findText(false);
}

void SearchWidget::aboutFindPrev()
{
	findText(true);
}

void SearchWidget::aboutReplaceOne()
{
	const QString replaceText = m_replaceText->text();
	if (m_editor) {
		const QString searchText = m_searchText->text();
		QTextCursor cursor = m_editor->textCursor();
		bool selectedTextEqual =
			m_caseSensitive->isChecked()
			? cursor.selectedText() == searchText
			: cursor.selectedText().toLower() == searchText.toLower();
		if (selectedTextEqual) {
			cursor.insertText(replaceText);
			aboutFindNext();
		}
	}
}

void SearchWidget::aboutReplaceAll()
{
	const QString replaceText = m_replaceText->text();
	if (m_editor) {
		aboutFindNext();
		QTextCursor cursor = m_editor->textCursor();
		cursor.beginEditBlock();
		const int firstCursorPosition = cursor.selectionStart();
		while (cursor.hasSelection()) {
			cursor.insertText(replaceText);
			aboutFindNext();
			cursor = m_editor->textCursor();

			//
			// Прерываем случай, когда пользователь пытается заменить слово без учёта регистра
			// на такое же, например "иван" на "Иван", но т.к. поиск производится без учёта регистра,
			// он зацикливается
			//
			if (cursor.selectionStart() == firstCursorPosition) {
				break;
			}
		}
		cursor.endEditBlock();
	}
}

void SearchWidget::findText(bool _backward)
{
	const QString searchText = m_searchText->text();
	if (!searchText.isEmpty()
		&& m_editor != 0) {
		//
		// Поиск осуществляется от позиции курсора
		//
		QTextCursor cursor = m_editor->textCursor();
		if (searchText != m_lastSearchText) {
			cursor.setPosition(qMin(cursor.selectionStart(),cursor.selectionEnd()));
		}

		//
		// Настроить направление поиска
		//
		QTextDocument::FindFlags findFlags;
		if (_backward) {
			findFlags |= QTextDocument::FindBackward;
		}
		//
		// Учёт регистра
		//
		if (m_caseSensitive->isChecked()) {
			findFlags |= QTextDocument::FindCaseSensitively;
		}

		//
		// Поиск
		//
		bool searchRestarted = false;
		do {
			cursor = m_editor->document()->find(searchText, cursor, findFlags);
			if (!cursor.isNull()) {
				m_editor->setTextCursor(cursor);
			} else {
				//
				// Если достигнут конец, или начало документа зацикливаем поиск, если это первый проход
				//
				if (searchRestarted == false) {
					searchRestarted = true;
					cursor = m_editor->textCursor();
					cursor.movePosition(_backward ? QTextCursor::End : QTextCursor::Start);
					cursor = m_editor->document()->find(searchText, cursor, findFlags);
					if (!cursor.isNull()) {
						m_editor->setTextCursor(cursor);
					}
				} else {
					break;
				}
			}
		} while (!cursor.block().isVisible());
	}

	//
	// Сохраняем искомый текст
	//
	m_lastSearchText = searchText;
}
