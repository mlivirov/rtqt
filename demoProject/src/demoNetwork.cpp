#include "demoNetwork.h"

DemoTcpServer::DemoTcpServer(int port, QObject *parent): QTcpServer(parent)
{
	bool res = listen(QHostAddress::Any, port);
	res = res;
}

DemoTcpServer::~DemoTcpServer()
{

}

void DemoTcpServer::incomingConnection(int socket)
{
	Proceeder *p = new Proceeder(socket,this);
	proceeders.append(p);
	emit newConnection();
}

QByteArray DemoTcpServer::question(QByteArray &q)
{
	QByteArray b;
	b.append("ECHO>>");
	b.append(q);
	return b;
}


DemoTcpServer::Proceeder::Proceeder(int sock, QObject *parent):QThread(parent), sockD(sock)
{
	start();
}

DemoTcpServer::Proceeder::~Proceeder()
{
	sockD = 0;
	wait();
}

void DemoTcpServer::Proceeder::run()
{
	socket = new QTcpSocket();
	socket->setSocketDescriptor(sockD);
	sockD = 1;
	bool v = socket->isOpen();
	QHostAddress addr = socket->peerAddress();
	while(sockD)
	{
		if(socket->waitForReadyRead())
		{
			QByteArray q = socket->readAll();
			QByteArray a = ((DemoTcpServer*)parent())->question(q);
			socket->write(a);
		}
		else if(~socket->state() & QTcpSocket::ConnectedState)
			break;
	}

	delete socket;
}


DemoUdpServer::DemoUdpServer(int port, QObject *parent): QUdpSocket(parent)
{
	bind(QHostAddress::Any, port);

	connect(this, SIGNAL(readyRead()), SLOT(newDataReceived()));
}

DemoUdpServer::~DemoUdpServer()
{

}

void DemoUdpServer::newDataReceived()
{
	QHostAddress addr;
	quint16 port;
	QByteArray q(pendingDatagramSize(), 0);
	readDatagram(q.data(), q.size(), &addr, &port);
	this->writeDatagram(question(q), addr, port);
}

QByteArray DemoUdpServer::question(QByteArray &q)
{
	QByteArray b;
	b.append("ECHO>>");
	b.append(q);
	return b;
}


#include "meta/moc_demoNetwork.cpp"