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

#ifndef REFRESHDEVICES_H
#define REFRESHDEVICES_H

#include <QtCore>
#include <QThread>
#include <parted/parted.h>
#include "Device.h"

class RefreshDevices : public QThread
{
                Q_OBJECT
        public:
                explicit RefreshDevices(QObject *parent = 0);
                virtual ~RefreshDevices();

                QList<Device> devices;

        signals:
                void complete();

        public slots:
                void run();

};

#endif // REFRESHDEVICES_H
