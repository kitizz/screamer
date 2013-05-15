#include "programmer.h"

#include <QFile>
#include <QThread>
#include <QTextStream>
#include <QtCore/qmath.h>
#include "util.h"

#define MAX_MEM_SIZE 32768
#define SIZE_ATMEGA328 32768
#define SIZE_ATMEGA168 16384

#define SLAVE_READY     ((char)0x05)
#define LOADMODE_START  ((char)0x06)
#define DATABLOCK_SUCCESS  ((char)0x54)
#define DATABLOCK_FAILURE  ((char)0x07)

Programmer::Programmer(QObject *parent) :
    QObject(parent),
    m_isProgramming(false),
    m_fileBuffer(QByteArray(MAX_MEM_SIZE)), // TODO: Perhaps have this as a constant somewhere?
    m_stopProgramming(false),
    m_startAddress(-1),
    m_endAddress(MAX_MEM_SIZE),
    m_resends(0)
{
}

void Programmer::programMicro(QSerialPort port, Settings *settings)
{
    setStatus(Idle);

    setIsProgramming(true);

    int pageSize;		// For ATmega168, we have to load 128 bytes at a time (program full page). Document: AVR095
    int memoryAddressHigh;
    int memoryAddressLow;
    int checkSum;
    int lastAddress;
    int currentAddress;
    QString currentFile;
    int resend = 0;

//    byte[] buffer = new byte[10];
    bool success = false, badfile = true;
    int answer;
    QString filename;

    switch (settings->chip()) {
    case Settings::Atmega168:
        m_fileBuffer.resize(SIZE_ATMEGA168);
        break;
    case Settings::Atmega328:
        m_fileBuffer.resize(SIZE_ATMEGA328);
        break;
    }

    if (!loadHexFile(settings->hexFile(), &m_fileBuffer, &m_startAddress, &m_endAddress, settings)) {
        settings->writeLogLn("Error: Unable to Load Hex File");
        setStatus(Error);
        return;
    }

    // Set the reset type...
    switch (settings->resetType()) {
    case Settings::RTS:
        port.setRequestToSend(false);
        break;
    case Settings::DTR:
        port.setDataTerminalReady(false);
        break;
    }

    port.open(QSerialPort::ReadWrite);
    setStatus(Connecting);

    // Enter programming mode...
    if (!startProgramMode(port, settings)) {
        settings->writeLogLn("Unable to Enter Programming Mode");
        return;
    }

    // Send over the program.
    if (!sendProgram(port, m_fileBuffer, m_startAddress, m_endAddress, settings)) {
        settings->writeLogLn("Sending Program was unsuccessful.");
        return;
    }

    if (port.isOpen()) port.close();
    setStatus(Idle);
    Util::resetMicro(port, settings);


}

bool Programmer::startProgramMode(const QSerialPort &port, Settings *settings)
{
    settings->writeLogLn("Sending Chip into Program Mode...");
    setStatusText("Waiting for target chip to broadcast boot.");

    Util::resetMicro(port, settings);

    while(true) {
        if (m_stopProgramming) {
            setStatusText("Programming cancelled. Target chip did not enter programming mode.");
            settings->writeLogLn("Programming cancelled before chip entered programming mode.");
            return false;
        }
        if (port.bytesAvailable() == 0) {
            QThread::msleep(50);
            continue;
        }

        QByteArray response = port.readAll();
        settings->writeLogLn("<-" + Util::byte2hex(response));
        if (response.indexOf(SLAVE_READY) >= 0) {
            settings->writeLogLn("Received Broadcast!");
            break; // We have a winner!
        }
    }

    // Now put the chip into program mode
    port.write(LOADMODE_START, 1);
    settings->writeLogLn("->" + LOADMODE_START);

    setStatusText("Load Mode Command Sent");
}

bool Programmer::sendProgram(const QSerialPort &port, const QByteArray &fileBuffer, int startAddress, int endAddress, Settings *settings)
{
    int currentAddress = startAddress;
    int blockSize = 0;
    int pageSize;
    QByteArray header(4);

    switch (settings->chip()) {
    case Settings::Atmega168:
    case Settings::Atmega328:
        pageSize = 128;
    }

    while (currentAddress <= endAddress) {

        while (port.bytesAvailable() == 0) {
            QThread::msleep(10);
            if (m_stopProgramming) {
                break;
            }
        }

        if (m_stopProgramming) {
            setStatusText("The target chip did not finish loading. You will likely experience unexpected program execution.");
            break;
        }

        char response;
        port.read(&response, 1);
        settings->writeLogLn("<-" + int2hex(response));

        if (response == DATABLOCK_SUCCESS) {
            setStatus(Programming);
        } else if (response == DATABLOCK_FAILURE) {
            if (blockSize == 0) {
                QString msg = "Error : Incorrect initial response from target IC. Programming is incomplete and will now halt.";
                settings->writeLogLn(msg);
                setStatusText(msg);
                setStatus(Error);
                return false;
            }
            setStatus(Failure);
            currentAddress = currentAddress - blockSize;
            resendsIncrement();
        } else {
            QString msg = "Error : Incorrect response from target IC. Programming is incomplete and will now halt.";
            settings->writeLogLn(msg);
            setStatusText(msg);
            setStatus(Error);
            return false;
        }

        // Update the progress
        setCurrentAddress(currentAddress);
        setProgress((currentAddress - startAddress)/(endAddress - startAddress + 1));

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
        port.write(":", 1);

        // Send record header
        header[0] = (char)blockSize;
        header[1] = (char)memAddressLow;
        header[2] = (char)memAddressHigh;
        header[3] = (char)checkSum;
        port.write(header);

        // Send the record data
        port.write(fileBuffer.mid(currentAddress, blockSize), blockSize);

        QString msg;
        msg << "-> :" << Util::byte2hex(header) << "[+"
            << blockSize << " bytes of data]";
        settings->writeLogLn(msg);

        currentAddress += blockSize;
    }

    // Woo, success =)
    // Need to tell the chip that we're done
    port.write(":S");
    settings->writeLogLn("-> :S");

    setStatus(Programming);
}

qint64 Programmer::writeBlock(QSerialPort port, const char* data, qint64 start, qint64 size)
{
    return port.write(&(data[start]), size);
}

bool Programmer::loadHexFile(QUrl fileUrl, QByteArray *data, int *startAddress, int *endAddress, Settings *settings)
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
                QString msg;
                msg << "Warning on line " << lineNumber << ". Unsupported record type (whatever that means...)";
                settings->writeLog(msg);
            } else if (sub=="01") {
                // 01 - End of File
                file.close();
                return true;
            } else if (sub=="00") {
                // This line is dandy
            } else {
                QString msg;
                msg << "Warning on line " << lineNumber << ". Unknown record type (whatever that means...)";
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
                    QString msg;
                    msg << "Error at line " << lineNumber << ". Address " << Util::int2hex(address)
                        << " out of buffer (max=" << Util::int2hex(m_fileBuffer) << ").";
                    settings->writeLogLn(msg);
                }

                if (address > *endAddress) *endAddress = address;
                if (address < *startAddress) *startAddress = address;
                data[address] = (char)(line.mid(idx*2 + 9, 2).toUInt(0, 16));
            }
            continue;
        }

        if (line.startsWith("S")) {
            QString msg;
            msg << "Error in line " << lineNumber << ". Motorola S format not supported.";
            settings->writeLogLn(msg);
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
    emit statusTextChanged(arg);
}

int Programmer::resends() const
{
    return m_resends;
}


void Programmer::stopProgramming()
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
    emit resendsChanged(++m_resends)
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
