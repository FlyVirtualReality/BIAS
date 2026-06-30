#ifndef BIAS_BASIC_HTTP_SERVER_HPP
#define BIAS_BASIC_HTTP_SERVER_HPP

#include <QTcpServer>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariantMap>

class QTcpSocket;

namespace bias
{


    class BasicHttpServer : public QTcpServer
    {

        Q_OBJECT

        public:
            BasicHttpServer(QObject *parent=0);
            virtual void incomingConnection(qintptr socket);

        protected:
            // Return value: true if the connection should be kept open (HTTP keep-alive),
            // false if it should be closed after the response is sent.
            virtual bool handleGetRequest(QTcpSocket *socket, QStringList &tokens);
            virtual bool handleParamsRequest(QTcpSocket *socket, QStringList &paramsList);
            virtual void sendBadRequestResp(QTextStream &os, QString msg);
            virtual void sendRunningResp(QTextStream &os);
            virtual QVariantMap paramsRequestSwitchYard(QString name, QString value);

        protected slots:
            virtual void readClient();
            virtual void discardClient();
    };

    QStringList splitRequestString(QString reqString);
    QString replaceEscapeChars(QString input);


} // namespace bias


#endif // #ifndef BIAS_BASIC_HTTP_SERVER_HPP
