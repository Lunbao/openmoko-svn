/*
 *  Copyright (C) 2006-2007 OpenMoko Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Public License for more details.
 *
 *  Author: OpenedHand Ltd <info@openedhand.com>
 */

#include <string.h>

#include "search-bar.h"

#include "appmanager-data.h"
#include "navigation-area.h"
#include "package-store.h"

#include <libopkg/opkg.h>

static gboolean
combo_seperator_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gchar *s;
  gtk_tree_model_get (model, iter, 0, &s, -1);
  return !s;
}

static void
text_changed_cb (MokoSearchBar *searchbar, GtkEditable *editable, ApplicationManagerData *data)
{
  GtkTreeModel *filter;
  GtkEntry *entry;
  const gchar *needle;
  
  entry = moko_search_bar_get_entry (MOKO_SEARCH_BAR (data->searchbar));
  needle = gtk_entry_get_text (entry);
 
  data->searchbar_search_type = SEARCH_ON_NAME;
  data->searchbar_needle = needle;

  g_debug ("Search = %s", needle);

  GTimer *timer = g_timer_new ();
  filter = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tvpkglist));
  g_object_ref (G_OBJECT (filter));
  gtk_tree_view_set_model (GTK_TREE_VIEW (data->tvpkglist), NULL);

  g_timer_stop (timer);
  g_debug ("Unsetting model took %f seconds", g_timer_elapsed (timer, NULL));
  g_timer_start (timer);

  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter));

  g_timer_stop (timer);
  g_debug ("Search took %f seconds", g_timer_elapsed (timer, NULL));
  g_timer_start (timer);

  gtk_tree_view_set_model (GTK_TREE_VIEW (data->tvpkglist), filter);

  g_timer_stop (timer);
  g_debug ("Setting model took %f seconds", g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);

  g_object_unref (G_OBJECT (filter));
}

static void
combo_changed_cb (MokoSearchBar *searchbar, GtkComboBox *combo, ApplicationManagerData *data)
{
  GtkTreeModel *filter;
  const gchar *needle;
  GtkTreeIter cb_iter;
  GtkTreeModel *cb_model;
  gint active;

  if (!combo)
    combo = moko_search_bar_get_combo_box (MOKO_SEARCH_BAR (data->searchbar));

  active = gtk_combo_box_get_active (combo);
    
  if (active < 0)
    return;
  
  if (active <= 1)
  {
    data->searchbar_search_type = SEARCH_ON_STATUS;
    if (active == FILTER_INSTALLED)
      data->searchbar_needle = GINT_TO_POINTER (1);
    else
      data->searchbar_needle = GINT_TO_POINTER (0);
  }
  else
  {
    
    if (!gtk_combo_box_get_active_iter (combo, &cb_iter))
      return;

    cb_model = gtk_combo_box_get_model (combo);
    gtk_tree_model_get (cb_model, &cb_iter, 0, &needle, -1);
    
    data->searchbar_search_type = SEARCH_ON_SECTION;
    data->searchbar_needle = needle;
  }

  filter = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tvpkglist));
  g_object_ref (filter);
  gtk_tree_view_set_model (GTK_TREE_VIEW (data->tvpkglist), NULL);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter));
  gtk_tree_view_set_model (GTK_TREE_VIEW (data->tvpkglist), filter);
  g_object_unref (filter);
}

static void
searchbar_toggled_cb (MokoSearchBar *searchbar, gboolean search, ApplicationManagerData *data)
{  
  if (search)
    text_changed_cb (searchbar, NULL, data);
  else
    combo_changed_cb (searchbar, NULL, data);
}
/*
gboolean
section_search_hash (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GHashTable *hash)
{
  opkg_package_t *pkg;

  gtk_tree_model_get (model, iter, COL_POINTER, &pkg, -1);

  if (!g_hash_table_lookup (hash, pkg->section))
  {
    g_hash_table_insert (hash, pkg->section, pkg->section);
    g_debug ("hash table insert %s", pkg->section);
  }

  return FALSE;
}

void
section_hash_insert (gchar *key, gchar *value, GtkListStore *list)
{
  gtk_list_store_insert_with_values (list, NULL, -1, 0, key, -1);
}

gboolean
section_search_slist (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GSList **head)
{
  opkg_package_t *pkg;

  gtk_tree_model_get (model, iter, COL_POINTER, &pkg, -1);

  if (!g_slist_find_custom (*head, pkg->section, (GCompareFunc) strcmp))
  {
    *head = g_slist_prepend(*head, pkg->section);
    g_debug ("slist table insert %s", pkg->section);
  }

  return FALSE;
}
*/
void
slist_insert (gchar *value, GtkListStore *list)
{
  gtk_list_store_insert_with_values (list, NULL, -1, 0, value, -1);
}

GtkWidget *
search_bar_new (ApplicationManagerData *appdata, GtkTreeModel *pkg_list)
{
  GtkWidget *combo;
  GtkWidget *searchbar;
  GtkListStore *filter;
  GtkCellRenderer *renderer;
  /* GHashTable *hash; */
  /* GSList *slist = NULL; */

  filter = gtk_list_store_new (1, G_TYPE_STRING);
  appdata->filter_store = GTK_TREE_MODEL (filter);

  gtk_list_store_insert_with_values (filter, NULL, FILTER_INSTALLED, 0, "Installed", -1);
  gtk_list_store_insert_with_values (filter, NULL, FILTER_NOT_INSTALLED, 0, "Not Installed", -1);
  gtk_list_store_insert_with_values (filter, NULL, 3, 0, NULL, -1);

  /* profile these two methods to see which is quicker */
#if 0
  hash = g_hash_table_new (g_str_hash, g_str_equal);
  gtk_tree_model_foreach (pkg_list, (GtkTreeModelForeachFunc) section_search_hash, hash);
  g_hash_table_foreach (hash, (GHFunc) section_hash_insert, filter);
  g_hash_table_unref (hash);
#endif
 /* 
  gtk_tree_model_foreach (pkg_list, (GtkTreeModelForeachFunc) section_search_slist, &slist);
  slist = g_slist_sort (slist, (GCompareFunc) strcmp);
  g_slist_foreach (slist, (GFunc) slist_insert, filter);
  g_slist_free (slist);
*/
  
  renderer = gtk_cell_renderer_text_new ();
  
  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (filter));
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), combo_seperator_func, NULL, NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);
  
  searchbar = moko_search_bar_new_with_combo (GTK_COMBO_BOX (combo));
  g_signal_connect (searchbar, "combo-changed", G_CALLBACK (combo_changed_cb), appdata);
  g_signal_connect (searchbar, "text-changed", G_CALLBACK (text_changed_cb), appdata);
  g_signal_connect (searchbar, "toggled", G_CALLBACK (searchbar_toggled_cb), appdata);

  appdata->searchbar = searchbar;
  
  return searchbar;
}


void
search_bar_add_filter_item (ApplicationManagerData *appdata, gchar *item)
{
  gtk_list_store_insert_with_values (GTK_LIST_STORE (appdata->filter_store), NULL, -1, 0, item, -1);
}

void
search_bar_set_active_filter (MokoSearchBar *bar, SearchBarFilter filter)
{
  GtkComboBox *combo;
  
  combo = moko_search_bar_get_combo_box (bar);
  gtk_combo_box_set_active (combo, filter);
}
