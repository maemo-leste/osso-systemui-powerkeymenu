all: libsystemuiplugin_power_key_menu.so

clean:
	$(RM) libsystemuiplugin_power_key_menu.so

install: libsystemuiplugin_power_key_menu.so
	install -d $(DESTDIR)/usr/lib/systemui
	install -m 644 libsystemuiplugin_power_key_menu.so $(DESTDIR)/usr/lib/systemui

libsystemuiplugin_power_key_menu.so: osso-systemui-powerkeymenu.c xmlparser.c ezxml/ezxml.c 
	$(CC) $^ -o $@ -shared -Wall $(CFLAGS) $(LDFLAGS) -I./ezxml $(shell pkg-config --libs --cflags osso-systemui libxml-2.0 hildon-1 gconf-2.0 alarm libnotify gtk+-2.0 dbus-1 glib-2.0 sqlite3) -ltime -lhildon-plugins-notify-sv -L/usr/lib/hildon-desktop -Wl,-soname -Wl,$@ -Wl,-rpath -Wl,/usr/lib/hildon-desktop

.PHONY: all clean install
