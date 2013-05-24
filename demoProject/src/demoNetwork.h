#ifndef DEMONETWORK_H
#define DEMONETWORK_H


#include "headers.h"

#define TCPSERVER_PORT 8080
#define UDPSERVER_PORT 7081

class DemoTcpServer: public QTcpServer
{
	Q_OBJECT

	class Proceeder : public QThread
	{
		friend class MyTCPServer;		
		int sockD;
	public:
		Proceeder(int sockD, QObject *parent);
		~Proceeder();
	protected:
		QTcpSocket *socket;
		virtual void run();
	};
	QList<Proceeder *> proceeders;
public:
	DemoTcpServer(int port = TCPSERVER_PORT,QObject *parent = 0);
	~DemoTcpServer();
protected:
	virtual QByteArray question(QByteArray &q);
	virtual void incomingConnection(int socket);
};

class DemoUdpServer : public QUdpSocket
{
	Q_OBJECT
public:
	DemoUdpServer(int port = UDPSERVER_PORT, QObject *parent = 0);
	~DemoUdpServer();
protected Q_SLOTS:
	virtual QByteArray question(QByteArray &q);
	void newDataReceived();
};

#endif