#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSerialPort>
#include <QUrl>
#include "serial.h"

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int baudProgram READ baudProgram WRITE setBaudProgram NOTIFY baudProgramChanged)
    Q_PROPERTY(int frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(int chip READ chip WRITE setChip NOTIFY chipChanged)

    Q_PROPERTY(QSerialPort::BaudRate baudTerminal READ baudTerminal WRITE setBaudTerminal NOTIFY baudTerminalChanged)
    Q_PROPERTY(QSerialPort::DataBits dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(QSerialPort::Parity parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(QSerialPort::StopBits stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)

    Q_PROPERTY(Serial::TerminalCharacters terminaCharacters READ terminaCharacters WRITE setTerminaCharacters NOTIFY terminaCharactersChanged)
    Q_PROPERTY(bool echo READ echo WRITE setEcho NOTIFY echoChanged)
    Q_PROPERTY(bool autoOpenTerminal READ autoOpenTerminal WRITE setAutoOpenTerminal NOTIFY autoOpenTerminalChanged)
    Q_PROPERTY(bool logDownload READ logDownload WRITE setLogDownload NOTIFY logDownloadChanged)
    Q_PROPERTY(bool wrapTerminal READ wrapTerminal WRITE setWrapTerminal NOTIFY wrapTerminalChanged)

    Q_PROPERTY(QStringList hexFiles READ hexFiles WRITE setHexFiles NOTIFY hexFilesChanged)

    Q_ENUMS(Chip TerminalCharacters)

public:
    enum Chip { Atmega168=0, Atmega328=1 };
    enum TerminalCharacters { Ascii=0, Hex=1, Dec=2 };

    explicit Settings(QObject *parent = 0);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

    Q_INVOKABLE void writeLog(QString log);
    
    QUrl settingsFile() const;
    void setSettingsFile(QUrl arg);

    QString port() const;
    void setPort(QString arg);

    int baudProgram() const;
    void setBaudProgram(int arg);

    int frequency() const;
    void setFrequency(int arg);

    int chip() const;
    void setChip(int arg);

    QSerialPort::BaudRate baudTerminal() const;
    void setBaudTerminal(QSerialPort::BaudRate arg);

    QSerialPort::DataBits dataBits() const;
    void setDataBits(QSerialPort::DataBits arg);

    QSerialPort::Parity parity() const;
    void setParity(QSerialPort::Parity arg);

    QSerialPort::StopBits stopBits() const;
    void setStopBits(QSerialPort::StopBits arg);

    Serial::TerminalCharacters terminaCharacters() const;
    void setTerminaCharacters(Serial::TerminalCharacters arg);

    bool echo() const;
    void setEcho(bool arg);

    bool autoOpenTerminal() const;
    void setAutoOpenTerminal(bool arg);

    bool logDownload() const;
    void setLogDownload(bool arg);

    bool wrapTerminal() const;
    void setWrapTerminal(bool arg);

    QStringList hexFiles() const;
    void setHexFiles(QStringList arg);

signals:
    void settingsFileChanged(QUrl arg);
    void portChanged(QString arg);
    void baudProgramChanged(int arg);
    void frequencyChanged(int arg);
    void chipChanged(int arg);

    void baudTerminalChanged(QSerialPort::BaudRate arg);
    void dataBitsChanged(QSerialPort::DataBits arg);
    void parityChanged(QSerialPort::Parity arg);
    void stopBitsChanged(QSerialPort::StopBits arg);

    void terminaCharactersChanged(Serial::TerminalCharacters arg);
    void echoChanged(bool arg);
    void autoOpenTerminalChanged(bool arg);
    void logDownloadChanged(bool arg);
    void wrapTerminalChanged(bool arg);
    void hexFilesChanged(QStringList arg);

public slots:

private:
    QUrl m_settingsFile;
    QString m_log;
    
    QString m_port;
    int m_baudProgram;
    int m_frequency;
    int m_chip;
    QSerialPort::BaudRate m_baudTerminal;
    QSerialPort::DataBits m_dataBits;
    QSerialPort::Parity m_parity;
    QSerialPort::StopBits m_stopBits;
    Serial::TerminalCharacters m_terminaCharacters;
    bool m_echo;
    bool m_autoOpenTerminal;
    bool m_logDownload;
    bool m_wrapTerminal;
    QStringList m_hexFiles;
};

#endif // SETTINGS_H
