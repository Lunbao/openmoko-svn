
#include <moko-stock.h>
#include <moko-finger-scroll.h>
#include <libtaku/taku-table.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <glib/gi18n.h>
#include "today-task-manager.h"
#include "today-utils.h"

#define DEFAULT_WINDOW_ICON_NAME "gnome-fs-executable"

/* NOTE: Lots of this code taken from windowselector applet in
 * 	 matchbox-panel-2. As such, this only works with matchbox as the
 *	 window-manager (due to custom window atoms).
 */

enum {
        _MB_APP_WINDOW_LIST_STACKING,
        _MB_CURRENT_APP_WINDOW,
        UTF8_STRING,
        _NET_WM_VISIBLE_NAME,
        _NET_WM_NAME,
        _NET_ACTIVE_WINDOW,
        _NET_WM_ICON,
        N_ATOMS
};

static gboolean hidden = TRUE;
static Atom atoms[N_ATOMS];

static GdkFilterReturn
filter_func (GdkXEvent *xevent, GdkEvent *event, TodayData *data);

/* Retrieves the UTF-8 property @atom from @window */
static char *
get_utf8_property (TodayData *data, Window window, Atom atom)
{
        GdkDisplay *display;
        Atom type;
        int format, result;
        gulong nitems, bytes_after;
        guchar *val;
        char *ret;

        display = gtk_widget_get_display (GTK_WIDGET (data->tasks_table));

        type = None;
        val = NULL;
        
        gdk_error_trap_push ();
        result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
                                     window,
                                     atom,
                                     0,
                                     G_MAXLONG,
                                     False,
                                     atoms[UTF8_STRING],
                                     &type,
                                     &format,
                                     &nitems,
                                     &bytes_after,
                                     (gpointer) &val);  
        if (gdk_error_trap_pop () || result != Success)
                return NULL;
  
        if (type != atoms[UTF8_STRING] || format != 8 || nitems == 0) {
                if (val)
                        XFree (val);

                return NULL;
        }

        if (!g_utf8_validate ((char *) val, nitems, NULL)) {
                g_warning ("Invalid UTF-8 in window title");

                XFree (val);

                return NULL;
        }
        
        ret = g_strndup ((char *) val, nitems);
  
        XFree (val);
  
        return ret;
}

/* Retrieves the text property @atom from @window */
static char *
get_text_property (TodayData *data, Window window, Atom atom)
{
        GdkDisplay *display;
        XTextProperty text;
        char *ret, **list;
        int result, count;

        display = gtk_widget_get_display (GTK_WIDGET (data->tasks_table));

        gdk_error_trap_push ();
        result = XGetTextProperty (GDK_DISPLAY_XDISPLAY (display),
                                   window,
                                   &text,
                                   atom);
        if (gdk_error_trap_pop () || result == 0)
                return NULL;

        count = gdk_text_property_to_utf8_list
                        (gdk_x11_xatom_to_atom (text.encoding),
                         text.format,
                         text.value,
                         text.nitems,
                         &list);
        if (count > 0) {
                int i;

                ret = list[0];

                for (i = 1; i < count; i++)
                        g_free (list[i]);
                g_free (list);
        } else
                ret = NULL;

        if (text.value)
                XFree (text.value);
  
        return ret;
}

/* Retrieves the name for @window */
static char *
window_get_name (TodayData *data, Window window)
{
        char *name;
  
        name = get_utf8_property (data,
                                  window,
                                  atoms[_NET_WM_VISIBLE_NAME]);
        if (name == NULL) {
                name = get_utf8_property (data,
                                          window,
                                          atoms[_NET_WM_NAME]);
        } if (name == NULL) {
                name = get_text_property (data,
                                          window,
                                          XA_WM_NAME);
        } if (name == NULL) {
                name = g_strdup (_("(untitled)"));
        }

        return name;
}

/* Retrieves the icon for @window */
static GdkPixbuf *
window_get_icon (TodayData *tdata, Window window)
{
        GdkPixbuf *pixbuf;
        GdkDisplay *display;
        Atom type;
        int format, result;
        int ideal_width, ideal_height, ideal_size;
        int best_width, best_height, best_size;
        int i, npixels, ip;
        gulong nitems, bytes_after, *data, *datap, *best_data;
        GtkSettings *settings;
        guchar *pixdata;

        /* First, we read the contents of the _NET_WM_ICON property */
        display = gtk_widget_get_display (GTK_WIDGET (tdata->tasks_table));

        type = 0;

        gdk_error_trap_push ();
        result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
                                     window,
                                     atoms[_NET_WM_ICON],
                                     0,
                                     G_MAXLONG,
                                     False,
                                     XA_CARDINAL,
                                     &type,
                                     &format,
                                     &nitems,
                                     &bytes_after,
                                     (gpointer) &data);
        if (gdk_error_trap_pop () || result != Success)
                return NULL;

        if (type != XA_CARDINAL || nitems < 3) {
                XFree (data);

                return NULL;
        }

        /* Got it. Now what size icon are we looking for? */
        settings = gtk_widget_get_settings (GTK_WIDGET (tdata->tasks_table));
        gtk_icon_size_lookup_for_settings (settings,
                                           GTK_ICON_SIZE_MENU,
                                           &ideal_width,
                                           &ideal_height);

        ideal_size = (ideal_width + ideal_height) / 2;

        /* Try to find the closest match */
        best_data = NULL;
        best_width = best_height = best_size = 0;

        datap = data;
        while (nitems > 0) {
                int cur_width, cur_height, cur_size;
                gboolean replace;

                if (nitems < 3)
                        break;

                cur_width = datap[0];
                cur_height = datap[1];
                cur_size = (cur_width + cur_height) / 2;

                if (nitems < (2 + cur_width * cur_height))
                        break;

                if (!best_data) {
                        replace = TRUE;
                } else {
                        /* Always prefer bigger to smaller */
                        if (best_size < ideal_size &&
                            cur_size > best_size)
                                replace = TRUE;
                        /* Prefer smaller bigger */
                        else if (best_size > ideal_size &&
                                 cur_size >= ideal_size && 
                                 cur_size < best_size)
                                replace = TRUE;
                        else
                                replace = FALSE;
                }

                if (replace) {
                        best_data = datap + 2;
                        best_width = cur_width;
                        best_height = cur_height;
                        best_size = cur_size;
                }

                datap += (2 + cur_width * cur_height);
                nitems -= (2 + cur_width * cur_height);
        }

        if (!best_data) {
                XFree (data);

                return NULL;
        }

        /* Got it. Load it into a pixbuf. */
        npixels = best_width * best_height;
        pixdata = g_new (guchar, npixels * 4);
        
        for (i = 0, ip = 0; i < npixels; i++) {
                /* red */
                pixdata[ip] = (best_data[i] >> 16) & 0xff;
                ip++;

                /* green */
                pixdata[ip] = (best_data[i] >> 8) & 0xff;
                ip++;

                /* blue */
                pixdata[ip] = best_data[i] & 0xff;
                ip++;

                /* alpha */
                pixdata[ip] = best_data[i] >> 24;
                ip++;
        }

        pixbuf = gdk_pixbuf_new_from_data (pixdata,
                                           GDK_COLORSPACE_RGB,
                                           TRUE,
                                           8,
                                           best_width,
                                           best_height,
                                           best_width * 4,
                                           (GdkPixbufDestroyNotify) g_free,
                                           NULL);

        /* Scale if necessary */
        if (best_width != ideal_width &&
            best_height != ideal_height) {
                GdkPixbuf *scaled;

                scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                  ideal_width,
                                                  ideal_height,
                                                  GDK_INTERP_BILINEAR);
                g_object_unref (pixbuf);

                pixbuf = scaled;
        }

        /* Cleanup */
        XFree (data);
  
        /* Return */
        return pixbuf;
}

static void
today_task_manager_free_tasks (TodayData *data)
{
	GList *c, *children;

	/* Free window list */
	children = gtk_container_get_children (
		GTK_CONTAINER (data->tasks_table));
	
	for (c = children; c; c = c->next) {
		GtkWidget *child = GTK_WIDGET (c->data);
		if (TAKU_IS_TILE (child))
			gtk_container_remove (GTK_CONTAINER (
				data->tasks_table), child);
	}
}

static void
today_task_manager_populate_tasks (TodayData *data)
{
        GdkDisplay *display;
        Atom type;
        int format, result, i;
        gulong nitems, bytes_after;
        Window *windows;

	/* Empty menu */
	today_task_manager_free_tasks (data);
	
        /* Retrieve list of app windows from root window */
        display = gtk_widget_get_display (GTK_WIDGET (data->tasks_table));

        type = None;

        gdk_error_trap_push ();
        result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
                                     GDK_WINDOW_XWINDOW (data->root_window),
                                     atoms[_MB_APP_WINDOW_LIST_STACKING],
                                     0,
                                     G_MAXLONG,
                                     False,
                                     XA_WINDOW,
                                     &type,
                                     &format,
                                     &nitems,
                                     &bytes_after,
                                     (gpointer) &windows);
        if (gdk_error_trap_pop () || result != Success)
                return;

        if (type != XA_WINDOW) {
                XFree (windows);

                return;
        }

        /* Load into menu */
        for (i = 0; i < nitems; i++) {
                char *name;
		GtkWidget *task_tile;
		GdkPixbuf *icon;

                name = window_get_name (data, windows[i]);
                task_tile = taku_icon_tile_new ();
		taku_icon_tile_set_primary (TAKU_ICON_TILE (task_tile), name);
		taku_icon_tile_set_secondary (TAKU_ICON_TILE (task_tile), "");
                g_free (name);

		icon = window_get_icon (data, windows[i]);
		if (icon) {
			taku_icon_tile_set_pixbuf (
				TAKU_ICON_TILE (task_tile), icon);
			g_object_unref (icon);
		} else {
			taku_icon_tile_set_icon_name (
				TAKU_ICON_TILE (task_tile),
				DEFAULT_WINDOW_ICON_NAME);
		}
		
                g_object_set_data (G_OBJECT (task_tile),
                                   "window",
                                   GUINT_TO_POINTER (windows[i]));

                /*g_signal_connect (task_tile,
                                  "activate",
                                  G_CALLBACK (window_menu_item_activate_cb),
                                  applet);*/

		gtk_container_add (GTK_CONTAINER (data->tasks_table),
			task_tile);
                gtk_widget_show (task_tile);
        }

        /* If no windows were found, insert an insensitive "No tasks" item */
        /*if (nitems == 0) {
                GtkWidget *menu_item;
                
                menu_item = gtk_menu_item_new_with_label (_("No tasks"));

                gtk_widget_set_sensitive (menu_item, FALSE);

                gtk_menu_shell_prepend (GTK_MENU_SHELL (applet->menu),
                                        menu_item);
                gtk_widget_show (menu_item);
        }*/

        /* Cleanup */
        XFree (windows);
}

static void
today_task_manager_notify_visible_cb (GObject *gobject,
				      GParamSpec *arg1,
				      TodayData *data)
{
	if ((!hidden) && (!GTK_WIDGET_VISIBLE (gobject))) {
		hidden = TRUE;
		today_task_manager_free_tasks (data);
	}
}

static gboolean
today_task_manager_visibility_notify_event_cb (GtkWidget *widget,
					       GdkEventVisibility *event,
					       TodayData *data)
{
	if (((event->state == GDK_VISIBILITY_PARTIAL) ||
	     (event->state == GDK_VISIBILITY_UNOBSCURED)) && (hidden)) {
		hidden = FALSE;
		today_task_manager_populate_tasks (data);
	} else if ((event->state == GDK_VISIBILITY_FULLY_OBSCURED) &&
		   (!hidden)) {
		hidden = TRUE;
		today_task_manager_free_tasks (data);
	}
	
	return FALSE;
}

static void
today_task_manager_unmap_cb (GtkWidget *widget, TodayData *data)
{
	if (!hidden) {
		hidden = TRUE;
		today_task_manager_free_tasks (data);
	}
}

static void
screen_changed_cb (GtkWidget *button, GdkScreen *old_screen, TodayData *data)
{
        GdkScreen *screen;
        GdkDisplay *display;
        GdkEventMask events;

        if (data->root_window) {
                gdk_window_remove_filter (data->root_window,
                                          (GdkFilterFunc) filter_func,
                                          data);
        }

        screen = gtk_widget_get_screen (data->tasks_table);
        display = gdk_screen_get_display (screen);

        /* Get atoms */
        atoms[_MB_APP_WINDOW_LIST_STACKING] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_MB_APP_WINDOW_LIST_STACKING");
        atoms[_MB_CURRENT_APP_WINDOW] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_MB_CURRENT_APP_WINDOW");
        atoms[UTF8_STRING] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "UTF8_STRING");
        atoms[_NET_WM_NAME] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_NET_WM_NAME");
        atoms[_NET_WM_VISIBLE_NAME] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_NET_WM_VISIBLE_NAME");
        atoms[_NET_WM_ICON] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_NET_WM_ICON");
        atoms[_NET_ACTIVE_WINDOW] =
                gdk_x11_get_xatom_by_name_for_display
                        (display, "_NET_ACTIVE_WINDOW");
        
        /* Get root window */
        data->root_window = gdk_screen_get_root_window (screen);

        /* Watch _MB_APP_WINDOW_LIST_STACKING */
        events = gdk_window_get_events (data->root_window);
        if ((events & GDK_PROPERTY_CHANGE_MASK) == 0) {
                gdk_window_set_events (data->root_window,
                                       events & GDK_PROPERTY_CHANGE_MASK);
        }
        
        gdk_window_add_filter (data->root_window,
                               (GdkFilterFunc) filter_func,
                               data);
        
        /* Rebuild list if around */
	if (!hidden) today_task_manager_populate_tasks (data);
}

/* Something happened on the root window */
static GdkFilterReturn
filter_func (GdkXEvent *xevent, GdkEvent *event, TodayData *data)
{
        XEvent *xev;

        xev = (XEvent *) xevent;

        if (xev->type == PropertyNotify) {
                if (xev->xproperty.atom ==
                    atoms[_MB_APP_WINDOW_LIST_STACKING]) {
                        /* _MB_APP_WINDOW_LIST_STACKING changed.
                         * Rebuild menu if around. */
                        if (!hidden)
                                today_task_manager_populate_tasks (data);
                }
        }

        return GDK_FILTER_CONTINUE;
}

GtkWidget *
today_task_manager_page_create (TodayData *data)
{
	GtkWidget *vbox, *toolbar, *viewport, *scroll;
	GtkToolItem *button;
	
	vbox = gtk_vbox_new (FALSE, 0);
	
	/* Create toolbar */
	toolbar = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

	/* Kill all apps button */
	button = today_toolbutton_new (MOKO_STOCK_FOLDER_DELETE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, 0);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		gtk_separator_tool_item_new (), 0);

	/* Kill app button */
	button = today_toolbutton_new (GTK_STOCK_DELETE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, 0);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		gtk_separator_tool_item_new (), 0);

	/* Switch to app button */
	button = today_toolbutton_new (GTK_STOCK_JUMP_TO);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, 0);

	/* Viewport / tasks table */
	viewport = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
				      GTK_SHADOW_NONE);
	
	data->tasks_table = taku_table_new ();
	gtk_container_add (GTK_CONTAINER (viewport), data->tasks_table);
	gtk_widget_show (data->tasks_table);

	scroll = moko_finger_scroll_new ();
	gtk_container_add (GTK_CONTAINER (scroll), viewport);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
	gtk_widget_show (viewport);
	gtk_widget_show (scroll);
	
	gtk_widget_show_all (toolbar);
	
	data->root_window = NULL;
	
	gtk_widget_add_events (viewport, GDK_VISIBILITY_NOTIFY_MASK);
	g_signal_connect (G_OBJECT (viewport), "visibility-notify-event",
		G_CALLBACK (today_task_manager_visibility_notify_event_cb),
		data);
	g_signal_connect (G_OBJECT (data->tasks_table), "notify::visible",
		G_CALLBACK (today_task_manager_notify_visible_cb), data);
        g_signal_connect (G_OBJECT (data->tasks_table), "screen-changed",
		G_CALLBACK (screen_changed_cb), data);
	g_signal_connect (G_OBJECT (vbox), "unmap",
		G_CALLBACK (today_task_manager_unmap_cb), data);
	
	return vbox;
}