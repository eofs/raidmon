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

    scanner = new Scanner(this);

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
    updateInternval = settings.value("interval", 10).toInt();
    useKDENotifications = settings.value("kdenotifications", true).toBool();
}

void RaidMon::saveSettings()
{
    QSettings settings("RaidMon", "RaidMon");
    settings.setValue("interval", ui->spinInterval->value());
    settings.setValue("kdenotifications", useKDENotifications);
}

void RaidMon::showAbout()
{
    QMessageBox::information(this, tr("About"), tr("RaidMon - RAID monitoring utility\nAuthor: Tomi Pajunen\nVersion: %1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_REVISION));
}

void RaidMon::accept()
{
    // Use new values
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
    ui->spinInterval->setValue(updateInternval);
}

void RaidMon::readRaidStatus()
{
    QStringList allowedStatuses;
    QStringList messages;
    QString message;

    // See http://www.kernel.org/doc/Documentation/md.txt for state documentation
    allowedStatuses << "active"
                    << "active-idle"
                    << "clean"
                    << "write-pending"
                    << "read-auto";

    QList<RaidDev> devs = scanner->get_devices();
    QList<RaidDev> errDevs;

    foreach (RaidDev dev, devs) {
        if (allowedStatuses.contains(dev.state)) {
            message = QString("Device: %1 - %2").arg(dev.name).arg(dev.state);
        } else {
            errDevs.append(dev);
        }
        qDebug() << message;
        messages.append(message);
    }

    if (errDevs.count()) {
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
