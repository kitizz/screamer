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
    connect(this, &Programmer::startProgramming, m_worker, &Worker::kayGo);
    connect(m_worker, &Worker::closePort, this, &Programmer::closePort);

    m_workerThread.start();
}

void Programmer::programMicro(Settings *settings)
{
    setIsProgramming(true);

    if (!openPort(settings)) {
        setIsProgramming(false);
        return;
    }

    emit startProgramming(settings, m_port);
}

void Programmer::resetMicro(Settings *settings)
{
    QSerialPort *port = settings->getPort();

    if (!port) {
        settings->writeLogLn("Reset unsuccessful. Port could not be opened.");
        qDebug() << "Unable to open the port";
        return;
    }

    Util::resetMicro(port, settings);

    port->close();
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

    setStatus(Programmer::Connecting);

//    port->setRequestToSend(true);

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

    setStatus(Programmer::Idle);
    setProgress(0, 0, 0);
    m_programmer->setResends(0);

    Util::resetMicro(port, settings);
}

bool Worker::startProgramMode(QSerialPort *port, Settings *settings)
{
    settings->writeLogLn("Sending Chip into Program Mode...");
    setStatus(Programmer::Connecting, "Waiting for target chip to broadcast boot.");

    Util::resetMicro(port, settings);

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
    port->flush();
    settings->writeLogLn("->" + loadmode_start);

    setStatus(Programmer::Connected, "Load Mode Command Sent");
    return true;
}

bool Worker::sendProgram(QSerialPort *port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings)
{
    int currentAddress = startAddress;
    int blockSize = 0;
    int pageSize;
    QByteArray header(5, 0);

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

    while (true) {

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

        if (response == slave_ready) {
            // Hmmm a stray signal
            port->write(&loadmode_start, 1);
            port->flush();
            continue;
        } else if (response == datablock_success) {
            setStatus(Programmer::Programming);
            if (currentAddress > endAddress) break;
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

        int blockSizeHigh = blockSize / 256;
        int blockSizeLow = blockSize % 256;

        unsigned int checkSum = 0;
        checkSum += blockSizeHigh;
        checkSum += blockSizeLow;
        checkSum += memAddressHigh;
        checkSum += memAddressLow;

        for (int j=0; j<blockSize; ++j)
            checkSum += fileBuffer[currentAddress + j];

        // Reduce checksum to 8 bits
//        while (checkSum < 0) checkSum += 256;
        while (checkSum > 256) checkSum -= 256;
        // Two's compliment
        unsigned char checkSumChar = (unsigned char)(256 - checkSum);
        qDebug() << "CheckSumChar" << (int)checkSumChar;

        // Send Start character
        port->write(":", 1);

        // Send record header
        if (pageSize >= 256) {
            header[0] = (char)blockSizeLow;
            header[1] = (char)blockSizeHigh;
            header[2] = (char)memAddressLow;
            header[3] = (char)memAddressHigh;
            header[4] = (char)checkSumChar;
            port->write(header);

        } else {
            header[0] = (char)blockSize;
            header[1] = (char)memAddressLow;
            header[2] = (char)memAddressHigh;
            header[3] = (char)checkSumChar;
            port->write(header.mid(0,4));
        }
//        header[3] = 0x59;

        // Send the record data
        port->write(fileBuffer.mid(currentAddress, blockSize));
        port->flush();

        QString msg;
        QTextStream msgStream(&msg);
        msgStream << "-> :" << Util::byte2hex(header) << "[+"
            << blockSize << " bytes of data]";
        settings->writeLogLn(msg);

        currentAddress += blockSize;
    }

    // Woo, success =)
    // Need to tell the chip that we're done
    if (pageSize >= 256)
        port->write(": S");
    else
        port->write(":S");

    port->flush();
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

    QString msg;
    QTextStream msgStream(&msg);

    *endAddress = -1;
    *startAddress = MAX_MEM_SIZE;

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
                msg.clear();
                msgStream << "Warning on line " << lineNumber << ". Unsupported record type (whatever that means...)";
                settings->writeLog(msg);
            } else if (sub=="01") {
                // 01 - End of File
                file.close();
                return true;
            } else if (sub=="00") {
                // This line is dandy
            } else {
                msg.clear();
                msgStream << "Warning on line " << lineNumber << ". Unknown record type (whatever that means...)";
                settings->writeLog(msg);
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
                    msg.clear();
                    msgStream << "Error at line " << lineNumber << ". Address " << Util::int2hex(address)
                        << " out of buffer (max=" << Util::byte2hex(m_fileBuffer) << ").";
                    settings->writeLogLn(msg);
                }

                if (address > *endAddress) *endAddress = address;
                if (address < *startAddress) *startAddress = address;
                (*data)[address] = (char)(line.mid(idx*2 + 9, 2).toUInt(0, 16));
            }
            continue;
        }

        if (line.startsWith("S")) {
            msg.clear();
            msgStream << "Error in line " << lineNumber << ". Motorola S format not supported.";
            settings->writeLogLn(msg);
            file.close();
            return false;
        }
    } // End While line read

    file.close();

    return true;
}


void Programmer::setIsProgramming(bool arg, Settings *settings)
{
    if (m_isProgramming == arg) return;
    m_isProgramming = arg;
    if (!arg && settings) {
        if (settings->autoOpenTerminal())
            settings->setTerminalActive(true);
    }
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
    qDebug() << "Stop Programming";
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

void Worker::kayGo(Settings *settings, QSerialPort *port)
{
    if (m_running) return;
//    m_programmer->setIsProgramming(true);
    m_running = true;

    programMicro(settings, port);

    port->clear();

    m_stopProgramming = false;
    m_running = false;
    m_programmer->setIsProgramming(false, settings);

    if (!settings->terminalActive())
        emit closePort();
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


QSerialPort *Programmer::openPort(Settings *settings)
{
    m_port = settings->getPort();

    if (!m_port) return 0;

    emit portOpened(m_port);
    return m_port;
}

void Programmer::closePort()
{
    qDebug() << "Attempt to close port";
    if (!m_port || !m_port->isOpen()) return;

    m_port->close();
    while (m_port->isOpen()) QThread::msleep(50);
    qDebug() << "Port Closed";
}


void Programmer::stopProgramming()
{
    m_worker->stopProgramming();
}
