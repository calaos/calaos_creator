/******************************************************************************
**  Copyright (c) 2006-2012, Calaos. All Rights Reserved.
**
**  This file is part of Calaos Home.
**
**  Calaos Home is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 3 of the License, or
**  (at your option) any later version.
**
**  Calaos Home is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Calaos; if not, write to the Free Software
**  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**
******************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include "RefreshDevices.h"

namespace Ui {
class MainWindow;
}

const qint64 KIBIBYTE=1024;
const qint64 MEBIBYTE=(KIBIBYTE * KIBIBYTE);
const qint64 GIBIBYTE=(MEBIBYTE * KIBIBYTE);
const qint64 TEBIBYTE=(GIBIBYTE * KIBIBYTE);

enum { UNIT_SECTOR, UNIT_BYTE, UNIT_KIB, UNIT_MIB, UNIT_GIB, UNIT_TIB };

class MainWindow : public QMainWindow
{
                Q_OBJECT

        public:
                explicit MainWindow(QWidget *parent = 0);
                ~MainWindow();

                static QString format_size(qint64 sectors, qint64 sector_size);
                static double sector_to_unit(qint64 sectors, qint64 sector_size, int size_unit);

        protected:
                void changeEvent(QEvent *e);

        private slots:
                void on_buttonRefresh_clicked();
                void refreshDone();

                void on_buttonChoose_clicked();

                void tarListing(int exitCode, QProcess::ExitStatus exitStatus);
                void tarExtractVersion(int exitCode, QProcess::ExitStatus exitStatus);
                void createExit(int exitCode, QProcess::ExitStatus exitStatus);
                void createProgress();
                void createError(QProcess::ProcessError e);

                void on_addButton_clicked();

                void on_delButton_clicked();

                void on_modifyButton_clicked();

                void on_treeProperties_itemDoubleClicked(QTreeWidgetItem *item, int column);

                void on_treeProperties_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

                void on_buttonCreate_clicked();

        private:
                Ui::MainWindow *ui;

                RefreshDevices *refreshThread;
                QProcess *tarProcess;
                QProcess *createProcess;

                int stateGUI;
                enum { GUI_FW_NOK, GUI_FW_OK };

                QDir tempDir;
                QString version;

                QMap<QString, QString> local_config;
                QTreeWidgetItem *current_item;

                QString error_message;

                void disableGUI();
                void enableGUI();
                void updateLocalConfig();
};

#endif // MAINWINDOW_H
