#ifndef TERMINAL_H
#define TERMINAL_H

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include "settings.h"

class Terminal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSerialPort *port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(Settings *settings READ settings WRITE setsettings NOTIFY settingsChanged)
public:
    explicit Terminal(QObject *parent = 0);
    
    QSerialPort * port() const;
    void setPort(QSerialPort * arg);

    bool active() const;
    void setActive(bool arg);

    QString text() const;
    void setText(QString arg);

    Q_INVOKABLE void sendText(QString text);

    Settings *settings() const;
    void setsettings(Settings *arg);

signals:
    void portChanged(QSerialPort * arg);
    void activeChanged(bool arg);
    void textChanged();
    void settingsChanged(Settings * arg);

public slots:
    void updateInput();
    void changePort();
    void updatePort();

private:
    QTimer m_timer;
    
    QSerialPort *m_port;
    bool m_active;
    QString m_text;
    Settings *m_settings;
};

#endif // TERMINAL_H
