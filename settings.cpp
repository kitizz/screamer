#include "settings.h"

#include <QVariant>
#include "util.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

Settings::Settings(QObject *parent) :
    QObject(parent),
    m_saving(false),
    m_settingsFile("settings.txt"),
    m_log(QString()),
    m_programmerActive(false),
    m_terminalActive(false)
{
    m_port = new QSerialPort();
    updatePorts();

    if(!load()) {
        m_portName = QString();
        m_baudProgram = QSerialPort::Baud9600;
        m_frequency = 8000000;
        m_chip = Atmega328;
        m_baudTerminal = QSerialPort::Baud9600;
        m_dataBits = QSerialPort::Data8;
        m_parity = QSerialPort::NoParity;
        m_stopBits = QSerialPort::OneStop;
        m_terminalCharacters = Settings::Ascii;
        m_echo = false;
        m_autoOpenTerminal = true;
        m_logDownload = true;
        m_wrapTerminal = true;
        m_hexFiles = QStringList();
        m_hexFile = QUrl();
        m_resetType = Settings::RTS;

        updatePort();

        save();
    }

    // Set up timer to poll for available ports
    m_timer.setInterval(2000);
    m_timer.setSingleShot(false);
    m_timer.start();
    connect(&m_timer, &QTimer::timeout, this, &Settings::updatePorts);

    connect(this, &Settings::programmerActiveChanged, this, &Settings::updatePort);

    connect(this, &Settings::portNameChanged, this, &Settings::changed);
    connect(this, &Settings::baudProgramChanged, this, &Settings::changed);
    connect(this, &Settings::frequencyChanged, this, &Settings::changed);
    connect(this, &Settings::chipChanged, this, &Settings::changed);

    connect(this, &Settings::baudTerminalChanged, this, &Settings::changed);
    connect(this, &Settings::dataBitsChanged, this, &Settings::changed);
    connect(this, &Settings::parityChanged, this, &Settings::changed);
    connect(this, &Settings::stopBitsChanged, this, &Settings::changed);
    connect(this, &Settings::resetTypeChanged, this, &Settings::changed);
    connect(this, &Settings::terminalCharactersChanged, this, &Settings::changed);
    connect(this, &Settings::echoChanged, this, &Settings::changed);
    connect(this, &Settings::autoOpenTerminalChanged, this, &Settings::changed);
    connect(this, &Settings::logDownloadChanged, this, &Settings::changed);
    connect(this, &Settings::wrapTerminalChanged, this, &Settings::changed);

    connect(this, &Settings::hexFileChanged, this, &Settings::changed);
    connect(this, &Settings::hexFilesChanged, this, &Settings::changed);


    connect(this, &Settings::changed, this, &Settings::save);
}

bool Settings::load()
{
    QFile file(settingsFile().path());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to access settings file:" << settingsFile();
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Settings File in unexpected format. Unable to load settings.";
        file.close();
        return false;
    }

    QVariantMap map = doc.object().toVariantMap();

    // Update the settings:
    Util::qvariant2qobject(map, this);
    return true;
}

void Settings::save()
{
    if (m_saving) return;
    m_saving = true;

    QStringList ignore;
    ignore << "objectName" << "selectedPort"
           << "programmerActive" << "terminalActive";
    QVariantMap map = Util::qobject2qvariant(this, ignore);

    QJsonObject json = QJsonObject::fromVariantMap(map);
    QJsonDocument doc = QJsonDocument(json);

    QFile file(settingsFile().path());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to Save to file:" << settingsFile();
        return;
    }

    QTextStream out(&file);
    out << doc.toJson();
    out.flush();

    file.close();

    m_saving = false;
}

void Settings::updatePorts()
{
    QStringList portNames;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        portNames << info.portName();
    }

    if (portNames == m_availablePorts) return;

    m_availablePorts = portNames;
    emit availablePortsChanged(m_availablePorts);
}

void Settings::writeLog(QString log)
{
    m_log.append(log + " ");
    emit logChanged();
}

void Settings::writeLogLn(QString log)
{
    m_log.append(log + "\n");
    emit logChanged();
}

void Settings::setSettingsFile(QUrl arg)
{
    if (m_settingsFile == arg) return;
    m_settingsFile = arg;
    emit settingsFileChanged(arg);
}

QString Settings::portName() const
{
    return m_portName;
}


QUrl Settings::settingsFile() const
{
    return m_settingsFile;
}


void Settings::setPortName(QString arg)
{
    if (m_portName == arg) return;
    m_portName = arg;
    if (m_port) {
        bool wasOpen = m_port->isOpen();
        if (wasOpen) m_port->close();
        while(m_port->isOpen()) QThread::msleep(50);

        m_port->setPortName(arg);
        updatePort();

        if (wasOpen) m_port->open(QIODevice::ReadWrite);
    }
    emit portNameChanged(arg);
}

QSerialPort::BaudRate Settings::baudProgram() const
{
    return m_baudProgram;
}


void Settings::setBaudProgram(QSerialPort::BaudRate arg)
{
    if (m_baudProgram == arg) return;
    m_baudProgram = arg;

    if (m_port && m_port->isOpen() && m_programmerActive) {
        if (!m_port->setBaudRate(arg))
            qDebug() << "Settings: Error Setting Baud rate to programmer's." << m_port->errorString();
    }

    emit baudProgramChanged(arg);
}

int Settings::frequency() const
{
    return m_frequency;
}


void Settings::setFrequency(int arg)
{
    if (m_frequency == arg) return;
    m_frequency = arg;
    emit frequencyChanged(arg);
}

Settings::Chip Settings::chip() const
{
    return m_chip;
}


void Settings::setChip(Chip arg)
{
    if (m_chip == arg) return;
    m_chip = arg;
    emit chipChanged(arg);
}

QSerialPort::BaudRate Settings::baudTerminal() const
{
    return m_baudTerminal;
}


void Settings::setBaudTerminal(QSerialPort::BaudRate arg)
{
    if (m_baudTerminal == arg) return;
    m_baudTerminal = arg;

    if (m_port && m_port->isOpen() && m_terminalActive && !m_programmerActive)
        m_port->setBaudRate(arg);

    emit baudTerminalChanged(arg);
}

QSerialPort::DataBits Settings::dataBits() const
{
    return m_dataBits;
}


void Settings::setDataBits(QSerialPort::DataBits arg)
{
    if (m_dataBits == arg) return;

    m_dataBits = arg;
    if (m_port && m_port->isOpen()) {
        if (!m_port->setDataBits(arg))
            qDebug() << "Settings: Error Setting Data bits." << m_port->errorString();
    }

    emit dataBitsChanged(arg);
}

QSerialPort::Parity Settings::parity() const
{
    return m_parity;
}


void Settings::setParity(QSerialPort::Parity arg)
{
    if (m_parity == arg) return;
    m_parity = arg;

    if (m_port && m_port->isOpen()) {
        if (!m_port->setParity(arg))
            qDebug() << "Settings: Error Setting Parity." << m_port->errorString();
    }

    emit parityChanged(arg);
}

QSerialPort::StopBits Settings::stopBits() const
{
    return m_stopBits;
}


void Settings::setStopBits(QSerialPort::StopBits arg)
{
    if (m_stopBits == arg) return;
    m_stopBits = arg;
    if (m_port && m_port->isOpen()) {
        if (!m_port->setStopBits(arg))
            qDebug() << "Settings: Error Setting Stop bits." << m_port->errorString();
    }

    emit stopBitsChanged(arg);
}

Settings::TerminalCharacters Settings::terminalCharacters() const
{
    return m_terminalCharacters;
}


void Settings::setTerminalCharacters(Settings::TerminalCharacters arg)
{
    if (m_terminalCharacters == arg) return;
    m_terminalCharacters = arg;
    emit terminalCharactersChanged(arg);
}

bool Settings::echo() const
{
    return m_echo;
}


void Settings::setEcho(bool arg)
{
    if (m_echo == arg) return;
    m_echo = arg;
    emit echoChanged(arg);
}

bool Settings::autoOpenTerminal() const
{
    return m_autoOpenTerminal;
}


void Settings::setAutoOpenTerminal(bool arg)
{
    if (m_autoOpenTerminal == arg) return;
    m_autoOpenTerminal = arg;
    emit autoOpenTerminalChanged(arg);
}

bool Settings::logDownload() const
{
    return m_logDownload;
}


void Settings::setLogDownload(bool arg)
{
    if (m_logDownload == arg) return;
    m_logDownload = arg;
    emit logDownloadChanged(arg);
}

bool Settings::wrapTerminal() const
{
    return m_wrapTerminal;
}


void Settings::setWrapTerminal(bool arg)
{
    if (m_wrapTerminal == arg) return;
    m_wrapTerminal = arg;
    emit wrapTerminalChanged(arg);
}

QStringList Settings::hexFiles() const
{
    return m_hexFiles;
}


void Settings::setHexFiles(QStringList arg)
{
    if (m_hexFiles == arg) return;
    m_hexFiles = arg;
    emit hexFilesChanged(arg);
}

QUrl Settings::hexFile() const
{
    return m_hexFile;
}


void Settings::setHexFile(QUrl arg)
{
    if (m_hexFile == arg) return;
    m_hexFile = arg;
    emit hexFileChanged(arg);
}

Settings::ResetType Settings::resetType() const
{
    return m_resetType;
}


void Settings::setResetType(Settings::ResetType arg)
{
    if (m_resetType == arg) return;
    m_resetType = arg;
    emit resetTypeChanged(arg);
}

QSerialPort *Settings::getPort()
{
    if (!m_port) return 0;

    if (m_port->portName() != portName()) {
        qDebug() << "Port names do not match up. They should...";
    }

    if (m_port->isOpen())
        qDebug() << "Port already open...";

    m_port->clear();

    if (m_port->isOpen() || m_port->open(QIODevice::ReadWrite)) {
        if (updatePort()) {
            return m_port;
        } else if (m_port->isOpen()) {
            qDebug() << "Error Opening Port." << m_port->errorString();
            m_port->close();
        }
    } else {
        qDebug() << "Error opening port." << m_port->errorString();
    }

    return 0;
}

bool Settings::updatePort()
{
    if (!m_port->isOpen()) return false;
    qDebug() << "Updating port";
    bool success = m_port->setDataBits(dataBits())
    && m_port->setStopBits(stopBits())
    && m_port->setParity(parity())
    && m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (m_programmerActive)
        success = success && m_port->setBaudRate(baudProgram());
    else if (m_terminalActive)
        success = success && m_port->setBaudRate(baudTerminal());

    if (!success)
        qDebug() << "Error updating port." << m_port->errorString();

    return success;

}

QString Settings::printPortInfo()
{
    QString result;
    QTextStream stream(&result);
    stream << "PortName: " << portName();
    stream << "\nBaud (Prog):" << baudProgram();
    stream << "\nBaud (Term):" << baudTerminal();
    stream << "\nData Bits:" << dataBits();
    stream << "\nStop Bits:" << stopBits();
    stream << "\nParity:" << parity();

    return result;
}

QStringList Settings::availablePorts() const
{
    return m_availablePorts;
}

QString Settings::log() const
{
    return m_log;
}

void Settings::clearLog()
{
    m_log.clear();
    emit logChanged();
}

bool Settings::programmerActive() const
{
    return m_programmerActive;
}


void Settings::setProgrammerActive(bool arg)
{
    if (m_programmerActive == arg) return;
    m_programmerActive = arg;
    emit programmerActiveChanged(arg);
}

bool Settings::terminalActive() const
{
    return m_terminalActive;
}


void Settings::setTerminalActive(bool arg)
{
    if (m_terminalActive == arg) return;
    m_terminalActive = arg;
    emit terminalActiveChanged(arg);
}
