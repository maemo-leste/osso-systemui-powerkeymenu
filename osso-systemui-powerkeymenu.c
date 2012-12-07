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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkevents.h>
#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>
#include <pango/pango.h>
#include <dbus/dbus.h>
#include <clockd/libtime.h>
#include <hildon/hildon.h>
#include <hildon/hildon-gtk.h>
#include <sqlite3.h>
#include <systemui.h>

#include "osso-systemui-powerkeymenu.h"
#include "xmlparser.h"

/* Globals */
system_ui_data *ui;
system_ui_callback_t power_key_menu_callback = {0,};
power_key_menu_t power_key_menu={0,};
gint state = -1;
gint power_key_menu_priority = 0;
GHashTable *model;

/* Forward declarations */
static void
powerkeymenu_init_menu(const gchar* path)
{
  GDir *dir;
  const gchar *direntry;

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

static void
powerkeymenu_button_clicked(GtkButton *button,gpointer   user_data)
{

}

static void
powerkeymenu_add_menu_entry(ezxml_t ex, void *data)
{
  HildonAppMenu *menu;
  GtkWidget *button;
  ezxml_t po;
  ezxml_t icon;
  ezxml_t keyfile;
  const char *title;
  const char *disabled_title = NULL;
  gboolean enabled = TRUE;

  menu = (HildonAppMenu*)data;

  keyfile = ezxml_child(ex, "keyfile");
  if(keyfile && keyfile->txt)
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
        if(tmpex && tmpex->txt)
        {
          disabled_title = ezxml_attr(tmpex, "name");
          if(disabled_title)
          {
            tmpex = ezxml_child(tmpex, "po");

            if(tmpex && tmpex->txt)
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
    if(po && po->txt)
      title = dgettext(po->txt, title);
  }
  else
     title = "Unknown!!!";

  // Create a button and add it to the menu
  button = hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
                                       HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                                       title,
                                       enabled?NULL:disabled_title);

  if(icon && icon->txt)
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

  powerkeymenu_init_menu(path);

  menu = HILDON_APP_MENU(hildon_app_menu_new());
  gtk_window_set_title(GTK_WINDOW(menu), "sui-powerkey-menu");

  powerkeymenu_xml_enum_menu(powerkeymenu_add_menu_entry, menu);

  return menu;
}


#if 0

static void
powerkeymenu_do_callback(int argc, system_ui_data *data)
{
  do_callback(data, &power_key_menu_callback, argc);
}


static int
powerkeymenu_open_handler(const char *interface,
                          const char *method,
                          GArray *args,
                          system_ui_data *data,
                          system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;
}

static int
powerkeymenu_close_handler(const char *interface,
                           const char *method,
                           GArray *args,
                           system_ui_data *data,
                           system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;

  powerkeymenu_do_callback(-4, ui);
  ipm_hide_window(power_key_menu.window);
  gtk_widget_destroy(power_key_menu.window);
  systemui_free_callback(&power_key_menu_callback);
}

static int
powerkeymenu_getstate_handler(const char *interface,
                              const char *method,
                              GArray *args,
                              system_ui_data *data,
                              system_ui_handler_arg *out)
{
  SYSTEMUI_DEBUG_FN;

  if(state != -1)
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

  ui = data;

  systemui_add_handler("powerkeymenu_open",
                       powerkeymenu_open_handler,
                       ui);

  systemui_add_handler("powerkeymenu_close",
                       powerkeymenu_close_handler,
                       ui);

  systemui_add_handler("powerkeymenu_getstate",
                       powerkeymenu_getstate_handler,
                       ui);

  systemui_add_handler("powerkeymenu_action",
                       powerkeymenu_action_handler,
                       ui);

  power_key_menu_priority = gconf_client_get_int(ui->gc_client,
                                                 "/system/systemui/powerkeymenu/window_priority",
                                                 NULL);

  if(power_key_menu_priority == 0)
    power_key_menu_priority = 195;

  return TRUE;
}

void plugin_close(system_ui_data *data)
{
  SYSTEMUI_DEBUG_FN;

  remove_handler("powerkeymenu_open", data);
  remove_handler("powerkeymenu_close", data);
  remove_handler("powerkeymenu_getstate", data);
  remove_handler("powerkeymenu_action", data);

  delete_event_cb();

  closelog();
}

#else
int main(int argc, char ** argv)
{
  GtkWidget *win;
  HildonAppMenu *menu;
  hildon_gtk_init(&argc, &argv);

  win = hildon_stackable_window_new ();
  menu = powerkeymenu_create_menu("/etc/systemui");

  // Show all menu items
  gtk_widget_show_all(GTK_WIDGET (menu));
  hildon_window_set_app_menu (HILDON_WINDOW (win), menu);

  //gtk_widget_realize(win);
  gtk_widget_show_all (win);
  gtk_main();
  return 0;
}
#endif
