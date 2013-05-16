#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QStringList>
#include <QSerialPort>
#include <QTimer>
#include <QUrl>
#include "serial.h"

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)

    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
//    Q_PROPERTY(QSerialPort *selectedPort READ selectedPort NOTIFY selectedPortChanged)

    Q_PROPERTY(QSerialPort::BaudRate baudProgram READ baudProgram WRITE setBaudProgram NOTIFY baudProgramChanged)
    Q_PROPERTY(int frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(Chip chip READ chip WRITE setChip NOTIFY chipChanged)

    Q_PROPERTY(QSerialPort::BaudRate baudTerminal READ baudTerminal WRITE setBaudTerminal NOTIFY baudTerminalChanged)
    Q_PROPERTY(QSerialPort::DataBits dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(QSerialPort::Parity parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(QSerialPort::StopBits stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)
    Q_PROPERTY(ResetType resetType READ resetType WRITE setResetType NOTIFY resetTypeChanged)

    Q_PROPERTY(TerminalCharacters terminalCharacters READ terminalCharacters WRITE setTerminalCharacters NOTIFY terminalCharactersChanged)
    Q_PROPERTY(bool echo READ echo WRITE setEcho NOTIFY echoChanged)
    Q_PROPERTY(bool autoOpenTerminal READ autoOpenTerminal WRITE setAutoOpenTerminal NOTIFY autoOpenTerminalChanged)
    Q_PROPERTY(bool logDownload READ logDownload WRITE setLogDownload NOTIFY logDownloadChanged)
    Q_PROPERTY(bool wrapTerminal READ wrapTerminal WRITE setWrapTerminal NOTIFY wrapTerminalChanged)

    Q_PROPERTY(QUrl hexFile READ hexFile WRITE setHexFile NOTIFY hexFileChanged)
    Q_PROPERTY(QStringList hexFiles READ hexFiles WRITE setHexFiles NOTIFY hexFilesChanged)

    Q_ENUMS(Chip TerminalCharacters ResetType)

public:
    enum Chip { Atmega168=0, Atmega328=1 };
    enum TerminalCharacters { Ascii=0, Hex=1, Dec=2 };
    enum ResetType { RTS=0, DTR=1};

    explicit Settings(QObject *parent = 0);

    Q_INVOKABLE bool load();


    Q_INVOKABLE void writeLog(QString log);
    Q_INVOKABLE void writeLogLn(QString log);
    
    QUrl settingsFile() const;
    void setSettingsFile(QUrl arg);

    QString portName() const;
    void setPortName(QString arg);

    QSerialPort::BaudRate baudProgram() const;
    void setBaudProgram(QSerialPort::BaudRate arg);

    int frequency() const;
    void setFrequency(int arg);

    Chip chip() const;
    void setChip(Chip arg);

    QSerialPort::BaudRate baudTerminal() const;
    void setBaudTerminal(QSerialPort::BaudRate arg);

    QSerialPort::DataBits dataBits() const;
    void setDataBits(QSerialPort::DataBits arg);

    QSerialPort::Parity parity() const;
    void setParity(QSerialPort::Parity arg);

    QSerialPort::StopBits stopBits() const;
    void setStopBits(QSerialPort::StopBits arg);

    Settings::TerminalCharacters terminalCharacters() const;
    void setTerminalCharacters(Settings::TerminalCharacters arg);

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

    QUrl hexFile() const;
    void setHexFile(QUrl arg);

    ResetType resetType() const;
    void setResetType(ResetType arg);

    QSerialPort *selectedPort() const;
    void setSelectedPort(QSerialPort *arg);

    Q_INVOKABLE void setupPort(QSerialPort *portName);
    Q_INVOKABLE void getPortFromName(QString portName);

    QStringList availablePorts() const;
signals:
    void changed();

    void settingsFileChanged(QUrl arg);
    void portNameChanged(QString arg);
    void baudProgramChanged(int arg);
    void frequencyChanged(int arg);
    void chipChanged(int arg);

    void baudTerminalChanged(QSerialPort::BaudRate arg);
    void dataBitsChanged(QSerialPort::DataBits arg);
    void parityChanged(QSerialPort::Parity arg);
    void stopBitsChanged(QSerialPort::StopBits arg);

    void terminalCharactersChanged(Settings::TerminalCharacters arg);
    void echoChanged(bool arg);
    void autoOpenTerminalChanged(bool arg);
    void logDownloadChanged(bool arg);
    void wrapTerminalChanged(bool arg);
    void hexFilesChanged(QStringList arg);
    void hexFileChanged(QUrl arg);
    void resetTypeChanged(ResetType arg);

    void selectedPortChanged(QSerialPort *arg);

    void availablePortsChanged(QStringList arg);

public slots:
    Q_INVOKABLE void save();
    Q_INVOKABLE void updatePorts();

private:
    bool m_saving;
    QTimer m_timer;
    QUrl m_settingsFile;
    QString m_log;
    
    QString m_port;
    QSerialPort::BaudRate m_baudProgram;
    int m_frequency;
    Chip m_chip;
    QSerialPort::BaudRate m_baudTerminal;
    QSerialPort::DataBits m_dataBits;
    QSerialPort::Parity m_parity;
    QSerialPort::StopBits m_stopBits;
    Settings::TerminalCharacters m_terminalCharacters;
    bool m_echo;
    bool m_autoOpenTerminal;
    bool m_logDownload;
    bool m_wrapTerminal;
    QStringList m_hexFiles;
    QUrl m_hexFile;
    ResetType m_resetType;
    QSerialPort * m_selectedPort;
    QStringList m_availablePorts;
};

#endif // SETTINGS_H
