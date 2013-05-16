#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include "settings.h"


class Programmer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProgramming READ isProgramming NOTIFY isProgrammingChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int resends READ resends NOTIFY resendsChanged)
    Q_PROPERTY(int currentAddress READ currentAddress NOTIFY currentAddressChanged)

    Q_ENUMS(Status)

public:
    enum Status { Idle, Connecting, Connected, Programming, Failure, Error };
    explicit Programmer(QObject *parent = 0);

    Q_INVOKABLE void programMicro(QSerialPort *port, Settings *settings);

    bool startProgramMode(QSerialPort *port, Settings *settings);
    bool sendProgram(QSerialPort *port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings);
    bool loadHexFile(QUrl fileUrl, QByteArray *data, int *startAddress, int *endAddress, Settings *settings);
    
    bool isProgramming() const;

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

signals:
    void isProgrammingChanged(bool arg);
    void progressChanged(qreal arg);
    void statusChanged(Status arg);
    void statusTextChanged(QString arg);

    void resendsChanged(int arg);

    void currentAddressChanged(int arg);

public slots:
    void stopProgramming();

private:
    void setIsProgramming(bool arg);
    bool m_isProgramming;
    bool m_stopProgramming;
    QByteArray m_fileBuffer;
    int m_startAddress;
    int m_endAddress;
    
    qreal m_progress;
    Status m_status;
    QString m_statusText;

    int m_resends;
    int m_currentAddress;
};

#endif // PROGRAMMER_H
