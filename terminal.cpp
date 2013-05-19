#include "terminal.h"
#include <QEvent>
#include <QThread>
#include <QKeyEvent>
#include <QDebug>
#include "util.h"

Terminal::Terminal(QObject *parent) :
    QObject(parent),
    m_port(0),
    m_active(false),
    m_settings(0)
{
    m_timer.setInterval(50);
    m_timer.setSingleShot(false);
    m_timer.start();
    connect(&m_timer, &QTimer::timeout, this, &Terminal::updateInput);

}

QSerialPort *Terminal::port() const
{
    return m_port;
}

void Terminal::setPort(QSerialPort *arg)
{
    if (m_port == arg) return;

    if (m_port && m_port->isOpen()) m_port->close();
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

    qDebug() << "Setting Active" << arg;
    if (m_port) {
        if (m_port->isOpen())
            m_port->close();
        if (arg) {
            updatePort();
            if (!openPort()) return;
            m_port->setRequestToSend(true);
        }
    }

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
    emit textChanged();
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
    if (!m_port || !m_port->isOpen() || !m_active) return;

    if (m_port->bytesAvailable() > 0) {
        // TODO: Do all the fancy hex, dec, stuff.
        m_text.append(m_port->readAll());

        if (m_text.length() > 2000)
            m_text.remove(0, m_text.length()-2000);
        emit textChanged();
    }
}

void Terminal::changePort()
{
    if (!m_settings) return;
    qDebug() << "Terminal Change Port:" << m_settings->portName();
    if (m_port && m_port->portName() == m_settings->portName())
        return;

    bool wasOpen = false;
    if (m_port) {
        if (m_port->isOpen()) {
            wasOpen = true;
            m_port->close();
        }
    }

    if (m_port)
        m_port->setPort(Util::findPort(m_settings->portName()));
    else
        m_port = new QSerialPort(m_settings->portName());

    if (m_port->error() != QSerialPort::NoError) {
        qDebug() << "Error Changing Port:" << m_port->error();
    } else {
        updatePort();
        if (wasOpen) openPort();
    }
}

void Terminal::updatePort()
{
    if (!m_active || !m_port) return;

    if (m_port->isOpen()) m_port->close();
    while (m_port->isOpen()) QThread::msleep(10);

    m_port->setBaudRate(m_settings->baudTerminal());
    m_port->setDataBits(m_settings->dataBits());
    m_port->setStopBits(m_settings->stopBits());
    m_port->setParity(m_settings->parity());
    m_port->setRequestToSend(true);
}

bool Terminal::openPort()
{
    if (m_port->isOpen()) {
        qDebug() << "Port already open...";
        return true;
    }
    m_port->open(QIODevice::ReadWrite);

    if (m_port->error() != QSerialPort::NoError) {
        qDebug() << "Error Opening Port Terminal:" << m_port->error();
        return false;
    }
    return true;
}

void Terminal::setsettings(Settings *arg)
{
    if (m_settings == arg) return;

    if (m_settings) {
        disconnect(m_settings, &Settings::portNameChanged, this, &Terminal::changePort);

        disconnect(m_settings, &Settings::baudTerminalChanged, this, &Terminal::updatePort);
        disconnect(m_settings, &Settings::dataBitsChanged, this, &Terminal::updatePort);
        disconnect(m_settings, &Settings::stopBitsChanged, this, &Terminal::updatePort);
        disconnect(m_settings, &Settings::parityChanged, this, &Terminal::updatePort);
    }

    m_settings = arg;

    if (m_settings) {
        connect(m_settings, &Settings::portNameChanged, this, &Terminal::changePort);

        connect(m_settings, &Settings::baudTerminalChanged, this, &Terminal::updatePort);
        connect(m_settings, &Settings::dataBitsChanged, this, &Terminal::updatePort);
        connect(m_settings, &Settings::stopBitsChanged, this, &Terminal::updatePort);
        connect(m_settings, &Settings::parityChanged, this, &Terminal::updatePort);
    }

    changePort();

    emit settingsChanged(arg);
}


