/**
   @file osso-systemui-powerkeymenu.c

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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <syslog.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>
#include <dbus/dbus.h>
#include <hildon/hildon.h>
#include <hildon/hildon-gtk.h>
#include <systemui.h>

#include "osso-systemui-powerkeymenu.h"
#include "xmlparser.h"

/* #define POWERKEYMENU_STANDALONE */

/* Globals */
powerkeymenu_t pkmenu = {
  .window = NULL,
  .menu = NULL,
  .ui = NULL,
  .callback = {0,},
  .window_priority = 0,
  .state = -1
};


/* Forward declarations */
static void
powerkeymenu_do_callback(int argc, system_ui_data *data);

/* Code :) */

static Window
powerkeymenu_get_current_app_window (void)
{
  unsigned long n;
  unsigned long extra;
  int format;
  int status;

  Atom atom_current_app_window = gdk_x11_get_xatom_by_name ("_MB_CURRENT_APP_WINDOW");
  Atom realType;
  Window win_result = None;
  guchar *data_return = NULL;

  status = XGetWindowProperty (GDK_DISPLAY(), GDK_ROOT_WINDOW (),
                               atom_current_app_window, 0L, 16L,
                               0, XA_WINDOW, &realType, &format,
                               &n, &extra,
                               &data_return);

  if (status == Success && realType == XA_WINDOW && format == 32 && n == 1 && data_return != NULL)
  {
    win_result = ((Window*) data_return)[0];
  }

  if (data_return)
    XFree (data_return);

  return win_result;
}

static gchar*
powerkeymenu_get_window_class (Window w)
{
  Atom r_type;
  int  r_fmt;
  unsigned long n_items;
  unsigned long r_after;
  unsigned char *r_prop;
  gchar *rv = NULL;

  if (Success == XGetWindowProperty (GDK_DISPLAY (), w, XA_WM_CLASS,
                                     0, 8192,
                                     False, XA_STRING,
                                     &r_type, &r_fmt, &n_items, &r_after,
                                     (unsigned char **)&r_prop) && r_type != 0)
  {
    if (r_prop)
      {
        /*
         * The property contains two strings separated by \0; we want the
         * second string.
         */
        gint len0 = strlen ((char*)r_prop);
        if (len0 == n_items)
          len0--;

        rv = g_strdup ((char*)r_prop + len0 + 1);

        XFree (r_prop);
      }
  }

  return rv;
}

static gboolean
powerkeymenu_is_desktop_active (void)
{
  Window active_window;
  gboolean rv = TRUE;

  gdk_error_trap_push();
  active_window = powerkeymenu_get_current_app_window();

  if(active_window)
  {
    gchar *wm_class = powerkeymenu_get_window_class(active_window);

    if(wm_class)
    {
      if(!strstr(wm_class,"desktop"))
        rv = FALSE;

      g_free(wm_class);
    }
  }

  gdk_flush();
  gdk_error_trap_pop();
  return rv;
}

static void
powerkeymenu_close_topmost_window(void)
{
  Window active_window;
  gboolean desktop = TRUE;

  gdk_error_trap_push();
  active_window = powerkeymenu_get_current_app_window();

  if(active_window)
  {
    gchar *wm_class = powerkeymenu_get_window_class(active_window);

    if(wm_class)
    {
      if(!strstr(wm_class,"desktop"))
        desktop = FALSE;

      g_free(wm_class);
    }
  }

  if(!desktop)
  {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));

    ev.xclient.type         = ClientMessage;
    ev.xclient.window       = active_window;
    ev.xclient.message_type = gdk_x11_get_xatom_by_name ("_NET_CLOSE_WINDOW");
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = CurrentTime;
    ev.xclient.data.l[1]    = GDK_WINDOW_XID (gdk_get_default_root_window ());

    XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False, SubstructureRedirectMask, &ev);
    XSync(GDK_DISPLAY(), False);
  }

  gdk_flush();
  gdk_error_trap_pop();
}

static void
powerkeymenu_init_menu(const gchar* path)
{
  GDir *dir;
  const gchar *direntry;

  SYSTEMUI_DEBUG_FN;

  powerkeymenu_xml_free();

  dir = g_dir_open(path, 0, NULL);

  if(!dir)
  {
    SYSTEMUI_ERROR("failed to open config dir %s", path);
    return;
  }

  while((direntry = g_dir_read_name(dir)))
  {
    if(g_str_has_suffix(direntry,".xml"))
    {
      gchar *filename = g_build_filename(path, direntry, NULL);
      powerkeymenu_xml_parse_config_file(filename);
      g_free(filename);
    }
  }

  g_dir_close(dir);

  powerkeymenu_xml_sort_menu();
}

static void*
powerkeymenu_dbus_message_get_arg(ezxml_t ezarg, system_ui_handler_arg *val)
{
  const char *s = ezxml_attr(ezarg, "type");

  val->arg_type = DBUS_TYPE_INVALID;
  if(!s)
  {
    return &val->data;
  }

  if(!strcmp(s,"string"))
  {
    val->arg_type = DBUS_TYPE_STRING;
    val->data.str = ezarg->txt;
    return &val->data.str;
  }
  else if(!strcmp(s,"byte"))
  {
    val->arg_type = DBUS_TYPE_BYTE;
    val->data.byt = strtol(ezarg->txt, NULL, 10);
    return &val->data.byt;
  }
  else if(!strcmp(s,"boolean"))
  {
    val->arg_type = DBUS_TYPE_BOOLEAN;
    if(!strcmp(ezarg->txt, "true"))
      val->data.bool_val = TRUE;
    else
      val->data.bool_val = FALSE;
    return &val->data.bool_val;
  }
  else if(!strcmp(s,"int16"))
  {
    val->arg_type = DBUS_TYPE_INT16;
    val->data.i16 = strtol(ezarg->txt, NULL, 10);
    return &val->data.i16;
  }
  else if(!strcmp(s,"uint16"))
  {
    val->arg_type = DBUS_TYPE_UINT16;
    val->data.u16 = strtoul(ezarg->txt, NULL, 10);
    return &val->data.u16;
  }
  else if(!strcmp(s,"int32"))
  {
    val->arg_type = DBUS_TYPE_INT32;
    val->data.i32 = strtol(ezarg->txt, NULL, 10);
    return &val->data.i32;
  }
  else if(!strcmp(s,"uint32"))
  {
    val->arg_type = DBUS_TYPE_UINT32;
    val->data.u32 = strtoul(ezarg->txt, NULL, 10);
    return &val->data.u32;
  }
  else if(!strcmp(s,"int64"))
  {
    val->arg_type = DBUS_TYPE_INT64;
    val->data.i64 = strtoll(ezarg->txt, NULL, 10);
    return &val->data.i64;
  }
  else if(!strcmp(s,"uint64"))
  {
    val->arg_type = DBUS_TYPE_UINT64;
    val->data.u64 = strtoull(ezarg->txt, NULL, 10);
    return &val->data.u64;
  }
  else if(!strcmp(s,"double"))
  {
    val->arg_type = DBUS_TYPE_DOUBLE;
    val->data.dbl = strtod(ezarg->txt, NULL);
    return &val->data.dbl;
  }

  return &val->data;
}

static void
powerkeymenu_dbus_message_append_args(DBusMessage *msg, ezxml_t ezargs)
{
  void *val;
  system_ui_handler_arg tmp;

  while(ezargs)
  {
    val = powerkeymenu_dbus_message_get_arg(ezargs, &tmp);
    if(tmp.arg_type != DBUS_TYPE_INVALID)
      dbus_message_append_args(msg, tmp.arg_type, val, DBUS_TYPE_INVALID);
    ezargs = ezargs->next;
  }
}

static void
powerkeymenu_button_clicked(GtkButton *button,gpointer data)
{
  ezxml_t ex=(ezxml_t)data;
  ezxml_t callback;
  ezxml_t retval;

  SYSTEMUI_DEBUG_FN;

  g_return_if_fail(ex != NULL);

  retval = ezxml_child(ex, "return");
  callback = ezxml_child(ex, "callback");

  if(retval)
  {
    switch(atol(retval->txt))
    {
      case 8:
        hildon_banner_show_information(NULL, NULL,
                                       dgettext("osso-powerup-shutdown",
                                                "powerup_ib_silent_activated"));
        break;
      case 9:
        hildon_banner_show_information(NULL, NULL,
                                       dgettext("osso-powerup-shutdown",
                                                "powerup_ib_general_activated"));
        break;
    }

    /* FIXME - do we really need to skip mce callback? */
    if(!callback)
    {
      if(atol(retval->txt) == 10)
        powerkeymenu_close_topmost_window();
#if !defined(POWERKEYMENU_STANDALONE)
      else
        powerkeymenu_do_callback(atol(retval->txt), pkmenu.ui);
#endif
    }
  }

  if(callback)
  {
    const gchar *service = ezxml_attr(callback, "service");
    const gchar *path = ezxml_attr(callback, "path");
    const gchar *interface = ezxml_attr(callback, "interface");
    const gchar *method = ezxml_attr(callback, "method");
    const gchar *signal = ezxml_attr(callback, "signal");
    const gchar *bus = ezxml_attr(callback, "bus");
    const gchar *autostart = ezxml_attr(callback, "autostart");
    DBusMessage *msg = NULL;

    if(path && interface && bus )
    {
      if(service && method)
        msg = dbus_message_new_method_call(service, path, interface, method);
      else if(signal)
        msg = dbus_message_new_signal(path, interface, signal);
    }

    if(!msg)
      SYSTEMUI_WARNING("Invalid callback parameters");
    else
    {
      DBusConnection *connection;

      if(autostart && !(strcmp(autostart, "yes")))
        dbus_message_set_auto_start(msg, TRUE);
      else
        dbus_message_set_auto_start(msg, FALSE);

      dbus_message_set_no_reply(msg, TRUE);
      powerkeymenu_dbus_message_append_args(msg, ezxml_child(callback, "argument"));
      connection = dbus_bus_get(!strcmp(bus, "session")?DBUS_BUS_SESSION:DBUS_BUS_SYSTEM, NULL);

      if(connection)
        dbus_connection_send(connection, msg, NULL);
      else
        SYSTEMUI_WARNING("connecting to %s bus failed", bus);

      dbus_message_unref(msg);
      if(connection)
      {
        dbus_connection_flush(connection);
        dbus_connection_unref(connection);
      }
    }
  }
}

static void
powerkeymenu_add_menu_entry(ezxml_t ex, void *data)
{
  HildonAppMenu *menu;
  GtkWidget *button;
  ezxml_t po;
  ezxml_t icon;
  ezxml_t keyfile;
  ezxml_t retval;
  const char *title;
  const char *disabled_title = NULL;
  gboolean enabled = TRUE;

  SYSTEMUI_DEBUG_FN;

  menu = (HildonAppMenu*)data;

  retval = ezxml_child(ex, "return");
  if(retval && atol(retval->txt) == 10)
  {
    /* Do not add 'End current task' if the active window is desktop */
    if(powerkeymenu_is_desktop_active())
      return;
  }

  keyfile = ezxml_child(ex, "keyfile");
  if(keyfile)
  {
    ezxml_t tmpex;
    const char *visible;
    const char *disabled;
    char buf[32];
    int fd;
    int len;
    gboolean continue_processing = TRUE;

    visible = ezxml_attr(ex, "visible");
    disabled = ezxml_attr(ex, "disabled");

    buf[0] = 0;
    fd = open(keyfile->txt, O_RDONLY, O_NONBLOCK);

    if(fd == -1)
    {
      SYSTEMUI_WARNING("Unable do open keyfile %s", keyfile->txt);
      return;
    }

    if((len = read(fd, buf, sizeof(buf))) >= 0)
    {
      buf[len] = 0;
      len--;
      while(len>=0 && isspace(buf[len]))
      {
        buf[len] = 0;
        len--;
      }

      if(visible && strcmp(buf, visible))
        continue_processing = FALSE;

      if(disabled && !strcmp(buf, disabled))
      {
        tmpex = ezxml_child(ex, "disabled_reason");
        if(tmpex)
        {
          disabled_title = ezxml_attr(tmpex, "name");
          if(disabled_title)
          {
            tmpex = ezxml_child(tmpex, "po");

            if(tmpex)
              disabled_title = dgettext(tmpex->txt, disabled_title);
          }
        }

        enabled = FALSE;
      }
    }
    else
    {
      SYSTEMUI_WARNING("Unable do read from keyfile %s", keyfile->txt);
      continue_processing = FALSE;
    }

    close(fd);

    if(!continue_processing)
      return;
  }

  po = ezxml_child(ex, "po");
  title = ezxml_attr(ex, "name");
  icon = ezxml_child(ex, "icon");

  if(title)
  {
    if(po)
      title = dgettext(po->txt, title);
  }
  else
     title = "Unknown!!!";

  // Create a button and add it to the menu
  button = hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
                                       enabled?HILDON_BUTTON_ARRANGEMENT_HORIZONTAL:HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                                       title,
                                       enabled?NULL:disabled_title);

  if(icon)
  {
    GtkWidget *image = NULL;
    if(g_str_has_prefix(icon->txt, "/"))
    {
      image = gtk_image_new_from_file(icon->txt);
    }
    else
    {
      GtkIconTheme *theme = gtk_icon_theme_get_default();
      GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(theme, icon->txt, 48,
                                                   GTK_ICON_LOOKUP_NO_SVG, NULL);
      if(pixbuf)
      {
        image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
      }
    }

    if(image)
      hildon_button_set_image(HILDON_BUTTON(button), image);
    else
      SYSTEMUI_ERROR("Failed to load icon %s", icon->txt);
  }
  g_signal_connect_after (button,
                          "clicked",
                          G_CALLBACK (powerkeymenu_button_clicked),
                          ex);

  if(!enabled)
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

  hildon_app_menu_append (menu, GTK_BUTTON (button));

}

static HildonAppMenu*
powerkeymenu_create_menu(const gchar *path)
{
  HildonAppMenu *menu;

  SYSTEMUI_DEBUG_FN;

  powerkeymenu_init_menu(path);

  menu = HILDON_APP_MENU(hildon_app_menu_new());
  gtk_window_set_title(GTK_WINDOW(menu), "sui-powerkey-menu");

  powerkeymenu_xml_enum_menu(powerkeymenu_add_menu_entry, menu);

  return menu;
}

static void
powerkeymenu_destroy_menu()
{
  if(pkmenu.window)
  {
#if !defined(POWERKEYMENU_STANDALONE)
    ipm_hide_window(pkmenu.window);
    powerkeymenu_do_callback(-6, pkmenu.ui);
#endif
    gtk_widget_destroy(pkmenu.window);
    pkmenu.window = NULL;
    g_object_unref(pkmenu.menu);
    pkmenu.menu = NULL;
    powerkeymenu_xml_free();
  }
}

static gboolean
power_key_menu_unmap_event_cb(GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   user_data)
{
  SYSTEMUI_DEBUG_FN;

  powerkeymenu_destroy_menu();

  return FALSE;
}

#if 0
static gboolean
power_key_menu_delete_event_cb(GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   user_data)
{
  SYSTEMUI_DEBUG_FN;

  return FALSE;
}

static gboolean
power_key_menu_key_press_event_cb(GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   user_data)
{
  SYSTEMUI_DEBUG_FN;

  return FALSE;
}
#endif

static void
powerkeymenu_show_menu()
{
  SYSTEMUI_DEBUG_FN;

  if(!pkmenu.window)
  {
    pkmenu.window = gtk_window_new(GTK_WINDOW_POPUP);

    pkmenu.menu = powerkeymenu_create_menu("/etc/systemui");


    gtk_widget_show_all(GTK_WIDGET (pkmenu.menu));
#if !defined(POWERKEYMENU_STANDALONE)
    ipm_show_window(GTK_WIDGET(pkmenu.menu), pkmenu.window_priority);
#endif
    hildon_app_menu_popup(pkmenu.menu,GTK_WINDOW(pkmenu.window));

    g_signal_connect_after(pkmenu.menu,
                           "unmap-event",
                           (GCallback)power_key_menu_unmap_event_cb,
                           NULL);
#if 0
    g_signal_connect_after(pkmenu.menu,
                           "delete-event",
                           (GCallback)power_key_menu_delete_event_cb,
                           NULL);
    g_signal_connect_after(pkmenu.menu,
                           "key-press-event",
                           (GCallback)power_key_menu_key_press_event_cb,
                           NULL);
#endif
  }
}

#if !defined(POWERKEYMENU_STANDALONE)

static void
powerkeymenu_do_callback(int argc, system_ui_data *data)
{
  do_callback(data, &pkmenu.callback, argc);
}



static int
powerkeymenu_open_handler(const char *interface,
                          const char *method,
                          GArray *args,
                          system_ui_data *data,
                          system_ui_handler_arg *out)
{
  int supported_args[1] = {'u'};
  SYSTEMUI_DEBUG_FN;

  if(!check_plugin_arguments(args, supported_args, 1))
    return FALSE;

  if(!pkmenu.window)
  {
    powerkeymenu_show_menu();
    pkmenu.state = 1;
  }

  out->arg_type = 'i';

  if(check_set_callback(args, &pkmenu.callback))
    out->data.i32 = -3;
  else
    out->data.i32 = -2;

  return 'i';

}

static int
powerkeymenu_close_handler(const char *interface,
                           const char *method,
                           GArray *args,
                           system_ui_data *data,
                           system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;

  powerkeymenu_do_callback(-4, pkmenu.ui);
  if(pkmenu.window)
  {
    ipm_hide_window(GTK_WIDGET(pkmenu.window));
    gtk_widget_destroy(GTK_WIDGET(pkmenu.window));
    pkmenu.window = NULL;
    g_object_unref(pkmenu.menu);
    pkmenu.menu = NULL;
    powerkeymenu_xml_free();
  }

  systemui_free_callback(&pkmenu.callback);

  return 'v';
}

#if 0
static int
powerkeymenu_getstate_handler(const char *interface,
                              const char *method,
                              GArray *args,
                              system_ui_data *data,
                              system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;

  if(pkmenu.state != -1)
    out->data.bool_val = TRUE;
  else
    out->data.bool_val = FALSE;

  out->arg_type = 'b';

  return 'b';

}

static int
powerkeymenu_action_handler(const char *interface,
                            const char *method,
                            GArray *args,
                            system_ui_data *data,
                            system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;

  out->arg_type = 'b';
  out->data.bool_val = FALSE;

  return 'b';
}
#endif

gboolean
plugin_init(system_ui_data *data)
{
  openlog("systemui-powerkeymenu", LOG_ALERT | LOG_USER, LOG_NDELAY);

  SYSTEMUI_DEBUG_FN;

  if( !data )
  {
    SYSTEMUI_ERROR("initialization parameter value is invalid");
    return FALSE;
  }

  pkmenu.ui = data;

  systemui_add_handler("powerkeymenu_open",
                       powerkeymenu_open_handler,
                       pkmenu.ui);

  systemui_add_handler("powerkeymenu_close",
                       powerkeymenu_close_handler,
                       pkmenu.ui);

#if 0
  systemui_add_handler("powerkeymenu_getstate",
                       powerkeymenu_getstate_handler,
                       ui);

  systemui_add_handler("powerkeymenu_action",
                       powerkeymenu_action_handler,
                       ui);
#endif

  pkmenu.window_priority = gconf_client_get_int(pkmenu.ui->gc_client,
                                                "/system/systemui/powerkeymenu/window_priority",
                                                0);
  if(pkmenu.window_priority == 0)
    pkmenu.window_priority = 195;

  return TRUE;
}

void plugin_close(system_ui_data *data)
{
  SYSTEMUI_DEBUG_FN;

  remove_handler("powerkeymenu_open", data);
  remove_handler("powerkeymenu_close", data);
#if 0
  remove_handler("powerkeymenu_getstate", data);
  remove_handler("powerkeymenu_action", data);
#endif

  closelog();
}

#else
int main(int argc, char ** argv)
{
  hildon_gtk_init(&argc, &argv);

  powerkeymenu_show_menu();
  gtk_main();
  return 0;
}
#endif
