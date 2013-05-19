#include "programmer.h"

#include <QDebug>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include <QtCore/qmath.h>
#include "util.h"

#define MAX_MEM_SIZE 32768
#define SIZE_ATMEGA328 32768
#define SIZE_ATMEGA168 16384

char slave_ready = (char)0x05;
char loadmode_start = (char)0x06;
char datablock_success = (char)0x54;
char datablock_failure = (char)0x07;

Programmer::Programmer(QObject *parent) :
    QObject(parent),
    m_isProgramming(false),
    m_resends(0),
    m_currentAddress(0),
    m_lastAddress(0),
    m_progress(0),
    m_status(Idle),
    m_statusText("Idle")
{
    m_worker = new Worker(this);
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &Programmer::programMicro, m_worker, &Worker::kayGo);
    connect(this, &Programmer::stopProgramming, m_worker, &Worker::stopProgramming);
    m_workerThread.start();
}

void Programmer::stopProgramming()
{
    m_worker->stopProgramming();
}

bool Worker::openPort(QSerialPort *port) {
    if (port->isOpen()) {
        qDebug() << "Port already open...";
        return true;
    }
    port->open(QIODevice::ReadWrite);

    if (port->error() != QSerialPort::NoError) {
        qDebug() << "Error Opening Port Terminal:" << port->error();
        return false;
    }
    return true;
}

void Worker::programMicro(Settings *settings, QSerialPort *port)
{
    setStatus(Programmer::Idle);

    switch (settings->chip()) {
    case Settings::Atmega168:
        m_fileBuffer.resize(SIZE_ATMEGA168);
        break;
    case Settings::Atmega328:
    case Settings::Atmega32u4:
        m_fileBuffer.resize(SIZE_ATMEGA328);
        break;
    }

    qDebug() << "Loading HEX file";
    if (!loadHexFile(settings->hexFile(), &m_fileBuffer, &m_startAddress, &m_endAddress, settings)) {
        settings->writeLogLn("Error: Unable to Load Hex File");
        setStatus(Programmer::Error);
        return;
    }

    qDebug() << "Loaded Hex File";

    // Set the reset type...
    switch (settings->resetType()) {
    case Settings::RTS:
        port->setRequestToSend(false);
        break;
    case Settings::DTR:
        port->setDataTerminalReady(false);
        break;
    }

    qDebug() << "Opening Port";
    setStatus(Programmer::Connecting);

    bool success = false;
    for (int i=0; i<2; ++i) {
        success = openPort(port);
        if (success) break;
        if (m_stopProgramming) break;
    }
    if (!success) {
        setStatus(Programmer::Error, "Unable to open chip.");
        return;
    }

    qDebug() << "Entering program mode";
    // Enter programming mode...
    if (!startProgramMode(port, settings)) {
        settings->writeLogLn("Unable to Enter Programming Mode");
        return;
    }

    qDebug() << "Start sending program";
    // Send over the program.
    if (!sendProgram(port, m_fileBuffer, m_startAddress, m_endAddress, settings)) {
        settings->writeLogLn("Sending Program was unsuccessful.");
        return;
    }

    if (port->isOpen()) port->close();
    setStatus(Programmer::Idle);
    setProgress(0, 0, 0);
    m_programmer->setResends(0);

    Util::resetMicro(port, settings);


}

void Worker::resetMicro(Settings *settings)
{
    QSerialPort *port = new QSerialPort(settings->portName());
    Util::resetMicro(port, settings);
}

bool Worker::startProgramMode(QSerialPort *port, Settings *settings)
{
    settings->writeLogLn("Sending Chip into Program Mode...");
    setStatus(Programmer::Connecting, "Waiting for target chip to broadcast boot.");

//    Util::resetMicro(port, settings);

    while(true) {
        if (m_stopProgramming) {
            setStatus(Programmer::Error, "Programming cancelled. Target chip did not enter programming mode.");
            settings->writeLogLn("Programming cancelled before chip entered programming mode.");
            return false;
        }
        if (port->bytesAvailable() == 0) {
            QThread::msleep(50);
            continue;
        }

        settings->writeLog("Receiving data...");
        QByteArray response = port->readAll();
        settings->writeLog(QString(response));
        settings->writeLogLn("<-" + Util::byte2hex(response));
        if (response.indexOf(slave_ready) >= 0) {
            settings->writeLogLn("Received Broadcast!");
            break; // We have a winner!
        }
    }

    // Now put the chip into program mode
    port->write(&loadmode_start, 1);
    settings->writeLogLn("->" + loadmode_start);

    setStatus(Programmer::Connected, "Load Mode Command Sent");
    return true;
}

bool Worker::sendProgram(QSerialPort *port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings)
{
    int currentAddress = startAddress;
    int blockSize = 0;
    int pageSize;
    QByteArray header(4, 0);

    switch (settings->chip()) {
    case Settings::Atmega168:
    case Settings::Atmega328:
        pageSize = 128;
        break;
    case Settings::Atmega32u4:
        pageSize = 256;
        break;
    default:
        qWarning() << "No page size for chip type.";
        settings->writeLogLn("Error: Undefined Page size for chip type.");
        setStatus(Programmer::Error, "No page size for chip");
        return false;
    }

    while (currentAddress <= endAddress) {

        while (port->bytesAvailable() == 0) {
            QThread::msleep(10);
            if (m_stopProgramming) {
                break;
            }
        }

        if (m_stopProgramming) {
            m_stopProgramming = false;
            setStatus(Programmer::Error, "The target chip did not finish loading. You will likely experience unexpected program execution.");
            break;
        }

        char response;
        port->read(&response, 1);
        settings->writeLogLn("<-" + Util::int2hex((int)response));

        if (response == datablock_success) {
            setStatus(Programmer::Programming);
        } else if (response == datablock_failure) {
            if (blockSize == 0) {
                QString msg = "Error : Incorrect initial response from target IC. Programming is incomplete and will now halt.";
                settings->writeLogLn(msg);
                setStatus(Programmer::Error, msg);
                return false;
            }
            setStatus(Programmer::Failure);
            currentAddress = currentAddress - blockSize;
            m_programmer->resendsIncrement();
        } else {
            // TODO: THis is probably not necessarilly the best
            QString msg = "Error : Incorrect response from target IC. Programming is incomplete and will now halt.";
            settings->writeLogLn(msg);
            setStatus(Programmer::Error, msg);
            return false;
        }

        // Update the progress
        setProgress(currentAddress, endAddress, (currentAddress - startAddress)/(endAddress - startAddress + 1));

        blockSize = qMin(pageSize, endAddress - currentAddress + 1);

        int memAddressHigh = currentAddress / 256;
        int memAddressLow = currentAddress % 256;

        int checkSum = 0;
        checkSum += blockSize;
        checkSum += memAddressHigh;
        checkSum += memAddressLow;

        for (int j=0; j<blockSize; ++j)
            checkSum += fileBuffer[currentAddress + j];

        // Reduce checksum to 8 bits
        while (checkSum > 256) checkSum -= 256;
        // Two's compliment
        checkSum = 256 - checkSum;

        // Send Start character
        port->write(":", 1);

        // Send record header
        header[0] = (char)blockSize;
        header[1] = (char)memAddressLow;
        header[2] = (char)memAddressHigh;
        header[3] = (char)checkSum;
        port->write(header);

        // Send the record data
        port->write(fileBuffer.mid(currentAddress, blockSize), blockSize);

        QTextStream msg;
        msg << "-> :" << Util::byte2hex(header) << "[+"
            << blockSize << " bytes of data]";
        settings->writeLogLn(*msg.string());

        currentAddress += blockSize;
    }

    // Woo, success =)
    // Need to tell the chip that we're done
    port->write(":S");
    settings->writeLogLn("-> :S");

    setStatus(Programmer::Programming);
    return true;
}

bool Worker::loadHexFile(QUrl fileUrl, QByteArray *data, int *startAddress, int *endAddress, Settings *settings)
{
    QFile file(fileUrl.toLocalFile());

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Programmer: Could not open file:" << fileUrl.toLocalFile();
        return false;
    }

    data->fill(0xFF);
    QTextStream in(&file);

    *endAddress = MAX_MEM_SIZE;
    *startAddress = -1;

    QString line;
    int lineNumber = 0;
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        lineNumber++;

        if (line.length() == 0) continue;

        if (line.startsWith(":")) {
            QString sub = line.mid(7,2);

            if (sub=="02" || sub=="04") {
                //	02 - extended segment address record
                //	04 - extended linear address record
                QTextStream msg;
                msg << "Warning on line " << lineNumber << ". Unsupported record type (whatever that means...)";
                settings->writeLog(*msg.string());
            } else if (sub=="01") {
                // 01 - End of File
                file.close();
                return true;
            } else if (sub=="00") {
                // This line is dandy
            } else {
                QTextStream msg;
                msg << "Warning on line " << lineNumber << ". Unknown record type (whatever that means...)";
                settings->writeLog(*msg.string());
            }

            bool ok;
            int byteCount = line.mid(1,2).toUInt(&ok, 16);
            if (!ok) {
                settings->writeLogLn("Unable to convert hex to int:" + line.mid(1,2));
                continue;
            }

            if (byteCount == 0) continue;

            int memoryAddressHigh = line.mid(3, 2).toUInt(0, 16);
            int memoryAddressLow = line.mid(5, 2).toUInt(0, 16);
            int memoryAddress = (memoryAddressHigh * 256) + memoryAddressLow;

            for (int idx = 0; idx < byteCount; idx++) {
                int address = memoryAddress + idx;
                if (address >= m_fileBuffer.length()) {
                    QTextStream msg;
                    msg << "Error at line " << lineNumber << ". Address " << Util::int2hex(address)
                        << " out of buffer (max=" << Util::byte2hex(m_fileBuffer) << ").";
                    settings->writeLogLn(*msg.string());
                }

                if (address > *endAddress) *endAddress = address;
                if (address < *startAddress) *startAddress = address;
                (*data)[address] = (char)(line.mid(idx*2 + 9, 2).toUInt(0, 16));
            }
            continue;
        }

        if (line.startsWith("S")) {
            QTextStream msg;
            msg << "Error in line " << lineNumber << ". Motorola S format not supported.";
            settings->writeLogLn(*msg.string());
            file.close();
            return false;
        }
    } // End While line read

    file.close();

    return true;
}


void Programmer::setIsProgramming(bool arg)
{
    if (m_isProgramming == arg) return;
    m_isProgramming = arg;
    emit isProgrammingChanged(arg);
}


bool Programmer::isProgramming() const
{
    return m_isProgramming;
}

qreal Programmer::progress() const
{
    return m_progress;
}


void Programmer::setProgress(qreal arg)
{
    if (m_progress == arg) return;

    // Limit the progress to 0.0->1.0
    if (arg > 1) arg = 1;
    if (arg < 0) arg = 0;

    m_progress = arg;
    emit progressChanged(arg);
}

Programmer::Status Programmer::status() const
{
    return m_status;
}


void Programmer::setStatus(Programmer::Status arg)
{
    if (m_status == arg) return;
    m_status = arg;
    emit statusChanged(arg);
}

QString Programmer::statusText() const
{
    return m_statusText;
}


void Programmer::setStatusText(QString arg)
{
    if (m_statusText == arg) return;
    m_statusText = arg;
    emit statusTextChanged();
}

int Programmer::resends() const
{
    return m_resends;
}

void Worker::stopProgramming()
{
    m_stopProgramming = true;
}


void Programmer::setResends(int arg)
{
    if (m_resends == arg) return;
    m_resends = arg;
    emit resendsChanged(arg);
}

void Programmer::resendsIncrement()
{
    emit resendsChanged(++m_resends);
}

int Programmer::currentAddress() const
{
    return m_currentAddress;
}


void Programmer::setCurrentAddress(int arg)
{
    if (m_currentAddress == arg) return;
    m_currentAddress = arg;
    emit currentAddressChanged(arg);
}


void Programmer::setLastAddress(int arg)
{
    if (m_lastAddress == arg) return;
    m_lastAddress = arg;
    emit lastAddressChanged(arg);
}

int Programmer::lastAddress() const
{
    return m_lastAddress;
}


void Worker::setStatus(Programmer::Status status, QString statusText)
{
    m_programmer->setStatus(status);

    if (statusText.isEmpty()) {
        switch (status) {
        case Programmer::Idle: statusText = "Idle"; break;
        case Programmer::Connecting: statusText = "Connecting"; break;
        case Programmer::Connected: statusText = "Connected"; break;
        case Programmer::Programming: statusText = "Programming"; break;
        case Programmer::Failure: statusText = "Failure"; break;
        case Programmer::Error: statusText = "Error"; break;
        }
    }
    m_programmer->setStatusText(statusText);
}

void Worker::setProgress(int current, int total, qreal progress)
{
    m_programmer->setProgress(progress);
    m_programmer->setCurrentAddress(current);
    m_programmer->setLastAddress(total);
}

void Worker::kayGo(Settings *settings)
{
    if (m_running) return;
    m_programmer->setIsProgramming(true);
    m_running = true;

    // Create and open the serial port
    QSerialPort *port = new QSerialPort(settings->portName());
    while(port->isOpen()) {
        port->close();
        QThread::msleep(100);
    }

    port->setBaudRate(settings->baudProgram());
    port->setDataBits(settings->dataBits());
    port->setStopBits(settings->stopBits());
    port->setParity(settings->parity());
    programMicro(settings, port);

    m_stopProgramming = false;
    m_running = false;
    m_programmer->setIsProgramming(false);
}


Worker::Worker(QObject *parent) : QObject(parent),
    m_programmer(0),
    m_running(false),
    m_stopProgramming(false),
    m_fileBuffer(QByteArray(MAX_MEM_SIZE, 0xFF)), // TODO: Perhaps have this as a constant somewhere?
    m_startAddress(-1),
    m_endAddress(MAX_MEM_SIZE)
{
}

Worker::Worker(Programmer *prog, QObject *parent): QObject(parent),
    m_running(false),
    m_stopProgramming(false),
    m_fileBuffer(QByteArray(MAX_MEM_SIZE, 0xFF)), // TODO: Perhaps have this as a constant somewhere?
    m_startAddress(-1),
    m_endAddress(MAX_MEM_SIZE)
{
    m_programmer = prog;
}
