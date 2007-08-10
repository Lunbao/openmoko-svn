/*
 *  moko-contacts; A rework of the dialers contact list, some e-book code taken
 *  from the orignal dialer contacts.c (written by tonyguan@fic-sh.com.cn)
 *
 *  Authored by OpenedHand Ltd <info@openedhand.com>
 *
 *  Copyright (C) 2006-2007 OpenMoko Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date$) [$Author$]
 */

#include <glib.h>

#include <stdio.h>
#include <string.h>
#include <libebook/e-book.h>

#include "moko-contacts.h"

#include "dialer-defines.h"

G_DEFINE_TYPE (MokoContacts, moko_contacts, G_TYPE_OBJECT)

#define MOKO_CONTACTS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        MOKO_TYPE_CONTACTS, MokoContactsPrivate))

struct _MokoContactsPrivate
{
  EBook      *book;

  GList      *contacts;
  GList      *entries;
  GHashTable *prefixes;
};

static void
moko_contacts_get_photo (MokoContacts *contacts, MokoContact *m_contact)
{
  MokoContactsPrivate *priv;
  EContact *e_contact;
  EContactPhoto *photo;
  GError *err = NULL;
  GdkPixbufLoader *loader;
  
  g_return_if_fail (MOKO_IS_CONTACTS (contacts));
  priv = contacts->priv;
  
  if (!e_book_get_contact (priv->book, m_contact->uid, &e_contact, &err))
  {
    g_warning ("%s\n", err->message);
    m_contact->photo = gdk_pixbuf_new_from_file (PKGDATADIR"/person.png", NULL);
    g_object_ref (m_contact->photo);
    return;
  }

  photo = e_contact_get (e_contact, E_CONTACT_PHOTO);

  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, 
                           photo->data.inlined.data,
                           photo->data.inlined.length,
                           NULL);
  gdk_pixbuf_loader_close (loader, NULL);

  m_contact->photo = gdk_pixbuf_loader_get_pixbuf (loader);
  if (GDK_IS_PIXBUF (m_contact->photo))
    g_object_ref (m_contact->photo);

  g_object_unref (loader);
}

MokoContactEntry*
moko_contacts_lookup (MokoContacts *contacts, const gchar *number)
{
  MokoContactsPrivate *priv;
  MokoContactEntry *entry;

  g_return_val_if_fail (MOKO_IS_CONTACTS (contacts), NULL);
  priv = contacts->priv;
  
  entry =  g_hash_table_lookup (priv->prefixes, number);

  if (entry && !GDK_IS_PIXBUF (entry->contact->photo))
    moko_contacts_get_photo (contacts, entry->contact);

  return entry;
}

GList*
moko_contacts_fuzzy_lookup (MokoContacts *contacts, const gchar *number)
{
  return NULL;
}

/* This takes the raw number from econtact, and spits out a 'normalised' 
 * version which does not contain any ' ' or '-' charecters. The reason for
 * this is that when inputing numbers into the dialer, you cannot add these
 * characters, but you can in contacts, which means the strings will not match
 * and autocomplete will not work
 */
static gchar*
normalize (const gchar *string)
{
  gint len = strlen (string);
  gchar buf[len];
  gint i;
  gint j = 0;

  for (i = 0; i < len; i++)
  {
    gchar c = string[i];
    if (c != ' ' && c != '-')
    {
      buf[j] = c;
      j++;
    }
  }
  buf[j] = '\0';
  return g_strdup (buf);
}

/* Calbacks */
static void
moko_contacts_add_contact (MokoContacts *contacts, EContact *e_contact)
{
  MokoContactsPrivate *priv;
  MokoContact *m_contact = NULL;
  const gchar *name;
  gint         i;

  g_return_if_fail (MOKO_IS_CONTACTS (contacts));
  g_return_if_fail (E_IS_CONTACT (e_contact));
  priv = contacts->priv;

  name = e_contact_get_const (e_contact, E_CONTACT_NAME_OR_ORG);
  if (!name || (g_utf8_strlen (name, -1) <= 0))
    name = "Unknown";
    
  /* Create the contact & append to the list */
  m_contact = g_new0 (MokoContact, 1);
  m_contact->name = g_strdup (name);
  m_contact->uid = e_contact_get (e_contact, E_CONTACT_UID);
  m_contact->photo = NULL;

  priv->contacts = g_list_append (priv->contacts, m_contact);
   
  /* Now go through the numbers,creating MokoNumber for them */
  for (i = E_CONTACT_FIRST_PHONE_ID; i < E_CONTACT_LAST_PHONE_ID; i++)
  {
    MokoContactEntry  *entry;
    const gchar *phone;

    phone = e_contact_get_const (e_contact, i);
    if (phone)
    {
      entry = g_new0 (MokoContactEntry, 1);
      entry->desc = g_strdup (e_contact_field_name (i));
      entry->number = normalize (phone);
      entry->contact = m_contact;

      g_print ("%s %s\n", m_contact->name, entry->number);

      priv->entries = g_list_append (priv->entries, (gpointer)entry);
      g_hash_table_insert (priv->prefixes, 
                           g_strdup (entry->number), 
                           (gpointer)entry);
    }
  }
}

static void
on_ebook_contacts_added (EBookView    *view, 
                        GList        *c_list, 
                        MokoContacts *contacts)
{
  MokoContactsPrivate *priv;
  GList *c;

  g_return_if_fail (MOKO_IS_CONTACTS (contacts));
  priv = contacts->priv;

  for (c = c_list; c != NULL; c = c->next)
    moko_contacts_add_contact (contacts, E_CONTACT (c->data));
}

static void
on_ebook_contacts_changed (EBookView    *view,
                           GList        *c_list,
                           MokoContacts *contacts)
{
  g_print ("Contacts changed\n");
}

static void
on_ebook_contacts_removed (EBookView    *view,
                           GList        *c_list,
                           MokoContacts *contacts)
{
  g_print ("Contacts removed\n");
}

/* GObject functions */
static void
moko_contacts_dispose (GObject *object)
{
  G_OBJECT_CLASS (moko_contacts_parent_class)->dispose (object);
}

static void
moko_contacts_finalize (GObject *contacts)
{
  MokoContactsPrivate *priv;
  GList *l;
  
  g_return_if_fail (MOKO_IS_CONTACTS (contacts));
  priv = MOKO_CONTACTS (contacts)->priv;

  g_hash_table_destroy (priv->prefixes);

  for (l = priv->contacts; l != NULL; l = l->next)
  {
    MokoContact *contact = (MokoContact*)l->data;
    if (contact)
    {
      g_free (contact->uid);
      g_free (contact->name);
      if (G_IS_OBJECT (contact->photo))
        g_object_unref (contact->photo);
    }
  }
  g_list_free (priv->contacts);
  
  for (l = priv->entries; l != NULL; l = l->next)
  {
    MokoContactEntry *entry = (MokoContactEntry*)l->data;
    if (entry)
    {
      g_free (entry->desc);
      g_free (entry->number);
      entry->contact = NULL;
    }
  }
  g_list_free (priv->entries);

  G_OBJECT_CLASS (moko_contacts_parent_class)->finalize (contacts);
}


static void
moko_contacts_class_init (MokoContactsClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = moko_contacts_finalize;
  obj_class->dispose = moko_contacts_dispose;

  g_type_class_add_private (obj_class, sizeof (MokoContactsPrivate)); 
}

static void
moko_contacts_init (MokoContacts *contacts)
{
  MokoContactsPrivate *priv;
  EBook *book;
  EBookView *view;
  EBookQuery *query;
  GList *contact, *c;

  priv = contacts->priv = MOKO_CONTACTS_GET_PRIVATE (contacts);

  priv->contacts = priv->entries = NULL;
  priv->prefixes = g_hash_table_new ((GHashFunc)g_str_hash,
                                     (GEqualFunc)g_str_equal);
  
  query = e_book_query_any_field_contains ("");

  /* Open the system book and check that it is valid */
  book = priv->book = e_book_new_system_addressbook (NULL);
  if (!book)
  {
    g_warning ("Failed to create system book\n");
    return;
  }
  if (!e_book_open (book, TRUE, NULL))
  {
    g_warning ("Failed to open system book\n");
    return;
  }
  if (!e_book_get_contacts (book, query, &contact, NULL))
  {
    g_warning ("Failed to get contacts from system book\n");
    return;
  }
  
  /* Go through the contacts, creating the contact structs, and entry structs*/
  for (c = contact; c != NULL; c = c->next)
  {
    moko_contacts_add_contact (contacts, E_CONTACT (c->data));
  }

    /* We now have a list of entries that we can organise by the first 
   * AUTOCOMPLETE_N_CHARS digits 
   */
  /*for (e = priv->entries; e != NULL; e = e->next)
  {
    MokoContactEntry *entry = (MokoContactEntry*)e->data;
    gchar buf[AUTOCOMPLETE_N_CHARS+1];
    gint i;
    gint len = strlen (entry->number);
    GList *list = NULL;
  
    if (len > AUTOCOMPLETE_N_CHARS)
      len = AUTOCOMPLETE_N_CHARS;
    
    for (i =0; i < len; i++)
      buf[i] = entry->number[i];
    buf[len+1] = '\0';

    list = g_hash_table_lookup (priv->prefixes, buf);
    list = g_list_append (list, entry);
    
    g_hash_table_insert (priv->prefixes, g_strdup (buf), (gpointer)list);
  }
*/
  /* Connect to the ebookviews signals */
  if (e_book_get_book_view (book, query, NULL, 0, &view, NULL))
  {
    g_signal_connect (G_OBJECT (view), "contacts-added",
                      G_CALLBACK (on_ebook_contacts_added), (gpointer)contacts);
    g_signal_connect (G_OBJECT (view), "contacts-changed",
                    G_CALLBACK (on_ebook_contacts_changed), (gpointer)contacts);
    g_signal_connect (G_OBJECT (view), "contacts-removed",
                    G_CALLBACK (on_ebook_contacts_removed), (gpointer)contacts);
  }
}

MokoContacts*
moko_contacts_get_default (void)
{
  static MokoContacts *contacts = NULL;
  
  if (contacts == NULL)
    contacts = g_object_new (MOKO_TYPE_CONTACTS, 
                             NULL);

  return contacts;
}