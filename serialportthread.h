#ifndef SERIALPORTTHREAD_H
#define SERIALPORTTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class SerialPortThread : public QThread
{
	Q_OBJECT

public:
	SerialPortThread(QObject *parent = 0);
	~SerialPortThread();

	void transaction(const QString &portName, const QString &request);
	void run();

signals:
    void S_PortNotOpen(bool a);
	void response(const QString &s);
    void timeout(bool a);
    void last_return();

private:
	QString portName;
	QString request;
	int waitTimeout;
	QMutex mutex;
	QWaitCondition cond;
	bool quit;
};

#endif // SERIALPORTTHREAD_H
