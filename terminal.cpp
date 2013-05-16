#include "terminal.h"
#include <QEvent>
#include <QKeyEvent>
#include <QDebug>

Terminal::Terminal(QObject *parent) :
    QObject(parent)
{
    m_timer.setInterval(50);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &Terminal::updateInput);
}

QSerialPort *Terminal::port() const
{
    return m_port;
}

void Terminal::setPort(QSerialPort *arg)
{
    if (m_port == arg) return;

    if (m_port->isOpen()) m_port->close();
    m_port = arg;

    emit portChanged(arg);
}

bool Terminal::active() const
{
    return m_active;
}

void Terminal::setActive(bool arg)
{
    if (m_active == arg) return;
    m_active = arg;
    emit activeChanged(arg);
}

QString Terminal::text() const
{
    return m_text;
}

void Terminal::setText(QString arg)
{
    if (m_text == arg) return;
    m_text = arg;
    emit textChanged(arg);
}

void Terminal::sendText(QString text)
{
    if (!m_port) return;

    if (!m_port->isOpen()) {
        qWarning() << "Terminal: Active Port is not open.";
        return;
    }

    m_port->write(text.toLocal8Bit());
    m_port->write("\n");
}

Settings *Terminal::settings() const
{
    return m_settings;
}

void Terminal::updateInput()
{
    if (!m_port->isOpen()) return;

    if (m_active && m_port->bytesAvailable() > 0) {
        // TODO: Do all the fancy hex, dec, stuff.
        m_text.append(m_port->readAll());
    }
}


void Terminal::setsettings(Settings *arg)
{
    if (m_settings == arg) return;
    m_settings = arg;
    emit settingsChanged(arg);
}


