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
#ifndef NAYK_HTTP_SERVER_H
#define NAYK_HTTP_SERVER_H

#include <QObject>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>
#include <QMap>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>

#include "http.h"
#include "log.h"

namespace nayk {
//======================================================================================================


//======================================================================================================
class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = nullptr);
    virtual ~HttpServer();
    //
    QString lastError() const { return _lastError; }
    QString requestContentType() const { return _requestContentType; }
    QString requestCharset() const { return _requestCharset; }
    bool readRequestOK() const { return _readRequestOK; }
    bool readRequestError() const { return !_readRequestOK; }
    bool writeResponseOK() const { return _writeResponseOK; }
    bool writeResponseError() const { return !_writeResponseOK; }
    void clearResponseHeaders() { mResponseHeaders.clear(); }
    void clearResponseCookies() { mResponseCookies.clear(); }
    void addResponseHeader(const QString &name, const QString &value) { mResponseHeaders.insert(name, value); }
    void addResponseCookie(const QString &name, const QString &value) { mResponseCookies.insert(name, value); }
    void setResponseContentType(const QString &value) { mResponseHeaders.insert(HeaderContentType, value); }
    void setResponseContent(const QByteArray &value) { _responseContent = value; }
    QVariant requestParameter(const QString &name) const;
    QString requestPostParameter(const QString &name) const;
    QString requestGetParameter(const QString &name) const;
    QString requestHeader(const QString &name) const;
    QString requestCookie(const QString &name) const;
    QByteArray requestBinParameter(const QString &name) const;
    QJsonValue requestJsonParameter(const QString &name) const;
    QByteArray requestContent() { return _requestContent; }
    QString responseHeader(const QString &name) const;
    QString responseCookie(const QString &name) const;
    QByteArray responseContent() const { return _responseContent; }
    QMap<QString, QString> requestCookies() const { return mRequestCookies; }
    QMap<QString, QString> requestGetParameters() const { return mRequestGetParameters; }
    QMap<QString, QString> requestPostParameters() const { return mRequestPostParameters; }
    QMap<QString, QString> requestHeaders() const { return mRequestHeaders; }
    QMap<QString, QByteArray> requestBinParameters() const { return mRequestBinParameters; }
    void setDbgLogging(bool on = true) { _dbg = on; }

signals:
    void toLog(LogType, QString);
    void readRequestFinished(bool);
    void writeResponseFinished(bool);

public slots:
    void readRequest(bool *ok = nullptr);
    void writeResponse(bool *ok = nullptr);

private:
    bool _dbg {false};
    QString _requestContentType;
    QString _requestCharset;
    bool _readRequestOK {true};
    bool _writeResponseOK {true};
    QMap<QString, QString> mRequestCookies;
    QMap<QString, QString> mRequestGetParameters;
    QMap<QString, QString> mRequestPostParameters;
    QMap<QString, QString> mRequestHeaders;
    QMap<QString, QByteArray> mRequestBinParameters;
    QMap<QString, QJsonValue> mRequestJsonParameters;
    QByteArray _requestContent;
    QMap<QString, QString> mResponseHeaders;
    QMap<QString, QString> mResponseCookies;
    QByteArray _responseContent;
    //
    bool readRequestContent(QByteArray &buf);
    QMap<QString, QString> decodeQuery(const QString &strQuery, const QString &strPairSeparator = "&");
    QString encodeQuery(QMap<QString, QString> qmQuery, const QString &strPairSeparator = "&");
    QString encodeQuery(QVariantMap qvmQuery, const QString &strPairSeparator = "&");
    void processCookies();
    void processHeaders();
    void processGet();
    bool processPost();
    bool processReadRequest();
    bool processWriteResponse();
    bool writeResponseContent();

private slots:
    void startReadRequest();
    void startWriteResponse();

protected:
    QString _lastError {""};

};
//======================================================================================================
} // namespace nayk
#endif // NAYK_HTTP_SERVER_H
