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
