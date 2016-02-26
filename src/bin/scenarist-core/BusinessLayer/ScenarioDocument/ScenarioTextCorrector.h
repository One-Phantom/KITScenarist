#ifndef SCENARIOTEXTCORRECTOR_H
#define SCENARIOTEXTCORRECTOR_H

class QTextBlock;
class QTextCursor;
class QTextDocument;


namespace BusinessLogic
{
	class ScenarioTextDocument;

	/**
	 * @brief Класс корректирующий текст сценария
	 */
	class ScenarioTextCorrector
	{
	public:
		/**
		 * @brief Удалить декорации в заданном интервале текста. Если _endPosition равен нулю, то
		 *		  удалять до конца.
		 */
		static void removeDecorations(QTextDocument* _document, int _startPosition = 0, int _endPosition = 0);

		/**
		 * @brief Скорректировать сценарий на разрывах страниц
		 */
		static void correctScenarioText(ScenarioTextDocument* _document, int _startPosition, bool _force = false);

		/**
		 * @brief Скорректировать документ на разрывах страниц
		 * @note Используется при экспорте
		 */
		static void correctDocumentText(QTextDocument* _document, int _startPosition = 0);
	};
}

#endif // SCENARIOTEXTCORRECTOR_H
