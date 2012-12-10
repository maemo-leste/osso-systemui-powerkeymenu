/**
   @file xmlparser.h

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

#ifndef __OSSO_SYSTEMUI_POWERKEYMENU_XMLPARSER_H_INCLUDED__
#define __OSSO_SYSTEMUI_POWERKEYMENU_XMLPARSER_H_INCLUDED__

#include "ezxml.h"

typedef enum {
  POWERKEYMENU_NODE_TYPE_UNKNOWN,
  POWERKEYMENU_NODE_TYPE_POWERKEYMENU,
  POWERKEYMENU_NODE_TYPE_MENUITEM,
  POWERKEYMENU_NODE_TYPE_RETURN,
  POWERKEYMENU_NODE_TYPE_PO,
  POWERKEYMENU_NODE_TYPE_KEYFILE,
  POWERKEYMENU_NODE_TYPE_ICON,
  POWERKEYMENU_NODE_TYPE_TITLE,
  POWERKEYMENU_NODE_TYPE_DISABLED_REASON,
  POWERKEYMENU_NODE_TYPE_CALLBACK,
  POWERKEYMENU_NODE_TYPE_ARGUMENT,
  POWERKEYMENU_NODE_TYPE_LAST
}POWERKEYMENU_NODE_TYPE;

gboolean
powerkeymenu_xml_parse_config_file(const gchar* xmlfile);

void
powerkeymenu_xml_sort_menu();

void
powerkeymenu_xml_enum_menu(void (*callback)(ezxml_t ex, void *data), void *data);

void
powerkeymenu_xml_free();

typedef struct{
  ezxml_t ex;
  int prio;
}menu_t;

/* Bigger value makes no sense */
#define POWERKEYMENU_MAX_ENTRIES 32

#endif /* __OSSO_SYSTEMUI_POWERKEYMENU_XMLPARSER_H_INCLUDED__ */
