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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <iomanip>
#include <sstream>

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        refreshThread(NULL),
        tarProcess(NULL),
        createProcess(NULL),
        stateGUI(GUI_FW_NOK)
{
        ui->setupUi(this);

        QTimer::singleShot(100, this, SLOT(on_buttonRefresh_clicked()));

        ui->progressBar->setEnabled(false);
        ui->buttonCreate->setEnabled(false);

        /* Search and create a new temp dir */
        tempDir.setPath(QDir::tempPath());
        int cpt = 0;

        do
        {
                tempDir.setPath(QDir::tempPath() + "/calaos_cf_" + QString::number(cpt));
                cpt++;
        } while (tempDir.exists());

        tempDir.mkdir(tempDir.path());

        QStringList headers;
        headers << QString::fromUtf8("Propriétés") << QString::fromUtf8("Valeur");
        ui->treeProperties->setHeaderLabels(headers);

        local_config["fw_target"] = "calaos_tss";
        local_config["show_cursor"] = "false";
        local_config["use_ntp"] = "true";
        local_config["evas_engine"] = "software";
        local_config["touchscreen_driver"] = "XORG_ELOTOUCH";
        local_config["device_type"] = "calaos_server";
        local_config["dpms_enable"] = "true";
        local_config["dpms_standby"] = "600";
        local_config["dpms_standby_min"] = "10";
        local_config["dpms_standby_max"] = "1800";
        local_config["dpms_block"] = "false";
        local_config["eth0_dhcp"] = "false";
        local_config["eth0_address"] = "192.168.1.100";
        local_config["eth0_broadcast"] = "192.168.1.255";
        local_config["eth0_netmask"] = "255.255.255.0";
        local_config["eth0_gateway"] = "192.168.1.1";
        local_config["eth1_address"] = "10.0.0.200";
        local_config["eth1_broadcast"] = "10.0.0.255";
        local_config["eth1_netmask"] = "255.255.255.0";
        local_config["eth1_gateway"] = "";
        local_config["dns_address"] = "192.168.1.1";
        local_config["calaos_user"] = "user";
        local_config["calaos_password"] = "pass";
        local_config["ssh_enable"] = "true";

        updateLocalConfig();
}

MainWindow::~MainWindow()
{
        tempDir.remove("VERSION");
        tempDir.remove("local_config.xml");
        tempDir.remove("create_cf.sh");
        tempDir.rmdir(tempDir.path());

        delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
        QMainWindow::changeEvent(e);
        switch (e->type()) {
        case QEvent::LanguageChange:
                ui->retranslateUi(this);
                break;
        default:
                break;
        }
}

QString MainWindow::format_size(qint64 sectors, qint64 sector_size)
{
        std::stringstream s;
        s << std::setiosflags(std::ios::fixed) << std::setprecision(2);

        if ((sectors * sector_size) < KIBIBYTE)
        {
                s << sector_to_unit(sectors, sector_size, UNIT_BYTE);
                return QString("%1 B").arg(s.str().c_str());
        }
        else if ((sectors * sector_size) < MEBIBYTE)
        {
                s << sector_to_unit(sectors, sector_size, UNIT_KIB);
                return QString("%1 KiB").arg(s.str().c_str());
        }
        else if ((sectors * sector_size) < GIBIBYTE)
        {
                s << sector_to_unit(sectors, sector_size, UNIT_MIB);
                return QString("%1 MiB").arg(s.str().c_str());
        }
        else if ((sectors * sector_size) < TEBIBYTE)
        {
                s << sector_to_unit(sectors, sector_size, UNIT_GIB);
                return QString("%1 TiB").arg(s.str().c_str());
        }

        return "";
}

double MainWindow::sector_to_unit(qint64 sectors, qint64 sector_size, int size_unit)
{
        switch (size_unit)
        {
        case UNIT_BYTE: return sectors * sector_size;
        case UNIT_KIB: return sectors / (static_cast<double>(KIBIBYTE) / sector_size);
        case UNIT_MIB: return sectors / (static_cast<double>(MEBIBYTE) / sector_size);
        case UNIT_GIB: return sectors / (static_cast<double>(GIBIBYTE) / sector_size);
        case UNIT_TIB: return sectors / (static_cast<double>(TEBIBYTE) / sector_size);
        default: return sectors;
        }
}

void MainWindow::enableGUI()
{
        ui->groupBox1->setEnabled(true);
        ui->groupBox2->setEnabled(true);
        ui->groupBox3->setEnabled(true);

        if (stateGUI == GUI_FW_OK)
        {
//                ui->progressBar->setEnabled(true);
                ui->buttonCreate->setEnabled(true);
        }
        else
        {
//                ui->progressBar->setEnabled(false);
                ui->buttonCreate->setEnabled(false);
        }
}

void MainWindow::disableGUI()
{
        ui->groupBox1->setEnabled(false);
        ui->groupBox2->setEnabled(false);
        ui->groupBox3->setEnabled(false);
//        ui->progressBar->setEnabled(false);
        ui->buttonCreate->setEnabled(false);
}

void MainWindow::on_buttonRefresh_clicked()
{
        disableGUI();

        refreshThread = new RefreshDevices(this);
        connect(refreshThread, SIGNAL(complete()), this, SLOT(refreshDone()));
        refreshThread->start();
}

void MainWindow::refreshDone()
{
        enableGUI();

        ui->comboDrive->clear();
        for (int i = 0;i < refreshThread->devices.size();i++)
        {
                Device dev = refreshThread->devices.at(i);
                
                //Device must be at least 512Mb, and don't show devices with size greater than 10Gb
                //It prevent to list computer's own disk drive and mess up with the system
                if ((dev.pdevice->length * dev.pdevice->sector_size) > 10 * GIBIBYTE)
                        continue;
                if ((dev.pdevice->length * dev.pdevice->sector_size) < 475 * MEBIBYTE)
                        continue;

                QString it = dev.pdevice->path;
                it += " - " + dev.model + " (" + format_size(dev.pdevice->length, dev.pdevice->sector_size) + ")";
                ui->comboDrive->addItem(it, QVariant(QString(dev.pdevice->path)));
        }

        if (refreshThread)
        {
                delete refreshThread;
                refreshThread = NULL;
        }
}

void MainWindow::on_buttonChoose_clicked()
{
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setNameFilter(tr("Firmwares (*.img)"));
        dialog.setViewMode(QFileDialog::Detail);

        if (dialog.exec())
        {
                QStringList fnames = dialog.selectedFiles();
                if (fnames.size() != 1) return;
                ui->lineEditFW->setText(fnames.at(0));

                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                disableGUI();

                QString program = "tar";
                QStringList arguments;
                arguments << "-jtf" << ui->lineEditFW->text();

                tarProcess = new QProcess(this);
                connect (tarProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(tarListing(int,QProcess::ExitStatus)));
                tarProcess->start(program, arguments);
        }
}

void MainWindow::tarListing(int exitCode, QProcess::ExitStatus exitStatus)
{
        if (!tarProcess)
        {
                ui->iconFW->setPixmap(QPixmap(":/data/dialog-cancel.png"));
                stateGUI = GUI_FW_NOK;

                QApplication::restoreOverrideCursor();
                enableGUI();

                return;
        }

        QByteArray bytes = tarProcess->readAllStandardOutput();
        QStringList lines = QString(bytes).split("\n");
        if (lines.contains("rootfs") &&
            lines.contains("vmlinuz") &&
            lines.contains("VERSION") &&
            lines.contains("BUILD") &&
            lines.contains("syslinux.cfg"))
        {
                ui->iconFW->setPixmap(QPixmap(":/data/dialog-ok.png"));
                ui->iconFW->setScaledContents(true);
                stateGUI = GUI_FW_OK;

                QString program = "tar";
                QStringList arguments;
                arguments << "-C" << tempDir.path() << "-xjf" << ui->lineEditFW->text() << "VERSION";

                delete tarProcess;
                tarProcess = new QProcess(this);
                connect (tarProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(tarExtractVersion(int,QProcess::ExitStatus)));
                tarProcess->start(program, arguments);

                return;
        }
        else
        {
                ui->iconFW->setPixmap(QPixmap(":/data/dialog-cancel.png"));
                ui->iconFW->setScaledContents(true);
                stateGUI = GUI_FW_NOK;
        }

        delete tarProcess;
        tarProcess = NULL;

        QApplication::restoreOverrideCursor();
        enableGUI();
}

void MainWindow::tarExtractVersion(int exitCode, QProcess::ExitStatus exitStatus)
{
        delete tarProcess;
        tarProcess = NULL;

        QApplication::restoreOverrideCursor();
        enableGUI();

        QFile file(tempDir.path() + "/VERSION");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
                ui->iconFW->setPixmap(QPixmap(":/data/dialog-cancel.png"));
                ui->iconFW->setScaledContents(true);
                stateGUI = GUI_FW_NOK;
        }

        version = file.readAll();
        version = version.trimmed();
        local_config["fw_version"] = version;

        updateLocalConfig();
}

void MainWindow::updateLocalConfig()
{
        ui->treeProperties->clear();

        QMapIterator<QString, QString> i(local_config);
        bool first = true;
        while (i.hasNext())
        {
                i.next();

                QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeProperties);
                item->setData(0, Qt::DisplayRole, i.key());
                item->setData(1, Qt::DisplayRole, i.value());
                item->setData(0, Qt::DecorationRole, QIcon(":/data/document-properties.png"));

                if (first) item->setSelected(true);
                first = false;
        }

        ui->treeProperties->resizeColumnToContents(0);
        ui->treeProperties->resizeColumnToContents(1);
}

void MainWindow::on_addButton_clicked()
{
        bool ok;
        QString text = QInputDialog::getText(this, QString::fromUtf8("Nouveau paramètre"),
                                          QString::fromUtf8("Entrez un nom de paramètre"), QLineEdit::Normal,
                                          QString(), &ok);

        if (ok && !text.isEmpty())
        {
                QString key = text;

                if (local_config.contains(text))
                {
                        QMessageBox::warning(this, tr("Calaos CF"), QString::fromUtf8("Ce paramètre existe déjà !"));

                        return;
                }

                local_config[key] = "";

                QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeProperties);
                item->setData(0, Qt::DisplayRole, key);
                item->setData(1, Qt::DisplayRole, QString());
                item->setData(0, Qt::DecorationRole, QIcon(":/data/document-properties.png"));
                ui->treeProperties->setCurrentItem(item);

                on_modifyButton_clicked();
        }
}

void MainWindow::on_delButton_clicked()
{
        if (current_item)
        {
                QString key = current_item->text(0);

                local_config.remove(key);
                delete current_item;
        }
}

void MainWindow::on_modifyButton_clicked()
{
        if (!current_item) return;

        QString key, value;

        key = current_item->text(0);
        value = local_config[key];

        bool ok;
        QString text = QInputDialog::getText(this, QString::fromUtf8("Modifier la valeur"),
                                          QString::fromUtf8("Modifier le paramètre: \"%1\"").arg(key), QLineEdit::Normal,
                                          value, &ok);
        if (ok && !text.isEmpty())
        {
                local_config[key] = text;
                current_item->setData(1, Qt::DisplayRole, text);
        }
}

void MainWindow::on_treeProperties_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
        on_modifyButton_clicked();
}

void MainWindow::on_treeProperties_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
        current_item = current;
}

void MainWindow::on_buttonCreate_clicked()
{
        QString lconfig = tempDir.path() + "/local_config.xml";

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        disableGUI();

        //Create local_config.xml based on the QMap
        QFile file(lconfig);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return;

        QTextStream out(&file);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << "\n";
        out << "<calaos:config xmlns:calaos=\"http://www.calaos.fr\">" << "\n";
        QMapIterator<QString, QString> i(local_config);
        while (i.hasNext())
        {
                i.next();
                out << "<calaos:option name=\"" << i.key() << "\" value=\"" << i.value() << "\" />" << "\n";
        }
        out << "</calaos:config>";

        QFile::copy(":/create_cf.sh", tempDir.path() + "/create_cf.sh");
        QFile::setPermissions(tempDir.path() + "/create_cf.sh", QFile::ExeGroup | QFile::ExeOther | QFile::ExeOther | QFile::ExeUser);

        QString program = tempDir.path() + "/create_cf.sh";
        QStringList arguments;
        QString drive = ui->comboDrive->itemData(ui->comboDrive->currentIndex()).toString();
        if (ui->comboDrive->count() <= 0) drive.clear();
        arguments << drive << ui->lineEditFW->text() << lconfig;

        error_message.clear();
        createProcess = new QProcess(this);
        connect(createProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(createProgress()));
        connect(createProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(createExit(int,QProcess::ExitStatus)));
        connect(createProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(createError(QProcess::ProcessError)));
        createProcess->start(program, arguments);
}

void MainWindow::createExit(int exitCode, QProcess::ExitStatus exitStatus)
{
        if (exitCode != 0)
                QMessageBox::warning(this, tr("Calaos CF"), QString::fromUtf8("La création du système a echoué !\n%1").arg(error_message));
        else
                QMessageBox::information(this, tr("Calaos CF"), QString::fromUtf8("La création du système est terminé avec succès."));

        createProcess = NULL;
        delete createProcess;

        QApplication::restoreOverrideCursor();
        enableGUI();
}

void MainWindow::createProgress()
{
        if (!createProcess) return;

        QByteArray bytes = createProcess->readAllStandardOutput();
        QStringList lines = QString(bytes).split("\n");
        foreach (QString line, lines)
        {
                bool ok;
                int progress = line.toInt(&ok);
                if (ok && progress >= 0 && progress <= 100) ui->progressBar->setValue(progress);
                if (!ok) error_message += line;
        }
}

void MainWindow::createError(QProcess::ProcessError e)
{
        QApplication::restoreOverrideCursor();
        enableGUI();

        QMessageBox::warning(this, tr("Calaos CF"), QString::fromUtf8("Erreur lors du lancement du script !"));
}
