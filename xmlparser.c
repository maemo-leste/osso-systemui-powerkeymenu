/**
   @file xmlparser.c

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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <syslog.h>

#include <systemui.h>

#include "ezxml.h"

#include "xmlparser.h"

menu_t menu[POWERKEYMENU_MAX_ENTRIES];
int menu_cnt;
GSList *xml = NULL;
static void
powerkeymenu_xml_free_xml(gpointer data, gpointer user_data)
{
  ezxml_free((ezxml_t)data);
}

void powerkeymenu_xml_free()
{
  g_slist_foreach(xml, powerkeymenu_xml_free_xml, NULL);
  g_slist_free(xml);
  xml = NULL;
  menu_cnt = 0;
}

static gboolean
powerkeymenu_xml_enum_menuitems(ezxml_t ex)
{
  const char *s;

  while(ex)
  {
    s = ezxml_attr(ex,"priority");

    if(!s)
    {
      SYSTEMUI_ERROR("Invalid or missing 'priority' attribute");
      return FALSE;
    }

    menu[menu_cnt].ex = ex;
    menu[menu_cnt].prio = atol(s);

    menu_cnt++;

    if(menu_cnt>=POWERKEYMENU_MAX_ENTRIES)
    {
      SYSTEMUI_ERROR("Too many menu entries, maximum supported - %d",
                     POWERKEYMENU_MAX_ENTRIES);
      return FALSE;
    }

    ex=ex->next;
  }

  return TRUE;
}

gboolean
powerkeymenu_xml_parse_config_file(const gchar* xmlfile)
{
  gboolean rv = FALSE;
  ezxml_t ex;
  ezxml_t iter;
  const char *s = NULL;

  ex = ezxml_parse_file(xmlfile);

  if(*ezxml_error(ex))
    SYSTEMUI_WARNING("Error while parsing %s - %s", xmlfile, ezxml_error(ex));

  iter = ex;
  while(iter)
  {
    if(!(s = ezxml_attr(iter, "path")) || strcmp(s, "/"))
    {
      SYSTEMUI_ERROR("Invalid or missing 'path' attribute");
      goto out;
    }

    if(!powerkeymenu_xml_enum_menuitems(ezxml_child(iter, "menuitem")))
      break;

    iter=iter->next;
  }

  rv = TRUE;

out:
  xml = g_slist_append(xml, ex);
  return rv;
}

static int
powerkeymenu_xml_menu_compare(const void * a, const void * b)
{
  return ((menu_t*)a)->prio - ((menu_t*)b)->prio;
}

void
powerkeymenu_xml_sort_menu()
{
  qsort (menu, menu_cnt, sizeof(menu_t), powerkeymenu_xml_menu_compare);
}

void
powerkeymenu_xml_enum_menu(void (*callback)(ezxml_t ex, void *data), void *data)
{
  int i;
  for( i=0; i<menu_cnt; i++)
    callback(menu[i].ex, data);
}


