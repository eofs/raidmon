/*
RaidMon - RAID monitoring utility
Copyright (C) 2010  Tomi Pajunen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Contact information:
by email: tomip86@gmail.com
*/

#include <QDBusConnection>
#include <QDBusInterface>

#include "raidmon.h"
#include "ui_raidmon.h"

RaidMon::RaidMon(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaidMon),
    statusClean(false),
    useKDENotifications(true)
{
    ui->setupUi(this);

    // Do some extra initialization
    setupExtra();

    // Run check to get initial status
    readRaidStatus();

    // Start timer (Read status info every 10 seconds)
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readRaidStatus()));
    timer->start(1000 * updateInternval);
}

RaidMon::~RaidMon()
{
    delete ui;
}

void RaidMon::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void RaidMon::setupExtra()
{
    // Setup elements
    createActions();
    createTrayIcon();
    trayIcon->show();

    // Load settigns
    loadSettings();

    // Connect signals & slots
    connect(this, SIGNAL(status(bool,QString,QStringList)), this, SLOT(setTrayIconStatus(bool,QString,QStringList)));

    // Set UI items
    ui->textDevices->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui->textDevices->setPlainText(devices.join("\n"));
    ui->spinInterval->setMinimum(1);
    ui->spinInterval->setValue(updateInternval);
    ui->checkKDENotifications->setChecked(useKDENotifications);



}

void RaidMon::createActions()
{
    settingsAction = new QAction(tr("&Settings"),this);
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(show()));

    aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(quit()));
}

void RaidMon::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(settingsAction);
    trayIconMenu->addAction(aboutAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon::fromTheme("drive-harddisk"));
    trayIcon->setToolTip(tr("Loading RAID information..."));
}

void RaidMon::loadSettings()
{
    QSettings settings("RaidMon","RaidMon");
    devices = settings.value("devices").value<QStringList>();
    updateInternval = settings.value("interval", 10).toInt();
    useKDENotifications = settings.value("kdenotifications", true).toBool();
}

void RaidMon::saveSettings()
{
    QSettings settings("RaidMon", "RaidMon");
    settings.setValue("devices", devices);
    settings.setValue("interval", ui->spinInterval->value());
    settings.setValue("kdenotifications", useKDENotifications);
}

void RaidMon::showAbout()
{
    QMessageBox::information(this, tr("About"), tr("RaidMon - RAID monitoring utility\nAuthor: Tomi Pajunen\nVersion: 0.1"));
}

void RaidMon::accept()
{
    // Use new values
    if (ui->textDevices->toPlainText().count() > 0)
        devices = ui->textDevices->toPlainText().split("\n");
    else
        devices.clear();
    updateInternval = ui->spinInterval->value();
    timer->start(1000 * updateInternval);
    useKDENotifications = ui->checkKDENotifications->isChecked();

    // Save form values
    saveSettings();

    this->hide();

    // Check RAID status
    readRaidStatus();
}

void RaidMon::reject()
{
    this->hide();

    // Reset form values
    ui->textDevices->setPlainText(devices.join("\n"));
    ui->spinInterval->setValue(updateInternval);

}

void RaidMon::readRaidStatus()
{
    if (devices.size() == 0) {
        emit status(true, "Configuration error.", QStringList() << "No devices configured.");
        return;
    }

    bool hasErrors = false;
    QStringList allowedStatuses;
    QStringList messages;
    QString message;
    allowedStatuses << "active" << "clean" << "write-pending";

    foreach (QString dev, devices) {
        QString dir(QString("/sys/block/%1/md/").arg(dev.split("/").last()));
        QFile statusFile(dir + "array_state");

        if (statusFile.open(QIODevice::ReadOnly)) {
            QByteArray status(statusFile.readLine());
            status = status.trimmed();
            if (allowedStatuses.contains(status)) {
                message = QString("Device: %1 - %2").arg(dev).arg(QString(status));
                qDebug() << message;
            } else {
                hasErrors = true;
                message = QString("Device: %1 - Invalid status '%2' detected!").arg(dev).arg(QString(status));
                qDebug() << message;
            }
        } else {
            hasErrors = true;
            message = QString("Could not open file %1").arg(statusFile.fileName());
            qDebug() << message;
        }
        messages << message;
    }

    // Change icon & tooltip
    if (hasErrors) {
        statusClean = false;
        emit status(true, tr("Error!"), messages);

        // Send message to knotify via D-Bus
        if (QDBusConnection::sessionBus().isConnected() && useKDENotifications) {
            QList<QVariant> args;
            args << "warning" << "kde" << QVariant::List << "RAID error detected!" << message << QVariant::ByteArray << QVariant::StringList << 5000 << QVariant::LongLong;
            QDBusInterface dbus_iface("org.kde.knotify", "/Notify", "org.kde.KNotify");
            dbus_iface.callWithArgumentList(QDBus::AutoDetect, "event", args);
        }
    } else if(!statusClean) {
        // If previously had errors but now everything is OK change icon back to normal
        statusClean = true;
        emit status(false, tr("Good!"), messages);
    }
}

void RaidMon::setTrayIconStatus(bool hasErrors, QString status, QStringList messages)
{
    // Set new icon
    if (hasErrors)
        trayIcon->setIcon(QIcon::fromTheme("dialog-error"));
    else
        trayIcon->setIcon(QIcon::fromTheme("drive-harddisk"));

    // Set new tooltip
    trayIcon->setToolTip(QString("Status: <strong>%1</strong><br/>%2").arg(status).arg(messages.join("<br/>")));

}
