/****************************************************************************
** Copyright (c) 2019 Evgeny Teterin (nayk) <sutcedortal@gmail.com>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#include <QSettings>

#include "filesys.h"
#include "db_client.h"

namespace nayk {
//=======================================================================================================
DBClient::DBClient(QObject *parent, const QString &settingsFileName, const QString &sqlDriver) : QObject( parent )
{
    if(!settingsFileName.isNull() && !settingsFileName.isEmpty()) _settingsFileName = settingsFileName;
    if(!sqlDriver.isNull() && !sqlDriver.isEmpty()) _sqlDriver = sqlDriver;
    readSettings();
}
//=======================================================================================================
DBClient::~DBClient()
{
    close();
}
//=======================================================================================================
void DBClient::setSettings(const QString &server, quint16 port, const QString &userName, const QString &password, const QString &baseName)
{
    _server = server;
    _port = port;
    _userName = userName;
    _password = password;
    _baseName = baseName;
    if(isOpen()) open(true);
}
//=======================================================================================================
void DBClient::setSettingsFileName(const QString &settingsFileName)
{
    if(settingsFileName != _settingsFileName) {
        _settingsFileName = settingsFileName;
        if(readSettings() && isOpen()) open(true);
    }
}
//=======================================================================================================
void DBClient::setSqlDriver(const QString &sqlDriver)
{
    if(_sqlDriver != sqlDriver) {
        bool op = isOpen();
        _sqlDriver = sqlDriver;
        if(op) open(true);
    }
}
//=======================================================================================================
bool DBClient::readSettings(const QString &settingsFileName)
{
    QString fileName = (settingsFileName.isNull() || settingsFileName.isEmpty()) ? _settingsFileName : settingsFileName;
    if(!FileSys::fileExists(fileName)) return false;
    emit toLog(LogDbg, tr("Чтение настроек соединяния с БД из файла %1").arg(fileName));

    QSettings ini(fileName, QSettings::IniFormat);
    ini.setIniCodec("UTF-8");
    _server = ini.value("database/server", _server).toString();
    _port = static_cast<quint16>(ini.value("database/port", _port).toInt());
    _userName = ini.value("database/user", _userName).toString();
    _password = ini.value("database/password", _password).toString();
    _baseName = ini.value("database/base", _baseName).toString();

    return true;
}
//=======================================================================================================
bool DBClient::writeSettings(const QString &settingsFileName)
{
    QString fileName = (settingsFileName.isNull() || settingsFileName.isEmpty()) ? _settingsFileName : settingsFileName;
    emit toLog(LogDbg, tr("Запись настроек соединяния с БД в файл %1").arg(fileName));

    QSettings ini(fileName, QSettings::IniFormat);
    ini.setIniCodec("UTF-8");
    ini.setValue("database/server", _server);
    ini.setValue("database/port", _port);
    ini.setValue("database/user", _userName);
    ini.setValue("database/password", _password);
    ini.setValue("database/base", _baseName);
    ini.sync();
    bool ok = (ini.status() == QSettings::NoError);
    if(!ok) emit toLog(LogError, tr("Ошибка при сохранении настроек соединяния с БД"));
    return ok;
}
//=======================================================================================================
bool DBClient::open(bool reconnect)
{
    if(reconnect) close();
    if(isOpen()) return !reconnect;

    _connectionName = "connect_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QSqlDatabase db = QSqlDatabase::addDatabase(_sqlDriver, _connectionName);

    if(!db.isValid()) {
        _lastError = tr("Не удалось создать подключение к БД");
        emit toLog(LogError, _lastError);
        return false;
    }

    db.setHostName(_server);
    db.setDatabaseName(_baseName);
    db.setUserName(_userName);
    db.setPassword(_password);
    db.setPort(_port);

    if (!db.open()) {
        _lastError = db.lastError().text();
        emit toLog(LogError, tr("Ошибка при подключении к СУБД: %1").arg( _lastError ));
        return false;
    }

    emit toLog(LogInfo, tr("Установлено подключение к СУБД"));
    emit toLog(LogDbg, tr("host=%1; port=%2; base=%3; user=%4")
               .arg(db.hostName())
               .arg(db.port())
               .arg(db.databaseName())
               .arg(db.userName())
               );
    emit dbOpen();
    return true;
}
//=======================================================================================================
bool DBClient::close()
{
    if(!isOpen()) return true;
    db().close();
    emit toLog(LogInfo, tr("Завершение соединения с СУБД"));
    emit dbClose();
    return true;
}
//=======================================================================================================
bool DBClient::isOpen(bool reconnect)
{
    return QSqlDatabase::database(_connectionName, reconnect).isOpen();
}
//=======================================================================================================
bool DBClient::execSQL(const QString &sqlText)
{
    QSqlQuery query(db());
    return execSQL(sqlText, &query, true);
}
//=======================================================================================================
bool DBClient::execSQL(const QString &sqlText, QSqlQuery *query, bool withTransaction)
{
    if(!isOpen(true)) {
        _lastError = tr("Ошибка выполнения SQL запроса: Нет соединения с БД");
        emit toLog(LogError, _lastError);
        return false;
    }

    if(!query) {
        _lastError = tr("Ошибка выполнения SQL запроса: QSqlQuery не задан");
        emit toLog(LogError, _lastError);
        return false;
    }

    QSqlDatabase dataBase = db();
    if(withTransaction) dataBase.transaction();

    if(!query->exec(sqlText)) {
        _lastError = tr("Ошибка выполнения SQL запроса: %1").arg(query->lastError().text());
        if(withTransaction) dataBase.rollback();
        emit toLog(LogDbg, "SQL: " + sqlText);
        emit toLog(LogError, _lastError);
        return false;
    }
    if(withTransaction) dataBase.commit();
    return true;
}
//=======================================================================================================
bool DBClient::getTables(QStringList *list)
{
    if(!list) {
        _lastError = tr("Неверный указатель на переменную списка.");
        emit toLog(LogError, _lastError);
        return false;
    }

    if(!isOpen()) {
        _lastError = tr("Ошибка выполнения SQL запроса: Нет соединения с БД");
        emit toLog(LogError, _lastError);
        return false;
    }

    list->clear();
    list->append( db().tables(QSql::Tables) );
    return true;
}
//=======================================================================================================
bool DBClient::getTableFields(const QString &tableName, QStringList *list)
{
    if(!list) {
        _lastError = tr("Неверный указатель на переменную списка");
        emit toLog(LogError, _lastError);
        return false;
    }

    if(!isOpen(true)) {
        _lastError = tr("Ошибка выполнения SQL запроса: Нет соединения с БД");
        emit toLog(LogError, _lastError);
        return false;
    }

    if(!tableExist(tableName)) {
        _lastError = tr("Ошибка выполнения SQL запроса: Таблица не найдена");
        emit toLog(LogError, _lastError);
        return false;
    }

    QString sql = "select * from \"" + tableName + "\" limit 1;";
    QSqlQuery query(db());

    if(!execSQL(sql, &query)) return false;

    QSqlRecord rec = query.record();

    list->clear();

    for(auto i=0; i<rec.count(); i++) {
        list->append(rec.field(i).name());
    }

    return true;
}
//=======================================================================================================
} // namespace nayk

