#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QDebug>
#include <QDir>

struct RaidDev
{
    QString name;
    QString level;
    QString state;
};

class Scanner : public QObject
{
    Q_OBJECT
public:
    explicit Scanner(QObject *parent = 0);

    QList<RaidDev> get_devices();

signals:

public slots:

};

#endif // SCANNER_H
