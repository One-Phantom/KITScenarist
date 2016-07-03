#include "SimpleTextEditorWidget.h"
#include "SimpleTextEditor.h"

#include <3rd_party/Widgets/ColoredToolButton/ColoredToolButton.h>
#include <3rd_party/Widgets/FlatButton/FlatButton.h>
#include <3rd_party/Widgets/ScalableWrapper/ScalableWrapper.h>

#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>


QVector<SimpleTextEditorWidget*> SimpleTextEditorWidget::s_editorsWidgets;

void SimpleTextEditorWidget::enableSpellCheck(bool _enable, SpellChecker::Language _language)
{
    //
    // Для каждого редактора
    //
    foreach (SimpleTextEditorWidget* editorWidget, s_editorsWidgets) {
        editorWidget->m_editor->setUseSpellChecker(_enable);
        editorWidget->m_editor->setSpellCheckLanguage(_language);
        editorWidget->m_editor->setHighlighterDocument(editorWidget->m_editor->document());
    }
}

SimpleTextEditorWidget::SimpleTextEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_editor(new SimpleTextEditor(this)),
    m_editorWrapper(new ScalableWrapper(m_editor, this)),
    m_toolbar(new QWidget(this)),
    m_textFont(new QComboBox(this)),
    m_textFontSize(new QComboBox(this)),
    m_textBold(new FlatButton(this)),
    m_textItalic(new FlatButton(this)),
    m_textUnderline(new FlatButton(this)),
    m_textColor(new ColoredToolButton(QIcon(":/Graphics/Icons/unknown.png"), this)),
    m_textBackgroundColor(new ColoredToolButton(QIcon(":/Graphics/Icons/unknown.png"), this)),
    m_toolbarSpace(new QLabel(this))
{
    initView();
    initConnections();
    initStyleSheet();

    s_editorsWidgets.append(this);
}

SimpleTextEditorWidget::~SimpleTextEditorWidget()
{
    s_editorsWidgets.removeOne(this);
}

void SimpleTextEditorWidget::setToolbarVisible(bool _visible)
{
    m_toolbar->setVisible(_visible);
}

void SimpleTextEditorWidget::setReadOnly(bool _readOnly)
{
    m_toolbar->setEnabled(!_readOnly);
    m_editor->setReadOnly(_readOnly);
}

QString SimpleTextEditorWidget::toHtml() const
{
    return m_editor->toHtml();
}

void SimpleTextEditorWidget::setHtml(const QString& _html)
{
    m_editor->setHtml(_html);
}

void SimpleTextEditorWidget::initView()
{
    //
    // Обновить масштаб
    //
    QSettings settings;
    m_editorWrapper->setZoomRange(settings.value("simple-editor/zoom-range", 0).toDouble());

    m_textFont->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_textFontSize->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_textColor->setIconSize(QSize(20, 20));
    m_textColor->setColorsPane(ColoredToolButton::Google);
    m_textBackgroundColor->setIconSize(QSize(20, 20));
    m_textBackgroundColor->setColorsPane(ColoredToolButton::Google);
    m_toolbarSpace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    //
    // Настроим панель инструментов
    //
    QHBoxLayout* toolbarLayout = new QHBoxLayout;
    toolbarLayout->setSpacing(0);
    toolbarLayout->setContentsMargins(QMargins());
    toolbarLayout->addWidget(m_textFont);
    toolbarLayout->addWidget(m_textFontSize);
    toolbarLayout->addWidget(m_textBold);
    toolbarLayout->addWidget(m_textItalic);
    toolbarLayout->addWidget(m_textUnderline);
    toolbarLayout->addWidget(m_textColor);
    toolbarLayout->addWidget(m_textBackgroundColor);
    toolbarLayout->addWidget(m_toolbarSpace);
    m_toolbar->setLayout(toolbarLayout);
    //
    // Настроим осноную компоновку виджета
    //
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());
    layout->addWidget(m_toolbar);
    layout->addWidget(m_editorWrapper);
    setLayout(layout);
}

void SimpleTextEditorWidget::initConnections()
{
    connect(m_editor, &SimpleTextEditor::textChanged, this, &SimpleTextEditorWidget::textChanged);

    connect(m_editorWrapper, &ScalableWrapper::zoomRangeChanged,
        [=] (const qreal _zoomRange) {
        //
        // Сохранить значение масштаба
        //
        QSettings settings;
        if (settings.value("simple-editor/zoom-range").toDouble() != _zoomRange) {
            settings.setValue("simple-editor/zoom-range", _zoomRange);
        }

        //
        // Обновить значение для всех простых редакторов
        //
        foreach (SimpleTextEditorWidget* editorWidget, s_editorsWidgets) {
            editorWidget->m_editorWrapper->setZoomRange(_zoomRange);
        }

        //
        // Перерисуем редактор, чтобы обновить его размер в обёртке
        //
        m_editor->update();
    });
}

void SimpleTextEditorWidget::initStyleSheet()
{
    m_textFont->setProperty("inTopPanel", true);
    m_textFont->setProperty("topPanelTopBordered", true);
    m_textFont->setProperty("topPanelLeftBordered", true);
    m_textFont->setProperty("topPanelRightBordered", true);
    m_textFontSize->setProperty("inTopPanel", true);
    m_textFontSize->setProperty("topPanelTopBordered", true);
    m_textFontSize->setProperty("topPanelRightBordered", true);
    m_textBold->setProperty("inTopPanel", true);
    m_textItalic->setProperty("inTopPanel", true);
    m_textUnderline->setProperty("inTopPanel", true);
    m_textColor->setProperty("inTopPanel", true);
    m_textBackgroundColor->setProperty("inTopPanel", true);
    m_toolbarSpace->setProperty("inTopPanel", true);
    m_toolbarSpace->setProperty("topPanelTopBordered", true);
    m_toolbarSpace->setProperty("topPanelRightBordered", true);

    m_editorWrapper->setProperty("nobordersContainer", true);
}
