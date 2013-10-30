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

#include "RefreshDevices.h"

RefreshDevices::RefreshDevices(QObject *parent) :
    QThread(parent)
{
//        ped_exception_set_handler(ped_exception_handler);
}

RefreshDevices::~RefreshDevices()
{
        ped_device_free_all();
}

void RefreshDevices::run()
{
        devices.clear();

        ped_device_probe_all();
        PedDevice *lp_device = ped_device_get_next(NULL);
        while (lp_device)
        {


                Device d;
                d.model = lp_device->model;
                d.pdevice = lp_device;
                d.size = lp_device ->length;
                devices.push_back(d);

                lp_device = ped_device_get_next(lp_device);
        }

        emit complete();
}
