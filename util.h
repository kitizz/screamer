/* This file borrows code from qjson (github.com/flavio/qjson)
 *
 *
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License version 2.1, as published by the Free Software Foundation.
*
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef UTIL_H
#define UTIL_H

#include <QString>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtCore/QLatin1String>
#include "settings.h"
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

class Util
{
public:
    static QString int2hex(int i);

    static QString byte2hex(QByteArray bytes);
    static QString byte2hex(QByteArray byte, int start, int length);

    static QString string2hex(QString s);

    static QString string2decimal(QString s);

    static void resetMicro(QSerialPort *port, Settings *settings);

    static QList<QSerialPortInfo> getAvailablePorts();

    static QSerialPortInfo findPort(QString name);

    // From qjson:
    static QVariantMap qobject2qvariant( const QObject* object,
                                                const QStringList& ignoredProperties = QStringList(QString(QLatin1String("objectName"))));

    static void qvariant2qobject(const QVariantMap& variant, QObject* object);

    static QString adjustPath(const QString &path);

private:
    Util();
};

#endif // UTIL_H
