
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "config.h"
#include "today.h"
#include "today-header-box.h"

static GtkToolItem *
today_toolbutton_new (const gchar *icon_name)
{
	GtkWidget *icon = gtk_image_new_from_icon_name (icon_name,
		GTK_ICON_SIZE_DIALOG);
	GtkToolItem *button = gtk_tool_button_new (icon, NULL);
	gtk_tool_item_set_expand (button, TRUE);
	return button;
}

static void
today_notebook_add_page_with_icon (GtkWidget *notebook, GtkWidget *child,
				   const gchar *icon_name, int padding)
{
	GtkWidget *icon = gtk_image_new_from_icon_name (icon_name,
		GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *align = gtk_alignment_new (0.5, 0.5, 1, 1);

	gtk_widget_show (icon);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), padding,
		padding, padding, padding);
	gtk_container_add (GTK_CONTAINER (align), icon);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), child, align);
	gtk_container_child_set (GTK_CONTAINER (notebook), child,
		"tab-expand", TRUE, NULL);
}

int
main (int argc, char **argv)
{
	TodayData data;
	GOptionContext *context;
	GtkWidget *viewport, *align, *vbox;
	GtkWidget *placeholder;
	
	static GOptionEntry entries[] = {
		{ NULL }
	};

	/* Initialise */
	bindtextdomain (GETTEXT_PACKAGE, TODAY_LOCALE_DIR);;
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (_(" - Today's information summary"));
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);

	/* Create widgets */
	data.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (data.window), _("Today"));
	data.vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (data.window), data.vbox);
	
	/* Toolbar */
	data.toolbar = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (data.vbox), data.toolbar, FALSE, FALSE, 0);

	data.dates_button = today_toolbutton_new ("dates");
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar), data.dates_button, 0);
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar),
		gtk_separator_tool_item_new (), 0);
	data.messages_button = today_toolbutton_new ("openmoko-messages");
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar), data.messages_button, 0);
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar),
		gtk_separator_tool_item_new (), 0);
	data.contacts_button = today_toolbutton_new ("contacts");
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar), data.contacts_button, 0);
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar),
		gtk_separator_tool_item_new (), 0);
	data.dial_button = today_toolbutton_new ("openmoko-dialer");
	gtk_toolbar_insert (GTK_TOOLBAR (data.toolbar), data.dial_button, 0);
	
	/* Notebook */
	data.notebook = gtk_notebook_new ();
	g_object_set (G_OBJECT (data.notebook), "can-focus", FALSE, NULL);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (data.notebook), GTK_POS_BOTTOM);
	gtk_box_pack_end (GTK_BOX (data.vbox), data.notebook, TRUE, TRUE, 0);
	
	/* Add home page */
	viewport = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
				      GTK_SHADOW_NONE);
	align = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 6, 6, 6);
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (align), vbox);
	gtk_container_add (GTK_CONTAINER (viewport), align);

	data.message_box = today_header_box_new_with_markup (
		"<b>Provider goes here</b>");
	gtk_box_pack_start (GTK_BOX (vbox), data.message_box, FALSE, TRUE, 0);

	data.events_box = today_header_box_new_with_markup (
		"<b>The date</b>");
	gtk_box_pack_start (GTK_BOX (vbox), data.events_box, FALSE, TRUE, 0);
	
	today_notebook_add_page_with_icon (data.notebook, viewport,
		GTK_STOCK_HOME, 6);
	
	/* Add new tasks page */
	placeholder = gtk_label_new ("New tasks");
	today_notebook_add_page_with_icon (data.notebook, placeholder,
		GTK_STOCK_ADD, 6);

	/* Add running tasks page */
	placeholder = gtk_label_new ("Running tasks");
	today_notebook_add_page_with_icon (data.notebook, placeholder,
		GTK_STOCK_EXECUTE, 6);
	
	/* Connect up signals */
	g_signal_connect (G_OBJECT (data.window), "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);

#if 1
	/* Force theme settings */
	g_object_set (gtk_settings_get_default (),
		"gtk-theme-name", "openmoko-standard-2", 
		"gtk-icon-theme-name", "openmoko-standard",
		NULL);
	gtk_window_set_default_size (GTK_WINDOW (data.window), 448, 640);
#endif

	
	/* Show and start */
	gtk_widget_show_all (data.window);
	gtk_main ();

	return 0;
}