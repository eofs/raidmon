#include "scanner.h"

Scanner::Scanner(QObject *parent) :
    QObject(parent)
{
}

QList<RaidDev> Scanner::get_devices() {
    qDebug() << "Scanning...";

    QList<RaidDev> devs;
    QFile mdstat("/proc/mdstat");

    if (!mdstat.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug("Unable to open %s, aborting.", qPrintable(mdstat.fileName()));
        return devs;
    }

    QByteArray data = mdstat.readAll();
    QTextStream in(&data);

    while (!in.atEnd()) {
        QString line = in.readLine();
        RaidDev dev;

        if (line.indexOf("Personalities") == 0)
            continue;

        if (line.indexOf("md") == 0) {
            QStringList words = line.split(" ");
            dev.name = words[0];
            dev.state = words[2];
            dev.level = words[3];
            devs.append(dev);
        }
        qDebug() << line;
    }

    return devs;
}
