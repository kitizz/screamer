#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QSerialPort>

class Serial : public QObject
{
    Q_OBJECT
    Q_ENUMS(QSerialPort::BaudRate)
    Q_ENUMS(QSerialPort::DataBits)
    Q_ENUMS(QSerialPort::Parity)
    Q_ENUMS(QSerialPort::StopBits)
    Q_ENUMS(QSerialPort::FlowControl)
    Q_ENUMS(QSerialPort::SerialPortError)
    Q_ENUMS(Chip)
    Q_ENUMS(TerminalCharacters)

public:
    explicit Serial(QObject *parent = 0);

    enum Chip { Atmega168=0, Atmega328=1 };
    enum TerminalCharacters { Ascii=0, Hex=1, Dec=2 };

signals:
    
public slots:
    
};

#endif // SERIAL_H
