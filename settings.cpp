#include "settings.h"

#include <QVariant>
#include "util.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

Settings::Settings(QObject *parent) :
    QObject(parent)
{
}

void Settings::load()
{
    QFile file(settingsFile().path());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to access settings file:" << settingsFile();
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromBinaryData(data);
    if (!doc.isObject()) {
        qWarning() << "Settings File in unexpected format. Unable to load settings.";
        file.close();
        return;
    }

    QVariantMap map = doc.object().toVariantMap();

    // Update the settings:
    Util::qvariant2qobject(map, this);
}

void Settings::save()
{
    QVariantMap map = Util::qobject2qvariant(this);
    QJsonObject json = QJsonObject::fromVariantMap(map);
    QJsonDocument doc = QJsonDocument(json);

    QFile file(settingsFile().path());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to Save to file:" << settingsFile();
        return;
    }

    QTextStream out(&file);
    int size;
    out << doc.rawData(&size);
    out.flush();

    file.close();

    return;
}

void Settings::writeLog(QString log)
{
    m_log.append(log + " ");
}

void Settings::writeLogLn(QString log)
{
    m_log.append(log + "\n");
}

void Settings::setSettingsFile(QUrl arg)
{
    if (m_settingsFile == arg) return;
    m_settingsFile = arg;
    emit settingsFileChanged(arg);
}

QString Settings::port() const
{
    return m_port;
}


QUrl Settings::settingsFile() const
{
    return m_settingsFile;
}


void Settings::setPort(QString arg)
{
    if (m_port == arg) return;
    m_port = arg;
    emit portChanged(arg);
}

int Settings::baudProgram() const
{
    return m_baudProgram;
}


void Settings::setBaudProgram(int arg)
{
    if (m_baudProgram == arg) return;
    m_baudProgram = arg;
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
    emit stopBitsChanged(arg);
}

Serial::TerminalCharacters Settings::terminalCharacters() const
{
    return m_terminaCharacters;
}


void Settings::setTerminalCharacters(Serial::TerminalCharacters arg)
{
    if (m_terminaCharacters == arg) return;
    m_terminaCharacters = arg;
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
