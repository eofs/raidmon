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

#ifndef RAIDMON_H
#define RAIDMON_H

#include <QDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QStringList>
#include <QSettings>

namespace Ui {
    class RaidMon;
}

class RaidMon : public QDialog {
    Q_OBJECT
public:
    RaidMon(QWidget *parent = 0);
    ~RaidMon();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::RaidMon *ui;

    void setupExtra();
    void createActions();
    void createTrayIcon();
    void loadSettings();
    void saveSettings();

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    bool statusClean;
    QStringList devices;
    int updateInternval;
    bool useKDENotifications;
    QTimer *timer;

    // Actions
    QAction *quitAction;
    QAction *aboutAction;
    QAction *settingsAction;

public slots:
    void readRaidStatus();
    void setTrayIconStatus(bool hasErrors, QString status, QStringList messages);
    void showAbout();
    void accept();
    void reject();

signals:
    void status(bool hasErrors, QString status, QStringList messages);
};

#endif // RAIDMON_H
