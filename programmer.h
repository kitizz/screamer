#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QThread>
#include "settings.h"

class Worker;
class Programmer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProgramming READ isProgramming WRITE setIsProgramming NOTIFY isProgrammingChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int resends READ resends NOTIFY resendsChanged)
    Q_PROPERTY(int currentAddress READ currentAddress NOTIFY currentAddressChanged)
    Q_PROPERTY(int lastAddress READ lastAddress NOTIFY lastAddressChanged)

//    Q_PROPERTY(QSerialPort *port READ port WRITE setport NOTIFY portChanged)

    Q_ENUMS(Status)
//    Q_DECLARE_METATYPE(Status)

public:
    enum Status { Idle, Connecting, Connected, Programming, Failure, Error };

    explicit Programmer(QObject *parent = 0);

    Q_INVOKABLE void stopProgramming();
//    Q_INVOKABLE void resetMicro(Settings *settings);

//    bool startProgramMode(QSerialPort *port, Settings *settings);
//    bool sendProgram(QSerialPort *port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings);
//    bool loadHexFile(QUrl fileUrl, QByteArray *data, int *startAddress, int *endAddress, Settings *settings);
    
    bool isProgramming() const;
    void setIsProgramming(bool arg);

    qreal progress() const;
    void setProgress(qreal arg);

    Status status() const;
    void setStatus(Status arg);

    QString statusText() const;
    void setStatusText(QString arg);

    int resends() const;
    void setResends(int arg);
    void resendsIncrement();

    int currentAddress() const;
    void setCurrentAddress(int arg);

    QSerialPort *port() const;
    void setport(QSerialPort *arg);

    int lastAddress() const;
    void setLastAddress(int arg);

signals:
    void programMicro(Settings *settings);

    void isProgrammingChanged(bool arg);
    void progressChanged(qreal arg);
    void statusChanged(Status arg);
    void statusTextChanged();

    void resendsChanged(int arg);

    void currentAddressChanged(int arg);

    void portChanged(QSerialPort *arg);

    void lastAddressChanged(int arg);


public slots:

private:
    bool m_isProgramming;
    
    qreal m_progress;
    Status m_status;
    QString m_statusText;

    int m_resends;
    int m_currentAddress;
    int m_lastAddress;

    Worker *m_worker;
    QThread m_workerThread;
};

class Worker: public QObject
{
    Q_OBJECT

public:
    explicit Worker(QObject *parent=0);
    explicit Worker(Programmer *prog, QObject *parent=0);

    void setStatus(Programmer::Status status, QString statusText=QString());
    void setProgress(int current, int total, qreal progress);

    bool openPort(QSerialPort *port);
public slots:
    void kayGo(Settings *settings);
    void programMicro(Settings *settings, QSerialPort *port);
    void resetMicro(Settings *settings);

    bool startProgramMode(QSerialPort *port, Settings *settings);
    bool sendProgram(QSerialPort *port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings);
    bool loadHexFile(QUrl fileUrl, QByteArray *data, int *startAddress, int *endAddress, Settings *settings);
    void stopProgramming();

private:
    Programmer *m_programmer;
    bool m_running;
    bool m_stopProgramming;

    QByteArray m_fileBuffer;
    int m_startAddress;
    int m_endAddress;

};

#endif // PROGRAMMER_H
