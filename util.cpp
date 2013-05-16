#include "util.h"
#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>

Util::Util()
{
}


QString Util::int2hex(int i)
{
    QString result;
    result.setNum(i, 16);
    if (i<16)
        result.prepend("0");

    qDebug() << "int2hex:" << i << "to" << result;
    return result;
}

QString Util::byte2hex(QByteArray bytes)
{
    QString result;
    foreach (char b, bytes)
        result.append(" " + int2hex(b));

    return result;
}

QString Util::byte2hex(QByteArray byte, int start, int length)
{
    QString result;
    int top = start + length;
    for (int i = start; i < top; ++i)
        result.append(" " + int2hex(byte[i]));

    return result;
}

QString Util::string2hex(QString s)
{
    QString result;
    for (int i = 0; i < s.length(); ++i)
        result.append(" " + int2hex(s[i].unicode()));
    return result;
}

QString Util::string2decimal(QString s)
{
    QString result;
    for (int i = 0; i < s.length(); ++i) {
        QString dec;
        dec.setNum(s[i].unicode());
        result.append(" " + dec);
    }
    return result;
}

void Util::resetMicro(QSerialPort *port, Settings *settings)
{
    switch(settings->resetType()) {
    case Settings::RTS:
        qDebug() << "Reset: RTS";
        port->setRequestToSend(true);
        QThread::msleep(10);
        port->setRequestToSend(false);
        if (settings->logDownload())
            settings->writeLog("-- Reset RTS\n");
        break;

    case Settings::DTR:
        qDebug() << "Reset: DTR";
        port->setDataTerminalReady(true);
        QThread::msleep(10);
        port->setDataTerminalReady(false);
        if (settings->logDownload())
            settings->writeLog("-- Reset DTR\n");
        break;
    }
    QThread::msleep(10);
}

QList<QSerialPortInfo> Util::getAvailablePorts()
{
    return QSerialPortInfo::availablePorts();
}

QSerialPortInfo Util::findPort(QString name)
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (name == info.portName()) return info;
    }
    return QSerialPortInfo();
}

QVariantMap Util::qobject2qvariant(const QObject *object, const QStringList &ignoredProperties)
{
    QVariantMap result;
    const QMetaObject *metaobject = object->metaObject();
    int count = metaobject->propertyCount();
    for (int i=0; i<count; ++i) {
        QMetaProperty metaproperty = metaobject->property(i);
        const char *name = metaproperty.name();

        if (ignoredProperties.contains(QLatin1String(name)) || (!metaproperty.isReadable()))
            continue;

        QVariant value = object->property(name);
        result[QLatin1String(name)] = value;
    }
    return result;
}

void Util::qvariant2qobject(const QVariantMap &variant, QObject *object)
{
    const QMetaObject *metaobject = object->metaObject();

    QVariantMap::const_iterator iter;
    for (iter = variant.constBegin(); iter != variant.constEnd(); ++iter) {
        int pIdx = metaobject->indexOfProperty( iter.key().toLatin1() );

        if ( pIdx < 0 )
            continue;

        QMetaProperty metaproperty = metaobject->property( pIdx );
        QVariant::Type type = metaproperty.type();
        QVariant v( iter.value() );
        if ( v.canConvert( type ) ) {
            v.convert( type );
            metaproperty.write( object, v );
        } else if (QString(QLatin1String("QVariant")).compare(QLatin1String(metaproperty.typeName())) == 0) {
            metaproperty.write( object, v );
        }
    }
}

QString Util::adjustPath(const QString &path)
{
#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
        return QString::fromLatin1("%1/../Resources/%2")
                .arg(QCoreApplication::applicationDirPath(), path);
#elif !defined(Q_OS_ANDROID)
    const QString pathInInstallDir =
            QString::fromLatin1("%1/../%2").arg(QCoreApplication::applicationDirPath(), path);
    if (QFileInfo(pathInInstallDir).exists())
        return pathInInstallDir;
#endif
#endif
    return path;
}
