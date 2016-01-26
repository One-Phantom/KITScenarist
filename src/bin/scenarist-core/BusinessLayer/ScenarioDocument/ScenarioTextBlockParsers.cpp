#include "ScenarioTextBlockParsers.h"

#include "ScenarioTemplate.h"

#include <QString>
#include <QStringList>
#include <QRegularExpression>

using namespace BusinessLogic;


CharacterParser::Section CharacterParser::section(const QString& _text)
{
	CharacterParser::Section section = SectionUndefined;

	if (_text.split("(").count() == 2) {
		section = SectionState;
	} else {
		section = SectionName;
	}

	return section;
}

QString CharacterParser::name(const QString& _text)
{
	//
	// В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
	// эти указания даются в скобках
	//

	QString name = _text;
	return name.remove(QRegularExpression("[(](.*)")).simplified().toUpper();
}

QString CharacterParser::state(const QString& _text)
{
	//
	// В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
	// эти указания даются в скобках, они нам как раз и нужны
	//

	const QRegularExpression rx_state("[(](.*)");
	QRegularExpressionMatch match = rx_state.match(_text);
	QString state;
	if (match.hasMatch()) {
		state = match.captured(0);
		state = state.remove("(").remove(")");
	}
	return state.toUpper();
}

// ****

SceneHeadingParser::Section SceneHeadingParser::section(const QString& _text)
{
	SceneHeadingParser::Section section = SectionUndefined;

	if (_text.split(", ").count() == 2) {
		section = SectionScenarioDay;
	} else if (_text.split(" - ").count() >= 2) {
		section = SectionTime;
	} else {
		const int splitDotCount = _text.split(". ").count();
		if (splitDotCount == 1) {
			section = SectionPlace;
		} else {
			section = SectionLocation;
		}
	}

	return section;
}

QString SceneHeadingParser::placeName(const QString& _text)
{
	QString placeName;

	if (_text.split(". ").count() > 0) {
		placeName = _text.split(". ").value(0);
	}

	return placeName.toUpper();
}

QString SceneHeadingParser::locationName(const QString& _text, bool _force)
{
	QString locationName;

	if (_text.split(". ").count() > 1) {
		locationName = _text.mid(_text.indexOf(". ") + 2);
		if (!_force) {
			const QString suffix = locationName.split(" - ").last();
			locationName = locationName.remove(" - " + suffix);
			locationName = locationName.simplified();
		}
	}

	return locationName.toUpper();
}

QString SceneHeadingParser::scenarioDayName(const QString& _text)
{
	QString scenarioDayName;

	if (_text.split(", ").count() == 2) {
		scenarioDayName = _text.split(", ").last();
	}

	return scenarioDayName.toUpper();
}

QString SceneHeadingParser::timeName(const QString& _text)
{
	QString timeName;

	if (_text.split(" - ").count() >= 2) {
		timeName = _text.split(" - ").last().split(",").first();
	}

	return timeName.toUpper();
}

// ****

QStringList SceneCharactersParser::characters(const QString& _text)
{
	QString characters = _text.simplified();

	//
	// Удалим потенциальные приставку и окончание
	//
	ScenarioBlockStyle style = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::SceneCharacters);
	QString stylePrefix = style.prefix();
	if (!stylePrefix.isEmpty()
		&& characters.startsWith(stylePrefix)) {
		characters.remove(QRegularExpression(QString("^[%1]").arg(stylePrefix)));
	}
	QString stylePostfix = style.postfix();
	if (!stylePostfix.isEmpty()
		&& characters.endsWith(stylePostfix)) {
		characters.remove(QRegularExpression(QString("[%1]$").arg(stylePostfix)));
	}

	QStringList charactersList = characters.split(",", QString::SkipEmptyParts);

	//
	// Убираем символы пробелов и поднимаем регистр
	//
	for (int index = 0; index < charactersList.size(); ++index) {
		charactersList[index] = charactersList[index].simplified().toUpper();
	}

	return charactersList;
}
