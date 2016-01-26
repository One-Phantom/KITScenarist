#include "Database.h"

#include <BusinessLayer/ScenarioDocument/ScenarioXml.h>

#include <3rd_party/Helpers/DiffMatchPatchHelper.h>

#include <QApplication>
#include <QDateTime>
#include <QRegularExpression>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextCodec>
#include <QUuid>
#include <QVariant>

using namespace DatabaseLayer;

namespace {
	/**
	 * @brief Получить ключ хранения номера версии приложения
	 */
	static QString applicationVersionKey() {
		return
#ifdef MOBILE_OS
				"application-version-mobile";
#else
				"application-version";
#endif
	}
}


bool Database::canOpenFile(const QString& _databaseFileName, bool _isLocal)
{
	bool canOpen = true;

	//
	// Проверки специфичные для локальных файлов
	//
	if (_isLocal) {
		QSqlDatabase database = QSqlDatabase::addDatabase(SQL_DRIVER, "tmp_database");
		database.setDatabaseName(_databaseFileName);
		database.open();

		QSqlQuery q_checker(database);

		//
		// 1. Если файл был создан в настольной версии приложения, его нельзя открывать в мобильной и наоборот
		//
		if (q_checker.exec(
#ifdef MOBILE_OS
				"SELECT COUNT(value) AS cant_open FROM system_variables WHERE variable = 'application-version' "
#else
				"SELECT COUNT(value) AS cant_open FROM system_variables WHERE variable = 'application-version-mobile' "
#endif
				)
			&& q_checker.next()
			&& q_checker.record().value("cant_open").toBool() == true) {
			//
			// Если есть метка противоположного приложения, то открыть нельзя
			//
			canOpen = false;
			//
			// ... сформироуем сообщение об ошибке
			//
			s_openFileError =
#ifdef MOBILE_OS
				QApplication::translate("DatabaseLayer::Database", "Project was created in desktop version.");
#else
				QApplication::translate("DatabaseLayer::Database", "Project was created in mobile version.");
#endif
		}
		//
		// 2. Если файл был создан в более поздней версии приложения, его нельзя открывать
		//
		else if (q_checker.exec("SELECT value FROM system_variables WHERE variable = 'application-version' ")
				 && q_checker.next()
				 && q_checker.value("value").toString().split(" ").first() > QApplication::applicationVersion()) {
			canOpen = false;
			s_openFileError =
				QApplication::translate("DatabaseLayer::Database",
					"Project was modified in higher version. You need update application to latest version for open it.");
		}
	}

	QSqlDatabase::removeDatabase("tmp_database");

	return canOpen;
}

QString Database::openFileError()
{
	return s_openFileError;
}

void Database::setCurrentFile(const QString& _databaseFileName)
{
	//
	// Если использовалась база данных, то удалим старое соединение
	//
	closeCurrentFile();

	//
	// Установим текущее имя базы данных
	//
	if (DATABASE_NAME != _databaseFileName) {
		DATABASE_NAME = _databaseFileName;
		CONNECTION_NAME = "local_database [" + DATABASE_NAME + "]";
	}

	//
	// Откроем базу данных, или создадим новую
	//
	instanse();
}

void Database::closeCurrentFile()
{
	if (QSqlDatabase::contains(CONNECTION_NAME)) {
		QSqlDatabase::removeDatabase(CONNECTION_NAME);
	}
}

QString Database::currentFile()
{
	return instanse().databaseName();
}

QSqlQuery Database::query()
{
	return QSqlQuery(instanse());
}

void Database::transaction()
{
	//
	// Для первого запроса открываем транзакцию
	//
	if (s_openedTransactions == 0) {
		instanse().transaction();
	}

	//
	// Увеличиваем счётчик открытых транзакций
	//
	++s_openedTransactions;
}

void Database::commit()
{
	//
	// Уменьшаем счётчик транзакций
	//
	--s_openedTransactions;

	//
	// При закрытии корневой транзакции фиксируем изменения в базе данных
	//
	if (s_openedTransactions == 0) {
		instanse().commit();
	}
}


//********
// Скрытая часть


QString Database::CONNECTION_NAME = "local_database";
QString Database::SQL_DRIVER      = "QSQLITE";
QString Database::DATABASE_NAME   = ":memory:";

QString Database::s_openFileError = QString::null;
int Database::s_openedTransactions = 0;

QSqlDatabase Database::instanse()
{
	QSqlDatabase database;

	if (!QSqlDatabase::contains(CONNECTION_NAME)) {
		open(database, CONNECTION_NAME, DATABASE_NAME);
	} else {
		database = QSqlDatabase::database(CONNECTION_NAME);
	}

	return database;
}

void Database::open(QSqlDatabase& _database, const QString& _connectionName, const QString& _databaseName)
{
	_database = QSqlDatabase::addDatabase(SQL_DRIVER, _connectionName);
	_database.setDatabaseName(_databaseName);
	_database.open();

	Database::States states = checkState(_database);

	if (!states.testFlag(SchemeFlag))
		createTables(_database);
	if (!states.testFlag(IndexesFlag))
		createIndexes(_database);
	if (!states.testFlag(EnumsFlag))
		createEnums(_database);
	if (states.testFlag(OldVersionFlag))
		updateDatabase(_database);
}

// Проверка состояния базы данных
// например:
// - БД отсутствует
// - БД пуста
// - БД имеет старую версию
// - БД имеет последнюю версию
// - и т.д.
Database::States Database::checkState(QSqlDatabase& _database)
{
	QSqlQuery q_checker(_database);
	Database::States states = Database::EmptyFlag;

	//
	// Созданы ли таблицы
	//
	if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'table' ") &&
		q_checker.next() &&
		q_checker.record().value("size").toInt()) {
		//
		// Все остальные проверки имеют смысл, только если проходит данная проверка
		//
		states = states | Database::SchemeFlag;

		//
		// Созданы ли индексы
		//
		if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'index' ") &&
			q_checker.next() &&
			q_checker.record().value("size").toInt()) {
			states = states | Database::IndexesFlag;
		}

		//
		// Проверка версии
		//
		if (q_checker.exec(
				QString("SELECT value as version FROM system_variables WHERE variable = '%1' ")
				.arg(::applicationVersionKey()))
			&& q_checker.next()
			&& q_checker.record().value("version").toString() != QApplication::applicationVersion()) {
			states = states | Database::OldVersionFlag;
		}
	}

	return states;
}

void Database::createTables(QSqlDatabase& _database)
{
	QSqlQuery q_creator(_database);
	_database.transaction();

	// Таблица с историей запросов
	q_creator.exec("CREATE TABLE _database_history "
				   "( "
				   "id TEXT PRIMARY KEY, " // uuid
				   "query TEXT NOT NULL, "
				   "query_values TEXT NOT NULL, "
				   "datetime TEXT NOT NULL "
				   "); "
				   );

	// Таблица системных переменных
	q_creator.exec("CREATE TABLE system_variables "
				   "( "
				   "variable TEXT PRIMARY KEY ON CONFLICT REPLACE, "
				   "value TEXT NOT NULL "
				   "); "
				   );

	// Таблица "Место"
	q_creator.exec("CREATE TABLE places "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Локация"
	q_creator.exec("CREATE TABLE locations "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL, "
				   "description TEXT DEFAULT(NULL) "
				   "); "
				   );

	// Таблица "Фотографии локаций"
	q_creator.exec("CREATE TABLE locations_photo "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "fk_location_id INTEGER NOT NULL, "
				   "photo BLOB NOT NULL, "
				   "sort_order INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	// Таблица "Сценарний день"
	q_creator.exec("CREATE TABLE scenary_days "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Время"
	q_creator.exec("CREATE TABLE times "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Персонажи"
	q_creator.exec("CREATE TABLE characters "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL, "
				   "real_name TEXT DEFAULT(NULL), "
				   "description TEXT DEFAULT(NULL) "
				   "); "
				   );

	// Таблица "Состояния персонажей"
	q_creator.exec("CREATE TABLE character_states "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Фотографии персонажей"
	q_creator.exec("CREATE TABLE characters_photo "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "fk_character_id INTEGER NOT NULL, "
				   "photo BLOB NOT NULL, "
				   "sort_order INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	// Таблица "Текст сценария"
	q_creator.exec("CREATE TABLE scenario "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "text TEXT NOT NULL, "
				   "is_draft INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	//
	// Создаём таблицу изменений сценария
	//
	q_creator.exec("CREATE TABLE scenario_changes "
				   "("
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "uuid TEXT NOT NULL, "
				   "datetime TEXT NOT NULL, "
				   "username TEXT NOT NULL, "
				   "undo_patch TEXT NOT NULL, " // отмена изменения
				   "redo_patch TEXT NOT NULL, " // повтор изменения (наложение для соавторов)
				   "is_draft INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	//
	// Таблица с данными сценария
	//
	q_creator.exec("CREATE TABLE scenario_data "
				   "("
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "data_name TEXT NOT NULL UNIQUE, "
				   "data_value TEXT DEFAULT(NULL) "
				   ")"
				   );

	//
	// Таблица "Разработка"
	//
	q_creator.exec("CREATE TABLE research "
				   "("
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "parent_id INTEGER DEFAULT(NULL), "
				   "type INTEGER NOT NULL DEFAULT(0), " // 0 - папка, 1 - текст
				   "name TEXT NOT NULL, "
				   "description TEXT DEFAULT(NULL), "
				   "url TEXT DEFAULT(NULL), "
				   "image BLOB DEFAULT(NULL), "
				   "sort_order INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	_database.commit();
}

void Database::createIndexes(QSqlDatabase& _database)
{
	Q_UNUSED(_database);
}

void Database::createEnums(QSqlDatabase& _database)
{
	QSqlQuery q_creator(_database);
	_database.transaction();

	// Служебная информация
	{
		q_creator.exec(
					QString("INSERT INTO system_variables VALUES ('application-version-on-create', '%1')")
					.arg(QApplication::applicationVersion())
					);
		q_creator.exec(
					QString("INSERT INTO system_variables VALUES ('%1', '%2')")
					.arg(::applicationVersionKey())
					.arg(QApplication::applicationVersion())
					);
	}

	// Справочник мест
	{
		q_creator.exec(
					QString("INSERT INTO places (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "INT"))
					);
		q_creator.exec(
					QString("INSERT INTO places (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "EXT"))
					);
	}

	// Справочник времени
	{
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "DAY"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "NIGHT"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "MORNING"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "EVENING"))
					);
	}

	// Справочник состояний персонажей
	{
		//: Voice over
		q_creator.exec(
					QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "V.O."))
					);
		//: Off-screen
		q_creator.exec(
					QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "O.S."))
					);
		//: Continued
		q_creator.exec(
					QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "CONT'D"))
					);
	}

	_database.commit();
}

void Database::updateDatabase(QSqlDatabase& _database)
{
	QSqlQuery q_checker(_database);

	//
	// Определим версию базы данных
	//
	q_checker.exec(
				QString("SELECT value as version FROM system_variables WHERE variable = '%1' ")
				.arg(::applicationVersionKey())
				);
	q_checker.next();
	QString databaseVersion = q_checker.record().value("version").toString();
	int versionMajor = databaseVersion.split(".").value(0, "0").toInt();
	int versionMinor = databaseVersion.split(".").value(1, "0").toInt();
	int versionBuild = databaseVersion.split(".").value(2, "1").split(" ").value(0, "1").toInt();

	//
	// Вызываются необходимые процедуры обновления БД в зависимости от её версии
	//
	// 0.X.X
	//
	if (versionMajor <= 0) {
		//
		// 0.0.X
		//
		if (versionMinor <= 0) {
			if (versionMinor < 0
				|| versionBuild <= 1) {
				updateDatabaseTo_0_0_2(_database);
			}
			if (versionMinor < 0
				|| versionBuild <= 4) {
				updateDatabaseTo_0_0_5(_database);
			}
		}
		//
		// 0.1.X
		//
		if (versionMinor <= 1) {
			if (versionMinor < 1
				|| versionBuild <= 0) {
				updateDatabaseTo_0_1_0(_database);
			}
		}
		//
		// 0.2.X
		//
		if (versionMinor <= 2) {
			if (versionMinor < 2
				|| versionBuild <= 7) {
				updateDatabaseTo_0_2_8(_database);
			}
		}
		//
		// 0.3.X
		//
		if (versionMinor <= 3) {
			if (versionMinor < 3
				|| versionBuild <= 2) {
				updateDatabaseTo_0_3_3(_database);
			}
		}
		//
		// 0.4.X
		//
		if (versionMinor <= 4) {
			if (versionMinor < 4
				|| versionBuild <= 4) {
				updateDatabaseTo_0_4_5(_database);
			}
			if (versionMinor < 4
				|| versionBuild <= 9) {
				updateDatabaseTo_0_5_0(_database);
			}
		}
		//
		// 0.5.x
		//
		if (versionMinor <= 5) {
			if (versionMinor < 5
				|| versionBuild <= 5) {
				updateDatabaseTo_0_5_6(_database);
			}
			if (versionMinor < 5
				|| versionBuild <= 7) {
				updateDatabaseTo_0_5_8(_database);
			}
			if (versionMinor < 5
				|| versionBuild <= 8) {
				updateDatabaseTo_0_5_9(_database);
			}
		}
	}

	//
	// Обновляется версия программы
	//
	q_checker.exec(
				QString("INSERT INTO system_variables VALUES ('%1', '%2')")
				.arg(::applicationVersionKey())
				.arg(QApplication::applicationVersion())
				);
}

void Database::updateDatabaseTo_0_0_2(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		q_updater.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "MORNING"))
					);
		q_updater.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QApplication::translate("DatabaseLayer::Database", "EVENING"))
					);
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_0_5(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Обновление таблицы локаций
		//
		q_updater.exec("ALTER TABLE locations ADD COLUMN description TEXT DEFAULT(NULL)");

		// Таблица "Фотографии локаций"
		q_updater.exec("CREATE TABLE locations_photo "
					   "( "
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "fk_location_id INTEGER NOT NULL, "
					   "photo BLOB NOT NULL, "
					   "sort_order INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );

		//
		// Обновление таблицы персонажей
		//
		q_updater.exec("ALTER TABLE characters ADD COLUMN real_name TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE characters ADD COLUMN description TEXT DEFAULT(NULL)");

		// Таблица "Фотографии персонажей"
		q_updater.exec("CREATE TABLE characters_photo "
					   "( "
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "fk_character_id INTEGER NOT NULL, "
					   "photo BLOB NOT NULL, "
					   "sort_order INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_1_0(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Обновление таблицы сценария
		//
		q_updater.exec("ALTER TABLE scenario ADD COLUMN name TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN synopsis TEXT DEFAULT(NULL)");
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_2_8(QSqlDatabase& _database)
{
	//
	// Заменить при помощи регулярок все
	//       font-family:'*';
	//		 font-size:*;
	//
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		const QRegularExpression rx_fontFamilyCleaner("font-family:([^;]*);");
		const QRegularExpression rx_fontSizeCleaner("font-size:([^;]*);");

		//
		// Персонажи
		//
		// ... очистим данные
		//
		q_updater.exec("SELECT id, description FROM characters");
		QMap<int, QString> charactersDescriptions;
		while (q_updater.next()) {
			const int id = q_updater.record().value("id").toInt();
			QString description = q_updater.record().value("description").toString();
			description = description.remove(rx_fontFamilyCleaner);
			description = description.remove(rx_fontSizeCleaner);
			charactersDescriptions.insert(id, description);
		}
		//
		// ... обновим данные
		//
		q_updater.prepare("UPDATE characters SET description = ? WHERE id = ?");
		foreach (int id, charactersDescriptions.keys()) {
			q_updater.addBindValue(charactersDescriptions.value(id));
			q_updater.addBindValue(id);
			q_updater.exec();
		}

		//
		// Локации
		//
		// ... очистим данные
		//
		q_updater.exec("SELECT id, description FROM locations");
		QMap<int, QString> locationsDescriptions;
		while (q_updater.next()) {
			const int id = q_updater.record().value("id").toInt();
			QString description = q_updater.record().value("description").toString();
			description = description.remove(rx_fontFamilyCleaner);
			description = description.remove(rx_fontSizeCleaner);
			locationsDescriptions.insert(id, description);
		}
		//
		// ... обновим данные
		//
		q_updater.prepare("UPDATE locations SET description = ? WHERE id = ?");
		foreach (int id, locationsDescriptions.keys()) {
			q_updater.addBindValue(locationsDescriptions.value(id));
			q_updater.addBindValue(id);
			q_updater.exec();
		}

		//
		// Синопсис сценария
		//
		// ... очистим данные
		//
		q_updater.exec("SELECT id, synopsis FROM scenario");
		QMap<int, QString> scenarioSynopsis;
		while (q_updater.next()) {
			const int id = q_updater.record().value("id").toInt();
			QString synopsis = q_updater.record().value("synopsis").toString();
			synopsis = synopsis.remove(rx_fontFamilyCleaner);
			synopsis = synopsis.remove(rx_fontSizeCleaner);
			scenarioSynopsis.insert(id, synopsis);
		}
		//
		// ... обновим данные
		//
		q_updater.prepare("UPDATE scenario SET synopsis = ? WHERE id = ?");
		foreach (int id, scenarioSynopsis.keys()) {
			q_updater.addBindValue(scenarioSynopsis.value(id));
			q_updater.addBindValue(id);
			q_updater.exec();
		}
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_3_3(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Создать таблицу состояний
		//
		q_updater.exec("CREATE TABLE character_states "
					   "( "
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "name TEXT UNIQUE NOT NULL "
					   "); "
					   );

		//
		// Добавление полей в таблицу сценария
		//
		q_updater.exec("ALTER TABLE scenario ADD COLUMN additional_info TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN genre TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN author TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN contacts TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN year TEXT DEFAULT(NULL)");
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_4_5(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Добавление поля в таблицу сценария
		//
		q_updater.exec("ALTER TABLE scenario ADD COLUMN is_draft INTEGER NOT NULL DEFAULT(0)");

		//
		// Добавление самого черновика
		//
		q_updater.exec("INSERT INTO scenario (id, text, is_draft) VALUES(null, '', 1)");
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_5_0(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Создаём таблицу для хранения всех запросов
		//
		q_updater.exec("CREATE TABLE _database_history "
					   "( "
					   "id TEXT PRIMARY KEY, " // uuid
					   "query TEXT NOT NULL, "
					   "query_values TEXT NOT NULL, "
					   "datetime TEXT NOT NULL "
					   "); "
					   );

		//
		// Создаём таблицу изменений сценария
		//
		q_updater.exec("CREATE TABLE scenario_changes "
					   "("
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "uuid TEXT NOT NULL, "
					   "datetime TEXT NOT NULL, "
					   "username TEXT NOT NULL, "
					   "undo_patch TEXT NOT NULL, " // отмена изменения
					   "redo_patch TEXT NOT NULL, " // повтор изменения (наложение для соавторов)
					   "is_draft INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );

		//
		// Таблица с данными сценария
		//
		q_updater.exec("CREATE TABLE scenario_data "
					   "("
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "data_name TEXT NOT NULL UNIQUE, "
					   "data_value TEXT DEFAULT(NULL) "
					   ")"
					   );

		//
		// Перенесём данные о сценарии в новую таблицу
		//
		q_updater.exec("SELECT name, additional_info, genre, author, contacts, year, synopsis "
					   "FROM scenario WHERE is_draft = 0");
		if (q_updater.next()) {
			QSqlQuery q_transporter(_database);
			q_transporter.prepare("INSERT INTO scenario_data (data_name, data_value) VALUES(?, ?)");

			const QString name = "name";
			q_transporter.addBindValue(name);
			q_transporter.addBindValue(q_updater.record().value(name));
			q_transporter.exec();

			const QString additionalInfo = "additional_info";
			q_transporter.addBindValue(additionalInfo);
			q_transporter.addBindValue(q_updater.record().value(additionalInfo));
			q_transporter.exec();

			const QString genre = "genre";
			q_transporter.addBindValue(genre);
			q_transporter.addBindValue(q_updater.record().value(genre));
			q_transporter.exec();

			const QString author = "author";
			q_transporter.addBindValue(author);
			q_transporter.addBindValue(q_updater.record().value(author));
			q_transporter.exec();

			const QString contacts = "contacts";
			q_transporter.addBindValue(contacts);
			q_transporter.addBindValue(q_updater.record().value(contacts));
			q_transporter.exec();

			const QString year = "year";
			q_transporter.addBindValue(year);
			q_transporter.addBindValue(q_updater.record().value(year));
			q_transporter.exec();

			const QString synopsis = "synopsis";
			q_transporter.addBindValue(synopsis);
			q_transporter.addBindValue(q_updater.record().value(synopsis));
			q_transporter.exec();
		}

		//
		// Сам сценарий
		//
		q_updater.exec("SELECT text FROM scenario WHERE is_draft = 0");
		if (q_updater.next()) {
			const QString defaultScenarioXml = BusinessLogic::ScenarioXml::defaultXml();
			const QString scenarioXml = q_updater.record().value("text").toString();
			const QString undoPatch = DiffMatchPatchHelper::makePatchXml(scenarioXml, defaultScenarioXml);
			const QString redoPatch = DiffMatchPatchHelper::makePatchXml(defaultScenarioXml, scenarioXml);
			q_updater.prepare("INSERT INTO scenario_changes (uuid, datetime, username, undo_patch, redo_patch) "
							  "VALUES(?, ?, ?, ?, ?)");
			q_updater.addBindValue(QUuid::createUuid().toString());
			q_updater.addBindValue(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss"));
			q_updater.addBindValue("user");
			q_updater.addBindValue(undoPatch);
			q_updater.addBindValue(redoPatch);
			q_updater.exec();
		}

	}

	_database.commit();
}

void Database::updateDatabaseTo_0_5_6(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Извлекаем текст сценария
		//
		q_updater.exec("SELECT id, text FROM scenario");
		QMap<int, QString> scenarioTexts;
		while (q_updater.next()) {
			const int id = q_updater.record().value("id").toInt();
			QString text = q_updater.record().value("text").toString();
			//
			// Заменяем старые теги на новые
			//
			text = text.replace("time_and_place", "scene_heading");
			scenarioTexts.insert(id, text);
		}
		//
		// ... обновим данные
		//
		q_updater.prepare("UPDATE scenario SET text = ? WHERE id = ?");
		foreach (int id, scenarioTexts.keys()) {
			q_updater.addBindValue(scenarioTexts.value(id));
			q_updater.addBindValue(id);
			q_updater.exec();
		}
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_5_8(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Добавляем таблицу "Разработка"
		//
		q_updater.exec("CREATE TABLE research "
					   "("
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "parent_id INTEGER DEFAULT(NULL), "
					   "type INTEGER NOT NULL DEFAULT(0), "
					   "name TEXT NOT NULL, "
					   "description TEXT DEFAULT(NULL), "
					   "url TEXT DEFAULT(NULL), "
					   "image BLOB DEFAULT(NULL), "
					   "sort_order INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_5_9(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Извлекаем текст сценария
		//
		q_updater.exec("SELECT id, text FROM scenario");
		QMap<int, QString> scenarioTexts;
		while (q_updater.next()) {
			const int id = q_updater.record().value("id").toInt();
			QString text = q_updater.record().value("text").toString();

			//
			// Вытаскиваем описание папок/групп/сцен и вставляем после заголовков или персонажей сцен
			//

			QStringList textLines = text.split("\n");
			QString lastDescription;
			const QString folderPrefix = "<folder_header description=\"";
			const QString sceneGroupPrefix = "<scene_group_header description=\"";
			const QString scenePrefix = "<scene_heading description=\"";
			const QString postfix = "\">";

			for (int lineNumber = 0; lineNumber < textLines.size(); ++lineNumber) {
				const QString& lineText = textLines.at(lineNumber);
				QString prefix;
				if (lineText.startsWith(scenePrefix)) {
					prefix = scenePrefix;
				} else if (lineText.startsWith(folderPrefix)) {
					prefix = folderPrefix;
				} else if (lineText.startsWith(sceneGroupPrefix)) {
					prefix = sceneGroupPrefix;
				}
				//
				// ... если в блоке есть описание, вытаскиваем его
				//
				if (!prefix.isEmpty()) {
					lastDescription = lineText.mid(prefix.length());
					lastDescription = lastDescription.left(lastDescription.length() - postfix.length());
					//
					// ... если для элемента задан цвет, нужно и его отрезать от описания
					//	   там получается вот такой текст [" color="#cc0000]
					//
					if (lastDescription.right(16).left(10) == "\" color=\"#") {
						lastDescription = lastDescription.left(lastDescription.length() - 16);
					}
				}
				//
				// ... проверяем не пора ли вставить описание
				//
				else if (!lastDescription.isEmpty()) {
					bool needInsert = false;
					//
					// ... после сцены
					//
					if (lineText == "</scene_heading>") {
						if (textLines.at(lineNumber + 1) == "<scene_characters>") {
							while (textLines.at(++lineNumber) != "</scene_characters>");
						}
						needInsert = true;
					}
					//
					// ... после папки или группы
					//
					else if (lineText == "</folder_header>"
							 || lineText == "</scene_group_header>") {
						needInsert = true;
					}

					//
					// ... вставляем, если нужно
					//
					if (needInsert) {
						//
						// ... убираем спецсимволы и тэги, пока не дойдём до содержимого
						//
						QTextDocument descriptionDoc;
						descriptionDoc.setHtml(lastDescription);
						descriptionDoc.setHtml(descriptionDoc.toPlainText());
						descriptionDoc.setHtml(descriptionDoc.toPlainText());
						//
						QStringList descriptionLines = descriptionDoc.toPlainText().split("\n", QString::SkipEmptyParts);

						foreach (const QString& descriptionLine, descriptionLines) {
							textLines.insert(++lineNumber, "<scene_description>");
							textLines.insert(++lineNumber, QString("<v><![CDATA[%1]]></v>").arg(descriptionLine));
							textLines.insert(++lineNumber, "<reviews/>");
							textLines.insert(++lineNumber, "</scene_description>");
						}

						//
						// ... очищаем, чтобы не вставлять его же в следующие сцены
						//
						lastDescription.clear();
					}
				}
			}
			text = textLines.join("\n");
			scenarioTexts.insert(id, text);
		}
		//
		// ... обновим данные
		//
		q_updater.prepare("UPDATE scenario SET text = ? WHERE id = ?");
		foreach (int id, scenarioTexts.keys()) {
			q_updater.addBindValue(scenarioTexts.value(id));
			q_updater.addBindValue(id);
			q_updater.exec();
		}
	}

	_database.commit();
}
