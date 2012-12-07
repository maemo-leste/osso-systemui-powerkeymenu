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
