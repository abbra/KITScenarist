#include "ScenarioTextCorrector.h"

#include "ScenarioTemplate.h"
#include "ScenarioTextBlockParsers.h"

#include <QApplication>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace BusinessLogic;

namespace {
	/**
	 * @brief Автоматически добавляемые продолжения в диалогах
	 */
	//: Continued
	static const char* DIALOG_CONTINUED = QT_TRANSLATE_NOOP("BusinessLogic::ScenarioTextCorrector", "CONT'D");

	/**
	 * @brief Курсор находится на границе сцены
	 */
	static bool cursorAtSceneBorder(const QTextCursor& _cursor) {
		return ScenarioBlockStyle::forBlock(_cursor.block()) == ScenarioBlockStyle::SceneHeading
				|| ScenarioBlockStyle::forBlock(_cursor.block()) == ScenarioBlockStyle::SceneGroupHeader
				|| ScenarioBlockStyle::forBlock(_cursor.block()) == ScenarioBlockStyle::SceneGroupFooter
				|| ScenarioBlockStyle::forBlock(_cursor.block()) == ScenarioBlockStyle::FolderHeader
				|| ScenarioBlockStyle::forBlock(_cursor.block()) == ScenarioBlockStyle::FolderFooter;
	}
}


ScenarioTextCorrector::ScenarioTextCorrector()
{

}

void ScenarioTextCorrector::correctScenarioText(QTextDocument* _document, int _startPosition)
{
	QTextCursor mainCursor(_document);
	mainCursor.setPosition(_startPosition);
	mainCursor.beginEditBlock();

	//
	// Сперва нужно подняться до начала сцены и начинать корректировки с этого положения
	//
	while (!mainCursor.atStart()
		   && !::cursorAtSceneBorder(mainCursor)) {
		mainCursor.movePosition(QTextCursor::PreviousBlock);
		mainCursor.movePosition(QTextCursor::StartOfBlock);
	}


	//
	// Для имён персонажей, нужно добавлять ПРОД (только, если имя полностью идентично предыдущему)
	//
	{
		QTextCursor cursor = mainCursor;

		//
		// Храним последнего персонажа сцены
		//
		QString lastSceneCharacter;
		while (!cursor.atEnd()) {
			if (ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::Character) {
				const QString character = CharacterParser::name(cursor.block().text());
				const bool isStartPositionInBlock =
					cursor.block().position() <= _startPosition
					&& cursor.block().position() + cursor.block().length() > _startPosition;
				//
				// Если имя текущего персонажа не пусто и курсор не находится в этом блоке
				//
				if (!character.isEmpty() && !isStartPositionInBlock) {
					//
					// Не второе подряд появление, удаляем из него вспомогательный текст, если есть
					//
					if (lastSceneCharacter.isEmpty()
						|| character != lastSceneCharacter) {
						foreach (const QTextLayout::FormatRange& range, cursor.block().textFormats()) {
							if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
								cursor.setPosition(cursor.block().position() + range.start);
								cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, range.length);
								cursor.removeSelectedText();
								break;
							}
						}
					}
					//
					// Если второе подряд, добавляем вспомогательный текст
					//
					else if (character == lastSceneCharacter){
						QString characterState = CharacterParser::state(cursor.block().text());
						if (characterState.isEmpty()) {
							//
							// ... вставляем текст
							//
							cursor.movePosition(QTextCursor::EndOfBlock);
							static const QString textForInsert =
								QString(" (%1)").arg(QApplication::translate("BusinessLogic::ScenarioTextCorrector", DIALOG_CONTINUED));
							cursor.insertText(textForInsert);
							//
							// ... настраиваем формат текста автодополнения
							//
							cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, textForInsert.length());
							QTextCharFormat format;
							format.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
							cursor.mergeCharFormat(format);
						}
					}

					lastSceneCharacter = character;
				}
			}
			//
			// Очищаем имя последнего, если текущая сцена закончилась
			//
			else if (::cursorAtSceneBorder(cursor)) {
				lastSceneCharacter.clear();
			}

			cursor.movePosition(QTextCursor::NextBlock);
			cursor.movePosition(QTextCursor::EndOfBlock);
		}
	}


	//
	// Переносить время и место на следующую страницу
	//

	//
	// Разрывать описание действия, или переносить
	//

	//
	// Разрывать диалоги, или переносить
	//


	mainCursor.endEditBlock();
}
