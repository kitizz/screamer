#include <QtWidgets/QApplication>
#include <QtQml>
#include <QtQuick/QQuickView>

#include "util.h"
#include "settings.h"
#include <QSerialPort>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qmlRegisterType<Settings>("Screamer", 1,0, "Settings");
    qmlRegisterType<QSerialPort>("Screamer", 1,0, "Serial");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(QUrl(Util::adjustPath("qml/Screamer/main.qml")));
    if ( !component.isReady() ) {
        qWarning("%s", qPrintable(component.errorString()));
        return -1;
    }
    QObject *topLevel = component.create();
    QQuickWindow *window = qobject_cast<QQuickWindow *>(topLevel);
    if ( !window ) {
        qWarning("Error: Your root item has to be a Window.");
        return -1;
    }
    QObject::connect(&engine, SIGNAL(quit()), &app, SLOT(quit()));
    window->show();
    return app.exec();
}
