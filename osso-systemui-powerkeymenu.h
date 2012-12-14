/**
   @file osso-systemui-powerkeymenu.h

   @brief Maemo systemui power key menu plugin

   Copyright (C) 2012 Ivaylo Dimitrov <freemangordon@abv.bg>

   This file is part of osso-systemui-powerkeymenu.

   osso-systemui-powerkeymenu is free software;
   you can redistribute it and/or modify it under the terms of the
   GNU Lesser General Public License version 2.1 as published by the
   Free Software Foundation.

   osso-systemui-powerkeymenu is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with osso-systemui-powerkeymenu.
   If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __OSSO_SYSTEMUI_POWERKEYMENU_H_INCLUDED__
#define __OSSO_SYSTEMUI_POWERKEYMENU_H_INCLUDED__
#include <mce/dbus-names.h>
#include <mce/mode-names.h>
#include <systemui.h>

typedef struct {
  GtkWidget *window;
  HildonAppMenu *menu;
  system_ui_data *ui;
  system_ui_callback_t callback;
  gint window_priority;
  gint state;
}powerkeymenu_t;

#endif /* __OSSO_SYSTEMUI_POWERKEYMENU_H_INCLUDED__ */
