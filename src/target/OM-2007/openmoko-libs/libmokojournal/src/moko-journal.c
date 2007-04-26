/* vi: set sw=2: */
/*
 * Copyright (C) 2007 by OpenMoko, Inc.
 * Written by OpenedHand Ltd <info@openedhand.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <string.h>
#include <strings.h>
#include <glib-object.h>
#include <libical/icaltypes.h>
#include <libecal/e-cal.h>
#include <libecal/e-cal-component.h>
#include "moko-journal.h"
#include "moko-time-priv.h"

#define WIFI_AP_MAC_ADDR_LEN 48

struct _MokoJournal
{
  ECal *ecal ;
  ECalView *ecal_view ;
  GList *entries_to_delete ;
  GArray *entries ;
};

struct _MokoJournalVoiceInfo
{
  gchar *dialed_number ;
  gboolean was_missed ;
};

struct _MokoJournalFaxInfo
{
  int nothing ;
};

struct _MokoJournalDataInfo
{
  int nothing ;
};

struct _MokoJournalEmailInfo
{
  int nothing ;
};

struct _MokoJournalSMSInfo
{
  int nothing ;
};

struct _MokoJournalEntry
{
  enum MokoJournalEntryType type;
  gchar *uid ;
  gchar *contact_uid ;
  gchar *summary ;
  enum MessageDirection direction ;
  MokoTime *dtstart ;
  MokoTime *dtend ;
  float start_longitude ;
  float start_latitude ;
  gchar *source ;
  MokoGSMLocation gsm_loc ;
  guchar *wifi_ap_mac ;
  guchar *bluetooth_ap_mac ;
  union
  {
    MokoJournalEmailInfo *email_info ;
    MokoJournalVoiceInfo *voice_info ;
    MokoJournalFaxInfo *fax_info ;
    MokoJournalDataInfo *data_info ;
    MokoJournalSMSInfo *sms_info ;
  } extra ;
};

struct _MokoJournalEntryInfo
{
  enum MokoJournalEntryType type;
  const gchar *type_as_string ;
};
typedef struct _MokoJournalEntryInfo MokoJournalEntryInfo ;

static const MokoJournalEntryInfo entries_info[] =
{
  {VOICE_JOURNAL_ENTRY, "VOICEENTRY"},
  {EMAIL_JOURNAL_ENTRY, "EMAILENTRY"},
  {SMS_JOURNAL_ENTRY, "SMSENTRY"},
  {FAX_JOURNAL_ENTRY, "FAXENTRY"},
  {DATA_JOURNAL_ENTRY, "DATAENTRY"},
  {0}
} ;

static MokoJournal* moko_journal_alloc () ;
static MokoJournalEmailInfo* moko_journal_email_info_new () ;
static MokoJournalVoiceInfo* moko_journal_voice_info_new () ;
static MokoJournalFaxInfo* moko_journal_fax_info_new () ;
static MokoJournalSMSInfo* moko_journal_sms_info_new () ;
static void moko_journal_voice_info_free (MokoJournalVoiceInfo *a_info) ;
static void moko_journal_fax_info_free (MokoJournalFaxInfo *a_info) ;
static void moko_journal_data_info_free (MokoJournalDataInfo *a_info) ;
static void moko_journal_email_info_free (MokoJournalEmailInfo *a_info) ;
static void moko_journal_sms_info_free (MokoJournalSMSInfo *a_info) ;
static MokoJournalDataInfo* moko_journal_data_info_new () ;
static gboolean moko_journal_find_entry_from_uid (MokoJournal *a_journal,
                                                  const gchar *a_uid,
                                                  MokoJournalEntry **a_entry,
                                                  int *a_offset) ;
static const gchar* entry_type_to_string (enum MokoJournalEntryType a_type) ;
static enum MokoJournalEntryType entry_type_from_string (const gchar* a_str) ;
static gboolean moko_journal_entry_type_is_valid
                                          (enum MokoJournalEntryType a_type) ;
static gboolean moko_journal_entry_to_icalcomponent (MokoJournalEntry *a_entry,
                                                     icalcomponent **a_comp) ;
static gboolean icalcomponent_to_entry (icalcomponent *a_comp,
                                          MokoJournalEntry **a_entry) ;
static gboolean icalcomponent_find_property (const icalcomponent *a_comp,
                                             const gchar *a_name,
                                             icalproperty **a_property) ;

static gboolean icalcomponent_find_property_as_string
                                              (const icalcomponent *a_comp,
                                               const gchar *a_name,
                                               gchar **a_property) ;
static void on_entries_added_cb (ECalView *a_view,
                                 GList *a_entries,
                                 MokoJournal *a_journal) ;
static void on_entries_removed_cb (ECalView *a_view,
                                   GList *uids,
                                   MokoJournal *a_journal) ;


static const gchar*
entry_type_to_string (enum MokoJournalEntryType a_type)
{
  MokoJournalEntryInfo *cur ;

  for (cur = (MokoJournalEntryInfo*)entries_info ; cur ; ++cur)
  {
    if (cur->type == a_type)
      return cur->type_as_string ;
  }
  return NULL ;
}

static enum MokoJournalEntryType
entry_type_from_string (const gchar* a_str)
{
  MokoJournalEntryInfo *cur ;

  for (cur = (MokoJournalEntryInfo*)entries_info ; cur ; ++cur)
  {
    if (!strcmp (cur->type_as_string, a_str))
    {
      return cur->type ;
    }
  }
  return UNDEF_ENTRY ;
}

/*<private funcs>*/
static MokoJournal*
moko_journal_alloc ()
{
  MokoJournal *result ;
  result = g_new0 (MokoJournal, 1) ;
  result->entries = g_array_new (TRUE, TRUE, sizeof (MokoJournalEntry*)) ;
  return result ;
}

static gboolean
moko_journal_find_entry_from_uid (MokoJournal *a_journal,
                                  const gchar *a_uid,
                                  MokoJournalEntry **a_entry,
                                  int *a_offset)
{
  int i=0 ;

  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_journal->entries, FALSE) ;
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_offset, FALSE) ;
  g_return_val_if_fail (a_uid, FALSE) ;

  if (!a_journal->entries)
    return FALSE ;

  for (i = 0 ; i < a_journal->entries->len ; ++i)
  {
    if (g_array_index (a_journal->entries, MokoJournalEntry*, i)
        && g_array_index (a_journal->entries, MokoJournalEntry*, i)->uid
        && !strcmp (g_array_index (a_journal->entries,
                                   MokoJournalEntry*, i)->uid,
                    a_uid))
    {
      *a_entry = g_array_index (a_journal->entries, MokoJournalEntry*, i) ;
      *a_offset = i ;
      return TRUE ;
    }
  }
  return FALSE ;
}

static void
moko_journal_free (MokoJournal *a_journal)
{
  g_return_if_fail (a_journal) ;
  if (a_journal->ecal)
  {
    g_object_unref (G_OBJECT (a_journal->ecal)) ;
    a_journal->ecal = NULL ;
  }
  if (a_journal->entries_to_delete)
  {
    g_list_free (a_journal->entries_to_delete) ;
    a_journal->entries_to_delete = NULL ;
  }
  if (a_journal->entries)
  {
    int i ;
    for (i=0 ; i < a_journal->entries->len ; ++i)
    {
      if (g_array_index (a_journal->entries, MokoJournalEntry*, i))
      {
        moko_journal_entry_free (g_array_index (a_journal->entries,
                                                MokoJournalEntry*, i));
        g_array_index (a_journal->entries, MokoJournalEntry*, i) = NULL;
      }
    }
    g_array_free (a_journal->entries, TRUE) ;
    a_journal->entries = NULL ;
  }
  g_free (a_journal) ;
}

static MokoJournalEmailInfo*
moko_journal_email_info_new ()
{
  return g_new0 (MokoJournalEmailInfo, 1) ;
}

static MokoJournalVoiceInfo*
moko_journal_voice_info_new ()
{
  return g_new0 (MokoJournalVoiceInfo, 1) ;
}

static MokoJournalFaxInfo*
moko_journal_fax_info_new ()
{
  return g_new0 (MokoJournalFaxInfo, 1) ;
}

static MokoJournalDataInfo*
moko_journal_data_info_new ()
{
  return g_new0 (MokoJournalDataInfo, 1) ;
}

static MokoJournalSMSInfo*
moko_journal_sms_info_new ()
{
  return g_new0 (MokoJournalSMSInfo, 1) ;
}

static void
moko_journal_email_info_free (MokoJournalEmailInfo *a_info)
{
  g_return_if_fail (a_info) ;
  g_free (a_info) ;
}

static void
moko_journal_voice_info_free (MokoJournalVoiceInfo *a_info)
{
  g_return_if_fail (a_info) ;

  if (a_info)
  {
    if (a_info->dialed_number)
    {
      g_free (a_info->dialed_number) ;
      a_info->dialed_number = NULL ;
    }
  }
  g_free (a_info) ;
}

static void
moko_journal_fax_info_free (MokoJournalFaxInfo *a_info)
{
  g_return_if_fail (a_info) ;
  g_free (a_info) ;
}

static void
moko_journal_data_info_free (MokoJournalDataInfo *a_info)
{
  g_return_if_fail (a_info) ;
  g_free (a_info) ;
}

static void
moko_journal_sms_info_free (MokoJournalSMSInfo *a_info)
{
  g_return_if_fail (a_info) ;
  g_free (a_info) ;
}

static MokoJournalEntry*
moko_journal_entry_alloc ()
{
  MokoJournalEntry *result ;

  result = g_new0 (MokoJournalEntry, 1) ;
  return result ;
}

static void
moko_journal_entry_free_real (MokoJournalEntry *a_entry)
{
  g_return_if_fail (a_entry) ;

  if (a_entry->uid)
  {
    g_free (a_entry->uid) ;
    a_entry->uid = NULL ;
  }
  if (a_entry->contact_uid)
  {
    g_free (a_entry->contact_uid) ;
    a_entry->contact_uid = NULL ;
  }
  if (a_entry->summary)
  {
    g_free (a_entry->summary) ;
    a_entry->summary = NULL ;
  }
  if (a_entry->dtstart)
  {
    moko_time_free (a_entry->dtstart) ;
    a_entry->dtstart = NULL ;
  }
  if (a_entry->dtend)
  {
    moko_time_free (a_entry->dtend) ;
    a_entry->dtend = NULL ;
  }
  if (a_entry->source)
  {
    g_free (a_entry->source) ;
    a_entry->source = NULL ;
  }
  if (a_entry->wifi_ap_mac)
  {
    g_free (a_entry->wifi_ap_mac) ;
    a_entry->wifi_ap_mac = NULL ;
  }
  if (a_entry->bluetooth_ap_mac)
  {
    g_free (a_entry->bluetooth_ap_mac) ;
    a_entry->bluetooth_ap_mac = NULL;
  }

  switch (a_entry->type)
  {
    case EMAIL_JOURNAL_ENTRY:
      if (a_entry->extra.email_info)
      {
        moko_journal_email_info_free (a_entry->extra.email_info) ;
        a_entry->extra.email_info = NULL ;
      }
      break ;
    case VOICE_JOURNAL_ENTRY:
      if (a_entry->extra.voice_info)
      {
        moko_journal_voice_info_free (a_entry->extra.voice_info) ;
        a_entry->extra.voice_info = NULL ;
      }
      break ;
    case FAX_JOURNAL_ENTRY:
      if (a_entry->extra.fax_info)
      {
        moko_journal_fax_info_free (a_entry->extra.fax_info) ;
        a_entry->extra.fax_info = NULL;
      }
      break ;
    case DATA_JOURNAL_ENTRY:
      if (a_entry->extra.data_info)
      {
        moko_journal_data_info_free (a_entry->extra.data_info) ;
        a_entry->extra.data_info = NULL ;
      }
      break ;
    case SMS_JOURNAL_ENTRY:
      if (a_entry->extra.sms_info)
      {
        moko_journal_sms_info_free (a_entry->extra.sms_info) ;
        a_entry->extra.sms_info = NULL ;
      }
      break ;
    default:
      g_warning ("unknown journal entry type. This is a leak!\n") ;
      break ;
  }
  g_free (a_entry) ;
}

static gboolean
moko_journal_entry_type_is_valid (enum MokoJournalEntryType a_type)
{
  if (a_type > 0 && a_type < NB_OF_ENTRY_TYPES)
    return TRUE ;
  return FALSE ;
}

static gboolean
moko_journal_entry_to_icalcomponent (MokoJournalEntry *a_entry,
                                     icalcomponent **a_comp)
{
  icalcomponent *comp = NULL ;
  icalproperty *prop = NULL ;
  gboolean result = FALSE ;
  gchar *str=NULL ;
  enum MessageDirection dir = DIRECTION_IN ;

  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_comp, FALSE) ;
  g_return_val_if_fail (moko_journal_entry_type_is_valid (a_entry->type),
                        FALSE) ;

  comp = icalcomponent_new (ICAL_VJOURNAL_COMPONENT) ;

  /*add uid, if it exists*/
  if (a_entry->uid)
  {
    prop = icalproperty_new_uid (a_entry->uid) ;
    icalcomponent_add_property (comp, prop) ;
  }

  /*add contact prop*/
  prop = icalproperty_new_contact
                          (moko_journal_entry_get_contact_uid (a_entry)) ;
  icalcomponent_add_property (comp, prop) ;

  /*add summary prop*/
  prop = icalproperty_new_summary (moko_journal_entry_get_summary (a_entry)) ;
  icalcomponent_add_property (comp, prop) ;

  /*add direction*/
  if (!moko_journal_entry_get_direction (a_entry, &dir))
  {
    g_warning ("failed to get entry direction") ;
    goto out ;
  }
  if (dir == DIRECTION_IN)
  {
    prop = icalproperty_new_x ("IN") ;
  }
  else
  {
    prop = icalproperty_new_x ("OUT") ;
  }
  icalproperty_set_x_name (prop, "X-OPENMOKO-ENTRY-DIRECTION") ;
  icalcomponent_add_property (comp, prop) ;

  /*add dtstart*/
  const MokoTime *date = moko_journal_entry_get_dtstart (a_entry) ;
  if (!date)
    goto out ;
  prop = icalproperty_new_dtstart (date->t) ;
  icalcomponent_add_property (comp, prop) ;

  /*add location start*/
  struct icalgeotype geo;
  if (moko_journal_entry_get_start_location (a_entry, (MokoLocation*)&geo))
  {
    prop = icalproperty_new_geo (geo) ;
    icalcomponent_add_property (comp, prop) ;
  }

  /*add source*/
  if (moko_journal_entry_get_source (a_entry))
    prop = icalproperty_new_x (moko_journal_entry_get_source (a_entry)) ;
  else
    prop = icalproperty_new_x ("null") ;
  icalproperty_set_x_name (prop, "X-OPENMOKO-ENTRY-SOURCE") ;
  icalcomponent_add_property (comp, prop) ;

  /*
   * add gsm location (pair of "local area code" and "cell id".
   * gsm location is stored in an
   * x-property named X-OPENMOKO-ENTRY-GSM-LOCATION that has the form:
   * X-OPENMOKO-GSMLOCATION;LAC="<int16>";CID=<int 16>: dummy
   */
  prop = icalproperty_new_x ("dummy") ;
  icalproperty_set_x_name (prop, "X-OPENMOKO-ENTRY-GSM-LOCATION") ;
  str = g_strdup_printf ("%d", a_entry->gsm_loc.lac);
  icalproperty_set_parameter_from_string (prop, "X-LAC", str) ;
  g_free (str) ;
  str = g_strdup_printf ("%d", a_entry->gsm_loc.cid);
  icalproperty_set_parameter_from_string (prop, "X-CID", str) ;
  g_free (str) ;
  icalcomponent_add_property (comp, prop) ;
  /*add entry type*/
  prop = icalproperty_new_x
                (entry_type_to_string (moko_journal_entry_get_type (a_entry))) ;
  icalproperty_set_x_name (prop, "X-OPENMOKO-ENTRY-TYPE") ;
  icalcomponent_add_property (comp, prop) ;

  switch (moko_journal_entry_get_type (a_entry))
  {
    case UNDEF_ENTRY:
      g_warning ("entry is of undefined type\n") ;
      return FALSE ;
    case EMAIL_JOURNAL_ENTRY:
      break ;
    case VOICE_JOURNAL_ENTRY:
      {
        MokoJournalVoiceInfo *info = NULL ;
        gchar *number="NULL" ;
        gchar *missed=NULL ;

        if (!moko_journal_entry_get_voice_info (a_entry, &info))
          break ;
        if (!info)
          break ;

        /*
         * serialize dialed number
         */
        if (moko_journal_voice_info_get_dialed_number (info))
          number = (gchar*)moko_journal_voice_info_get_dialed_number (info) ;
        prop = icalproperty_new_x (number) ;
        icalproperty_set_x_name (prop, "X-OPENMOKO-VOICE-DIALED-NUMBER") ;
        icalcomponent_add_property (comp, prop) ;

        /*
         * serialize the "was-missed" property
         */
        if (moko_journal_voice_info_get_was_missed (info))
          missed = "YES" ;
        else
          missed = "NO" ;
        prop = icalproperty_new_x (missed) ;
        icalproperty_set_x_name (prop, "X-OPENMOKO-VOICE-CALL-WAS-MISSED") ;
        icalcomponent_add_property (comp, prop) ;
      }
      break ;
    case FAX_JOURNAL_ENTRY:
      break ;
    case DATA_JOURNAL_ENTRY:
      break ;
    case SMS_JOURNAL_ENTRY:
      break ;
    default:
      break ;
  }

  *a_comp = comp ;
  prop = NULL ;
  comp = NULL ;
  result = TRUE ;
out:
  if (prop)
    icalproperty_free (prop) ;

  if (comp)
    icalcomponent_free (comp) ;

  return result ;
}

static gboolean
icalcomponent_to_entry (icalcomponent *a_comp,
                        MokoJournalEntry **a_entry)
{
  icalproperty *prop=NULL ;
  gchar *prop_name=NULL, *prop_value=NULL ;
  MokoJournalEntry *entry=NULL;

  g_return_val_if_fail (a_comp, FALSE) ;
  g_return_val_if_fail (icalcomponent_isa (a_comp) == ICAL_VJOURNAL_COMPONENT,
                        FALSE) ;

  entry = moko_journal_entry_alloc () ;

  /*get the type*/
  if (icalcomponent_find_property_as_string (a_comp, "X-OPENMOKO-ENTRY-TYPE",
                                             &prop_value))
  {
    enum MokoJournalEntryType entry_type = UNDEF_ENTRY ;
    if (!prop_value)
    {
      g_warning ("could not get entry type") ;
      goto out ;
    }
    entry_type = entry_type_from_string (prop_value) ;
    if (entry_type == UNDEF_ENTRY)
    {
      g_warning ("Could not recognize type of entry from: %s\n", prop_value);
      goto out ;
    }
    entry->type = entry_type ;
  }
  /*make sure we got the type*/
  if (entry->type == UNDEF_ENTRY || entry->type >= NB_OF_ENTRY_TYPES)
  {
    g_warning ("bad entry type") ;
    goto out ;
  }

  /*look for the x-properties we may have*/
  if (icalcomponent_find_property_as_string
                                    (a_comp, "X-OPENMOKO-ENTRY-DIRECTION",
                                     &prop_value))
  {
    if (prop_value && !strcmp (prop_value, "IN"))
      moko_journal_entry_set_direction (entry, DIRECTION_IN) ;
    else
      moko_journal_entry_set_direction (entry, DIRECTION_OUT) ;
  }
  if (icalcomponent_find_property_as_string
                                    (a_comp, "X-OPENMOKO-ENTRY-SOURCE",
                                     &prop_value))
  {
    if (!prop_value || (prop_value && !strcmp (prop_value, "null")))
    {
      moko_journal_entry_set_source (entry, NULL) ;
    }
    else
    {
      moko_journal_entry_set_source (entry, prop_value) ;
    }
  }

  if (icalcomponent_find_property (a_comp,
                                   "X-OPENMOKO-ENTRY-GSM-LOCATION",
                                   &prop))
  {
    if (prop )
    {
      gchar *str=NULL ;
      gint code=0 ;
      str = (gchar*)icalproperty_get_parameter_as_string (prop, "X-LAC") ;
      code = atoi (str) ;
      entry->gsm_loc.lac = (gushort)code ;
      str = (gchar*)icalproperty_get_parameter_as_string (prop, "X-CID") ;
      code = atoi (str) ;
      entry->gsm_loc.cid = (gushort)code ;
    }
  }


  /*now iterate through properties to scan core properties*/
  for (prop = icalcomponent_get_first_property (a_comp, ICAL_ANY_PROPERTY);
       prop ;
       prop = icalcomponent_get_next_property (a_comp, ICAL_ANY_PROPERTY))
  {
    prop_name = (gchar*)icalproperty_get_property_name (prop) ;
    if (!prop_name)
      continue ;
    if (icalproperty_isa (prop) == ICAL_UID_PROPERTY)
    {
      if (entry->uid)
        g_free (entry->uid) ;
      entry->uid = g_strdup (icalproperty_get_uid (prop)) ;
    }
    else if (icalproperty_isa (prop) == ICAL_CONTACT_PROPERTY)
    {
      moko_journal_entry_set_contact_uid
                                  (entry, icalproperty_get_contact (prop)) ;
    }
    else if (icalproperty_isa (prop) == ICAL_SUMMARY_PROPERTY)
    {
      moko_journal_entry_set_summary (entry, icalproperty_get_summary (prop)) ;
    }
    else if (icalproperty_isa (prop) == ICAL_DTSTART_PROPERTY)
    {
      moko_journal_entry_set_dtstart
        (entry,
         moko_time_new_from_icaltimetype (icalproperty_get_dtstart (prop)));
    }
    else if (icalproperty_isa (prop) == ICAL_GEO_PROPERTY)
    {
      struct icalgeotype geo = icalproperty_get_geo (prop);
      moko_journal_entry_set_start_location (entry, (MokoLocation*)&geo) ;
    }
  }

  /*scan extra info related properties*/
  switch (entry->type)
  {
    case VOICE_JOURNAL_ENTRY:
      {
        MokoJournalVoiceInfo *info = NULL ;
        prop = NULL ;
        prop_value = NULL ;

        moko_journal_entry_get_voice_info (entry, &info) ;
        g_return_val_if_fail (info, FALSE) ;

        /*
         * deserialize dialed number
         */
        if (icalcomponent_find_property_as_string
                                          (a_comp,
                                           "X-OPENMOKO-VOICE-DIALED-NUMBER",
                                           &prop_value))
        {
          if (prop_value)
          {
            moko_journal_voice_info_set_dialed_number (info, prop_value) ;
          }
        }
        prop_value = NULL ;
        if (icalcomponent_find_property_as_string
                                        (a_comp,
                                         "X-OPENMOKO-VOICE-CALL-WAS-MISSED",
                                         &prop_value))
        {
          if (prop_value)
          {
            gboolean was_missed = FALSE ;
            if (!strcmp ("YES", prop_value))
              was_missed = TRUE ;
            moko_journal_voice_info_set_was_missed (info, was_missed) ;
          }
        }
      }
      break ;
    case FAX_JOURNAL_ENTRY:
      break ;
    case DATA_JOURNAL_ENTRY:
      break ;
    case SMS_JOURNAL_ENTRY:
      break ;
    case EMAIL_JOURNAL_ENTRY:
      break ;
    default:
      break ;
  }

  *a_entry = entry ;
  entry = NULL ;

out:
  if (entry)
    moko_journal_entry_free (entry) ;
  return TRUE ;
}


static gboolean
icalcomponent_find_property (const icalcomponent *a_comp,
                             const gchar *a_name,
                             icalproperty **a_property)
{
  icalproperty *prop = NULL ;

  g_return_val_if_fail (a_comp, FALSE) ;

  for (prop = icalcomponent_get_first_property ((icalcomponent*)a_comp,
                                                ICAL_ANY_PROPERTY);
       prop;
       prop = icalcomponent_get_next_property ((icalcomponent*)a_comp,
                                               ICAL_ANY_PROPERTY))
  {
    if (icalproperty_get_property_name (prop)
        && ! strcasecmp (icalproperty_get_property_name (prop), a_name))
    {
      *a_property = prop ;
      return TRUE ;
    }
  }
  return FALSE ;
}

static gboolean
icalcomponent_find_property_as_string (const icalcomponent *a_comp,
                                       const gchar *a_name,
                                       gchar **a_property)
{
  icalproperty *prop = NULL ;

  g_return_val_if_fail (a_comp, FALSE) ;
  g_return_val_if_fail (a_name, FALSE) ;

  if (icalcomponent_find_property (a_comp, a_name, &prop))
  {
    *a_property = (gchar*) icalproperty_get_value_as_string (prop) ;
    return TRUE ;
  }
  return FALSE ;
}

/*</private funcs>*/


/*<public funcs>*/

/**
 * moko_journal_open_default:
 *
 * Opens the default journal.
 *
 * Return value: a pointer to the journal object
 */
MokoJournal*
moko_journal_open_default ()
{
  ECal *ecal = NULL;
  gchar *uri = NULL ;
  MokoJournal *result=NULL, *journal=NULL ;
  GError *error = NULL ;

  uri = g_build_filename ("file://", g_get_home_dir (),
                          ".moko", "journal", NULL);

  ecal = e_cal_new_from_uri (uri, E_CAL_SOURCE_TYPE_JOURNAL) ;
  if (!ecal)
  {
    g_warning ("failed to create ecal with uri: %s\n", uri) ;
    goto out ;
  }

  if (!e_cal_open (ecal, FALSE, &error))
  {
    g_warning ("could not open the journal at uri %s\n", uri) ;
    goto out ;
  }
  if (error)
  {
    g_warning ("got error: %s\n", error->message) ;
    goto out ;
  }
  journal = moko_journal_alloc () ;
  journal->ecal = ecal ;
  ecal = NULL ;

  result = journal ;
  journal = NULL ;

out:
  g_free (uri) ;

  if (ecal)
    g_object_unref (G_OBJECT (ecal)) ;

  if (journal)
    moko_journal_free (journal) ;

  if (error)
    g_error_free (error) ;

  return result ;
}

/**
 * moko_journal_close:
 * @journal: the journal to close
 *
 * Close the journal previously opened with moko_journal_open_default().
 * This function deallocates the memory of the Journal object.
 */
void
moko_journal_close (MokoJournal *a_journal)
{
  g_return_if_fail (a_journal) ;

  moko_journal_free (a_journal) ;
}

/**
 * moko_journal_add_entry:
 * @journal: the current instance of journal
 * @entry: the new entry to add to the journal. The journal is responsible
 * of deallocating the memory of the entry object.
 *
 * Add a journal entry to the journal
 *
 * Return value: TRUE if the entry got successfully added to the journal,
 * FALSE otherwise
 */
gboolean
moko_journal_add_entry (MokoJournal *a_journal, MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_entry, FALSE) ;

  g_array_append_val (a_journal->entries, a_entry) ;
  return TRUE ;
}

/**
 * moko_journal_get_nb_entries:
 * @journal: the current instance of journal
 *
 * Return value: the number of entries in the journal or a negative value
 * in case of error.
 */
int
moko_journal_get_nb_entries (MokoJournal *a_journal)
{
  g_return_val_if_fail (a_journal, -1) ;
  if (!a_journal->entries)
    return 0 ;
  return a_journal->entries->len ;
}

/**
 * moko_journal_get_entry_at:
 * @journal: the current instance of journal
 * @index: the index to get the journal entry from
 * @entry: out parameter. the resulting journal entry
 *
 * Get the journal entry at a given index.
 *
 * Return value: TRUE in case of success, FALSE otherwise.
 */
gboolean
moko_journal_get_entry_at (MokoJournal *a_journal,
                           guint a_index,
                           MokoJournalEntry **a_entry)
{
  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_journal->entries, FALSE) ;
  g_return_val_if_fail (a_index < a_journal->entries->len, FALSE) ;

  *a_entry = g_array_index (a_journal->entries, MokoJournalEntry*, a_index) ;
  return TRUE ;
}

/**
 * moko_journal_remove_entry_at:
 * @journal: the current instance of journal
 * @index: the index to remove the entry from
 *
 * Remove a journal entry from index #index
 */
gboolean
moko_journal_remove_entry_at (MokoJournal *a_journal,
                              guint a_index)
{
  MokoJournalEntry *entry = NULL ;

  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_journal->entries, FALSE) ;
  g_return_val_if_fail (a_index < a_journal->entries->len, FALSE) ;

  entry = g_array_index (a_journal->entries, MokoJournalEntry*, a_index) ;
  if (entry)
  {
    /*
     * if entry has been persisted already,
     * queue a removal from storage
     */
    if (entry->uid)
    {
      a_journal->entries_to_delete =
        g_list_prepend (a_journal->entries_to_delete, entry);
    }
    /*
     * remove entry from memory cache
     */
    g_array_remove_index (a_journal->entries, a_index) ;
    /*
     * if entry has not been pushed onto
     * storage deletion queue,
     * deallocate its memory
     */
    if (!entry->uid)
    {
      moko_journal_entry_free (entry) ;
    }
    return TRUE ;
  }
  return FALSE ;
}

/**
 * moko_journal_remove_entry_by_uid:
 * @journal: the current instance of journal
 * @uid: the uid of the journal entry to remove
 *
 * Remove the journal entry that has a given UID.
 *
 * Return value: TRUE in case of success, FALSE otherwise
 */
gboolean
moko_journal_remove_entry_by_uid (MokoJournal *a_journal,
                                  const gchar* a_uid)
{
  int i=0 ;
  MokoJournalEntry *entry=NULL ;

  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_journal->entries, FALSE) ;
  g_return_val_if_fail (a_uid, FALSE) ;

  for (i=0 ; i < a_journal->entries->len ; ++i)
  {
    entry = g_array_index (a_journal->entries, MokoJournalEntry*, i) ;
    if (entry && entry->uid && !strcmp (entry->uid, a_uid))
    {
      return moko_journal_remove_entry_at (a_journal, i) ;
    }
  }
  return FALSE ;
}

/**
 * moko_journal_write_to_storage:
 * @journal: the journal to save to storage
 *
 * Saves the journal to persistent storage (e.g disk) using the
 * appropriate backend. The backend currently used is evolution data server
 *
 * Return value: TRUE in case of success, FALSE otherwise
 */
gboolean
moko_journal_write_to_storage (MokoJournal *a_journal)
{
  MokoJournalEntry *cur_entry=NULL ;
  int i=0 ;
  GList *ecal_comps=NULL, *cur_elem=NULL ;
  ECalComponent *ecal_comp=NULL ;
  icalcomponent *ical_comp=NULL ;
  GError *error=NULL ;
  gboolean wrote = FALSE;
  gboolean result = TRUE;

  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_journal->ecal, FALSE) ;

  /*create ECalComponent objects out of the list of MokoJournalEntry* we have*/
  for (i=0; a_journal->entries && i<a_journal->entries->len; ++i)
  {
    cur_entry = g_array_index (a_journal->entries, MokoJournalEntry*, i) ;
    if (!cur_entry)
      break ;
    if (!moko_journal_entry_to_icalcomponent (cur_entry, &ical_comp))
    {
      if (ical_comp)
      {
        icalcomponent_free (ical_comp) ;
        ical_comp = NULL ;
      }
      continue ;
    }
    ecal_comp = e_cal_component_new () ;
    e_cal_component_set_icalcomponent (ecal_comp, ical_comp) ;
    g_object_set_data (G_OBJECT (ecal_comp), "journal-entry", cur_entry) ;
    ecal_comps = g_list_prepend (ecal_comps, ecal_comp) ;
    ecal_comp = NULL ;
    ical_comp = NULL ;
  }

  /*
   * walk the list of ECalComponent objects to either add them
   * to eds or modify them (in eds) if there were already present in eds.
   */
  for (cur_elem = ecal_comps ; cur_elem ; cur_elem = cur_elem->next)
  {
    if (!cur_elem->data)
    {
      g_warning ("Got an empty ECalComponent\n") ;
      result = FALSE ;
      continue ;
    }
    ical_comp = e_cal_component_get_icalcomponent (cur_elem->data) ;
    if (!ical_comp)
    {
      g_warning ("Got an empty icalcomponent") ;
      result = FALSE ;
      continue ;
    }
    cur_entry = g_object_get_data (G_OBJECT (cur_elem->data),
        "journal-entry") ;
    if (!cur_entry)
    {
      g_warning ("could not get journal entry from cur_elem->data") ;
      continue ;
    }
    if (cur_entry->uid)
    {
      /*entry exists already in eds, modify it*/
      if (!e_cal_modify_object (a_journal->ecal, ical_comp,
                                CALOBJ_MOD_THIS, &error))
      {
        g_warning ("Could not modify entry in the journal") ;
        result = FALSE ;
      }
    }
    else
    {
      /*entry did not exist in eds previously, add it*/
      if (!e_cal_create_object (a_journal->ecal, ical_comp,
                                &cur_entry->uid, &error))
      {
        g_warning ("Could not write the entry to the journal") ;
        result = FALSE ;
      }
      else
      {
        e_cal_component_commit_sequence (cur_elem->data) ;
        wrote = TRUE ;
      }
    }
    if (error)
    {
      g_warning ("got error: %s\n", error->message) ;
      g_error_free (error) ;
      error = NULL ;
    }
  }

  /*
   * remove entries that have been
   * queued to be removed
   */
  for (cur_elem = a_journal->entries_to_delete ;
       cur_elem ;
       cur_elem = cur_elem->next)
  {
    if (!cur_elem->data)
      continue ;

    if (((MokoJournalEntry*)cur_elem->data)->uid)
    {
      if (!e_cal_remove_object (a_journal->ecal,
                                ((MokoJournalEntry*)cur_elem->data)->uid,
                                &error))
      {
        /*
         * this can happen if another process for instance, has
         * already deleted the entry. In that case, we just
         * print message and continue
         */
        g_message ("failed to remove object with UID %s\n",
                   ((MokoJournalEntry*)cur_elem->data)->uid) ;
      }
      if (error)
      {
        g_warning ("got error %s\n", error->message) ;
        g_error_free (error) ;
        error = NULL ;
      }
    }
    moko_journal_entry_free ((MokoJournalEntry*)cur_elem->data) ;
    cur_elem->data = NULL ;
  }

  if (a_journal->entries_to_delete)
  {
    g_list_free (a_journal->entries_to_delete) ;
    a_journal->entries_to_delete = NULL ;
  }

  if (ecal_comps)
  {
    GList *cur;

    for (cur = ecal_comps ; cur ; cur = cur->next)
      g_object_unref (cur->data) ;
    g_list_free (ecal_comps) ;
  }

  return result ;
}

static void
on_entries_added_cb (ECalView *a_view,
                     GList *a_entries,
                     MokoJournal *a_journal)
{
  icalcomponent *ical_comp = NULL ;
  GList *cur_entry = NULL ;
  MokoJournalEntry *entry = NULL ;
  int offset=0 ;

  for (cur_entry = a_entries ; cur_entry ; cur_entry = cur_entry->next)
  {
    /*****************
     * <sanity checks>
     *****************/
    if (!icalcomponent_isa_component (cur_entry->data))
    {
      /*hugh ? this does not look like an ical component. ignore it.*/
      continue ;
    }
    if (!icalcomponent_get_uid (cur_entry->data))
    {
      /*hugh ? an ical component without uid ? ignore it*/
      continue ;
    }
    if (moko_journal_find_entry_from_uid
                                    (a_journal,
                                     icalcomponent_get_uid (cur_entry->data),
                                     &entry,
                                     &offset))
    {
      /*we already have the component in memory, ignore it*/
      continue ;
    }
    /*****************
     * </sanity checks>
     *****************/

    /*
     * okay, build a journal entry from the ical component and cache it
     * in memory
     */
    ical_comp = cur_entry->data ;
    if (!icalcomponent_to_entry (ical_comp, &entry) || !entry)
    {
      if (entry)
      {
        moko_journal_entry_free (entry) ;
        entry = NULL ;
      }
      continue ;
    }
    moko_journal_add_entry (a_journal, entry) ;
    entry = NULL ;
  }
}

static void
on_entries_removed_cb (ECalView *a_view,
                       GList *a_uids,
                       MokoJournal *a_journal)
{
  GList *cur = NULL ;

  g_return_if_fail (a_view && E_IS_CAL_VIEW (a_view)) ;
  g_return_if_fail (a_journal) ;

  /*TODO:
   * we should notify the world before removing an entry.
   * Otherwise, client code may hold a reference to
   * what will become a dangling pointer after we remove it here
   */
  for (cur = a_uids ; cur ; cur = cur->next)
  {
    if (cur->data)
    {
      if (!moko_journal_remove_entry_by_uid (a_journal, cur->data))
      {
        g_message ("failed to remove entry of uid %s\n",
                   (const char*)cur->data) ;
      }
    }
  }
}

/**
 * moko_journal_load_from_storage:
 * @a_journal: the journal to load entries into
 *
 * Read the journal entries stored in the persistent storage (filesystem)
 * and load then into the current instance of MokoJournal.
 *
 * Return value: TRUE in case of success, FALSE otherwise
 */
gboolean
moko_journal_load_from_storage (MokoJournal *a_journal)
{
  GError *error = NULL ;
  GList *objs = NULL ;

  g_return_val_if_fail (a_journal, FALSE) ;
  g_return_val_if_fail (a_journal->ecal, FALSE) ;

  if (!a_journal->ecal_view)
  {
    /*
     * first, issue a query to get a view that we can
     * listen to, to get notified by new objects that get added to eds
     * during the life time of a_journal
     */
    if (!e_cal_get_query (a_journal->ecal, "#t",
                          &a_journal->ecal_view,
                          &error)
        || error)
    {
      if (error)
      {
        if (error->message)
        {
          g_warning ("got error: %s\n", error->message) ;
        }
        g_error_free (error) ;
        error = NULL ;
      }
      return FALSE ;
    }
    g_signal_connect (G_OBJECT (a_journal->ecal_view),
                      "objects-added",
                      G_CALLBACK (on_entries_added_cb),
                      a_journal) ;
    g_signal_connect (G_OBJECT (a_journal->ecal_view),
                      "objects-removed",
                      G_CALLBACK (on_entries_removed_cb),
                      a_journal) ;

    e_cal_view_start (a_journal->ecal_view) ;
  }

  /*
   * really get the objects from eds to let the caller start working
   * right after the call completes
   */
  if (!e_cal_get_object_list (a_journal->ecal, "#t", &objs, &error))
  {
    g_warning ("failed to get ical journal objects from eds") ;
  }
  if (error)
  {
    g_warning ("got error %s\n", error->message) ;
    g_error_free (error) ;
    error = NULL ;
  }
  if (objs)
  {
    GList *cur=NULL ;
    MokoJournalEntry *entry = NULL ;
    for (cur = objs ; cur ; cur = cur->next)
    {
      if (!icalcomponent_isa (cur->data))
        continue ;
      if (icalcomponent_to_entry (cur->data, &entry) && entry)
      {
        moko_journal_add_entry (a_journal, entry) ;
        entry = NULL ;
      }
      if (entry)
      {
        g_warning ("entry should be NULL here") ;
        moko_journal_entry_free (entry) ;
        entry = NULL ;
      }
    }
    e_cal_free_object_list (objs) ;
    objs = NULL ;
  }

  /*give us a chance to get notified by entries arriving ...*/
  while (g_main_context_pending (g_main_context_default ()))
  {
    g_main_context_iteration (g_main_context_default (), FALSE) ;
  }

  return TRUE ;
}

/**
 * moko_journal_entry_new:
 *
 * Create a Journal entry with no properties set.
 * Use the JEntry accessors to get/set properties.
 *
 * Return value: the newly created journal entry object
 */
MokoJournalEntry*
moko_journal_entry_new (enum MokoJournalEntryType a_type)
{
  MokoJournalEntry *result ;
  result = moko_journal_entry_alloc () ;
  result->type = a_type ;
  return result ;
}

/**
 * moko_journal_entry_free:
 * @entry: the entry to free
 *
 * Deallocate the memory of the journal entry object
 */
void
moko_journal_entry_free (MokoJournalEntry *a_entry)
{
  g_return_if_fail (a_entry) ;

  moko_journal_entry_free_real (a_entry) ;
}

/**
 * moko_journal_entry_get_type:
 * @entry: the current journal entry
 *
 * get the primary type of the journal entry
 *
 * Return value: the type of the journal entry
 */
enum MokoJournalEntryType
moko_journal_entry_get_type (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, UNDEF_ENTRY) ;

  return a_entry->type ;
}


/**
 * moko_journal_entry_set_type:
 * @entry: the current instance of journal entry
 * @type: the new type
 *
 * Set the type of the journal entry
 */
void
moko_journal_entry_set_type (MokoJournalEntry *a_entry,
                             enum MokoJournalEntryType a_type)
{
  g_return_if_fail (a_entry) ;
  g_return_if_fail (a_type != UNDEF_ENTRY) ;

  a_entry->type = a_type ;
}

/**
 * moko_journal_entry_get_uid:
 * @entry: the current instance of journal entry
 *
 * Gets the UID of the current entry. This UID is non NULL if and
 * only if the entry has been persistet at least once.
 *
 * Return value: the UID in case the entry has been persisted at least once,
 * NULL otherwise. The client code must *NOT* free the returned string.
 */
const gchar*
moko_journal_entry_get_uid (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  return a_entry->uid ;
}

/**
 * moko_journal_entry_get_contact_uid:
 * @entry: the current instance of journal entry
 *
 * get the contact uid
 *
 * Return value: the UID of the contact. It can be NULL. Client code
 * must not deallocate or attempt to alter it.
 */
const gchar*
moko_journal_entry_get_contact_uid (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  return a_entry->contact_uid ;
}

void
moko_journal_entry_set_contact_uid (MokoJournalEntry *a_entry,
                                    const gchar *a_uid)
{
  g_return_if_fail (a_entry) ;

  if (a_entry->contact_uid)
  {
    g_free (a_entry->contact_uid) ;
    a_entry->contact_uid = NULL ;
  }

  if (a_uid)
  {
    a_entry->contact_uid = g_strdup (a_uid) ;
  }
}

/**
 * moko_journal_entry_get_summary:
 * @entry: the current instance of journal entry
 *
 * get the summary of the journal entry
 *
 * Return value: the summary of the journal entry. It can be NULL.
 * Client code must not deallocate or alter it.
 */
const gchar*
moko_journal_entry_get_summary (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  return a_entry->summary ;
}

/**
 * moko_journal_entry_set_summary:
 * @entry: the current instance of journal entry
 * @summary: the new summary of the journal entry. It is copied
 * so client code is reponsible of its lifecyle.
 *
 * Set the summary of the journal entry
 */
void
moko_journal_entry_set_summary (MokoJournalEntry *a_entry,
                                const gchar* a_summary)
{
  g_return_if_fail (a_entry) ;

  if (a_entry->summary)
  {
    g_free (a_entry->summary) ;
    a_entry->summary = NULL ;
  }
  if (a_summary)
  {
    a_entry->summary = g_strdup (a_summary) ;
  }
}

/**
 * moko_journal_entry_get_dtdstart:
 * @entry: the current instance of journal entry
 *
 * get the starting date associated to the journal entry
 *
 * Return value: an icaltimetype representing the starting date expected.
 * It can be NULL. Client code must not deallocate it.
 */
const MokoTime*
moko_journal_entry_get_dtstart (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  if (!a_entry->dtstart)
    a_entry->dtstart = moko_time_new_today () ;

  return a_entry->dtstart ;
}

/**
 * moko_journal_entry_get_start_location:
 * @a_entry: the current instance of journal entry
 * @a_location: the requested location
 *
 * Get the location at which the message got received or sent.
 *
 * Returns: TRUE upon sucessful completion, FALSE otherwise.
 */
gboolean
moko_journal_entry_get_start_location (MokoJournalEntry *a_entry,
                                       MokoLocation *a_location)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_location, FALSE) ;

  a_location->longitude = a_entry->start_longitude ;
  a_location->latitude = a_entry->start_latitude ;

  return TRUE ;
}

/**
 * moko_journal_entry_set_location:
 * @a_entry: the current intance of journal entry
 * @a_location: the new location
 *
 * Set a new location to the journal entry
 * Location represents the longitude/latitude at which a call or message
 * occured.
 *
 * Returns: TRUE upon successful completion, FALSE otherwise.
 */
gboolean
moko_journal_entry_set_start_location (MokoJournalEntry *a_entry,
                                       MokoLocation *a_location)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_location, FALSE) ;

  a_entry->start_longitude = a_location->longitude ;
  a_entry->start_latitude = a_location->latitude ;
  return TRUE ;
}

/**
 * moko_journal_entry_get_direction:
 * @entry: the current instance of journal entry
 * @direction: either DIRECTION_IN for a received message or DIRECTION_OUT
 * for a sent message.
 *
 * get the direction of the message
 *
 * Returns: TRUE in case of success, FALSE otherwise.
 */
gboolean
moko_journal_entry_get_direction (MokoJournalEntry *a_entry,
                                  enum MessageDirection *a_direction)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_direction, FALSE) ;

  *a_direction = a_entry->direction ;
  return TRUE ;
}

/**
 * moko_journal_entry_set_direction:
 * @entry: the current instance of journal entry
 * @direction: the new message direction to set
 *
 * set message direction
 *
 */
void moko_journal_entry_set_direction (MokoJournalEntry *a_entry,
                                       enum MessageDirection direction)
{
  g_return_if_fail (a_entry) ;
  g_return_if_fail (direction) ;

  a_entry->direction = direction ;
}

/**
 * moko_journal_entry_set_dtstart:
 * @entry: the current instance of journal entry
 * @dtstart: the new starting date associated to the journal entry.
 */
void
moko_journal_entry_set_dtstart (MokoJournalEntry *a_entry, MokoTime* a_dtstart)
{
  g_return_if_fail (a_entry) ;

  if (a_entry->dtstart)
  {
    moko_time_free (a_entry->dtstart) ;
    a_entry->dtstart = NULL ;
  }

  if (a_dtstart)
    a_entry->dtstart = a_dtstart ;
}

/**
 * moko_journal_entry_get_source:
 * @a_entry: the current instance of journal entry
 *
 * Returns: the source property. It is an arbitrary string representing
 * the application that was the source of the entry (like mokodialer)
 */
const gchar*
moko_journal_entry_get_source (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  return a_entry->source ;
}

/**
 * moko_journal_entry_set_source:
 * @a_entry: the current instance of journal entry
 * @a_source: the new source to set
 *
 * Set the source property. It is an arbitrary string representing
 * the application that was the source of the entry (like mokodialer)
 */
void
moko_journal_entry_set_source (MokoJournalEntry *a_entry,
                               const gchar *a_source)
{
  g_return_if_fail (a_entry) ;

  if (a_entry->source)
  {
    g_free (a_entry->source) ;
    a_entry->source = NULL ;
  }
  if (a_source)
    a_entry->source = g_strdup (a_source) ;
}

/**
 * moko_journal_entry_set_gsm_location:
 * @a_entry: the current instance of voice call extra properties set
 * @a_location: the gsm location
 *
 * Returns: TRUE upon completion, FALSE otherwise
 */
gboolean
moko_journal_entry_set_gsm_location (MokoJournalEntry *a_entry,
                                     MokoGSMLocation *a_location)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_location, FALSE) ;

  memcpy (&a_entry->gsm_loc, a_location, sizeof (MokoGSMLocation)) ;
  return TRUE ;
}

/**
 * moko_journal_entry_get_gsm_location:
 * @a_entry: the current instance of voice call extra properties set
 * @a_location: the gsm location
 *
 * Returns TRUE upon completion, FALSE otherwise
 */
gboolean
moko_journal_entry_get_gsm_location (MokoJournalEntry *a_info,
                                     MokoGSMLocation *a_location)
{
  g_return_val_if_fail (a_info, FALSE) ;
  g_return_val_if_fail (a_info, FALSE) ;

  memcpy (a_location, &a_info->gsm_loc, sizeof (MokoGSMLocation)) ;
  return TRUE ;
}

/**
 * moko_journal_entry_set_wifi_ap_mac_address:
 * @entry: the current instance of journal entry
 *
 * the mac address of the wifi access point.
 * It is must be a 48 bits long string of bytes.
 *
 * Returns: TRUE in case of success, FALSE otherwise.
 */
gboolean
moko_journal_entry_set_wifi_ap_mac_address (MokoJournalEntry *a_entry,
                                            const guchar *a_address)
{
  g_return_val_if_fail (a_entry, FALSE) ;

  if (!a_address)
  {
    if (a_entry->wifi_ap_mac)
    {
      g_free (a_entry->wifi_ap_mac) ;
      a_entry->wifi_ap_mac = NULL ;
    }
    return TRUE ;
  }

  if (!a_entry->wifi_ap_mac)
  {
    a_entry->wifi_ap_mac = g_new0 (guchar, WIFI_AP_MAC_ADDR_LEN) ;
  }
  g_return_val_if_fail (a_entry->wifi_ap_mac, FALSE) ;
  memcpy (a_entry->wifi_ap_mac, a_address, WIFI_AP_MAC_ADDR_LEN) ;

  return TRUE ;
}

/**
 * moko_journal_entry_get_wifi_ap_mac_address:
 * @entry: the current instance of journal entry
 *
 * Returns: the mac address of the wifi access point.
 * It is a 48 bits long string of bytes. The calling code must
 * *NOT* delete this pointer. This function can also return NULL if
 * no wifi access point mac address has been set in the entry.
 */
const guchar*
moko_journal_entry_get_wifi_ap_mac_address (MokoJournalEntry *a_entry)
{
  g_return_val_if_fail (a_entry, NULL) ;

  return a_entry->wifi_ap_mac ;
}

/**
 * moko_journal_entry_get_voice_info:
 * @a_entry: the current instance of journal entry
 * @a_info: the extra property set or NULL if info is not of type
 * VOICE_JOURNAL_ENTRY
 *
 * Returns the specific property set associated to instance of MokoJournalEntry
 * of type VOICE_JOURNAL_ENTRY.
 *
 * Returns: TRUE upon successful completion, FALSE otherwise.
 */
gboolean
moko_journal_entry_get_voice_info (MokoJournalEntry *a_entry,
                                   MokoJournalVoiceInfo **a_info)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_entry->type == VOICE_JOURNAL_ENTRY, FALSE) ;
  g_return_val_if_fail (a_info, FALSE) ;

  if (!a_entry->extra.voice_info)
  {
    a_entry->extra.voice_info = moko_journal_voice_info_new ();
  }
  g_return_val_if_fail (a_entry->extra.voice_info, FALSE) ;
  *a_info = a_entry->extra.voice_info ;
  return TRUE ;
}

void
moko_journal_voice_info_set_dialed_number (MokoJournalVoiceInfo *a_info,
                                           gchar *a_number)
{
  g_return_if_fail (a_info) ;

  if (a_info->dialed_number)
  {
    g_free (a_info->dialed_number) ;
    a_info->dialed_number = NULL ;
  }
  if (a_number)
    a_info->dialed_number = g_strdup (a_number) ;
}

const gchar*
moko_journal_voice_info_get_dialed_number (MokoJournalVoiceInfo *a_info)
{
  g_return_val_if_fail (a_info, NULL) ;

  return a_info->dialed_number ;
}

void
moko_journal_voice_info_set_was_missed (MokoJournalVoiceInfo *a_info,
                                        gboolean a_flag)
{
  g_return_if_fail (a_info) ;

  a_info->was_missed = a_flag ;
}

gboolean
moko_journal_voice_info_get_was_missed (MokoJournalVoiceInfo *a_info)
{
  g_return_val_if_fail (a_info, FALSE) ;

  return a_info->was_missed ;
}

/**
 * moko_journal_entry_get_fax_info:
 * @entry: the current instance of journal entry
 * @info: the fax info properties set
 *
 * get the extra properties set associated to journal entries of
 * type FAX_JOURNAL_ENTRY
 *
 * Returns: TRUE in case of success, FALSE otherwise.
 */
gboolean
moko_journal_entry_get_fax_info (MokoJournalEntry *a_entry,
                                 MokoJournalFaxInfo **a_info)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_entry->type == FAX_JOURNAL_ENTRY, FALSE) ;
  g_return_val_if_fail (a_info, FALSE) ;

  if (!a_entry->extra.fax_info)
  {
    a_entry->extra.fax_info = moko_journal_fax_info_new () ;
  }
  g_return_val_if_fail (a_entry->extra.fax_info, FALSE) ;
  *a_info = a_entry->extra.fax_info ;
  return FALSE ;
}

/**
 * moko_journal_entry_get_data_info:
 * @a_entry: the current instance of journal entry
 * @a_info: the resulting properties set
 *
 * Get the extra properties set associated to journal entries of type
 * DATA_JOURNAL_ENTRY
 *
 * Returns: TRUE in case of success, FALSE otherwise.
 */
gboolean moko_journal_entry_get_data_info (MokoJournalEntry *a_entry,
                                           MokoJournalDataInfo **a_info)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_entry->type == DATA_JOURNAL_ENTRY, FALSE) ;
  g_return_val_if_fail (a_info, FALSE) ;

  if (!a_entry->extra.data_info)
  {
    a_entry->extra.data_info = moko_journal_data_info_new () ;
  }
  g_return_val_if_fail (a_entry->extra.data_info, FALSE) ;
  *a_info = a_entry->extra.data_info ;
  return TRUE ;
}

/**
 * moko_journal_entry_get_sms_info:
 * @a_entry: the current instance of journal entry
 * @a_info: the resulting properties set
 *
 * Get the extra properties set associated to journal entries of type
 * SMS_JOURNAL_ENTRY
 */
gboolean
moko_journal_entry_get_sms_info (MokoJournalEntry *a_entry,
                                 MokoJournalSMSInfo **a_info)
{
  g_return_val_if_fail (a_entry, FALSE) ;
  g_return_val_if_fail (a_entry->type == SMS_JOURNAL_ENTRY, FALSE) ;
  g_return_val_if_fail (a_info, FALSE) ;

  if (a_entry->extra.sms_info)
  {
    a_entry->extra.sms_info = moko_journal_sms_info_new () ;
  }
  g_return_val_if_fail (a_entry->extra.sms_info, FALSE) ;
  *a_info = a_entry->extra.sms_info ;
  return TRUE ;
}

/**
 * moko_journal_entry_get_email_info:
 * @entry: the current instance of journal entry
 * @info: extra information attached to the email info, or NULL.
 * Client code must *NOT* of deallocate the returned info.
 * It is the duty of the MokoJournalEntry code to deallocate it when
 * necessary
 *
 * Return value: TRUE if the call succeeded, FALSE otherwise.
 */
gboolean
moko_journal_entry_get_email_info (MokoJournalEntry *a_entry,
                                   MokoJournalEmailInfo **a_info)
{
  g_return_val_if_fail (a_entry->type == EMAIL_JOURNAL_ENTRY, FALSE) ;

  if (!a_entry->extra.email_info)
  {
    a_entry->extra.email_info = moko_journal_email_info_new () ;
  }
  g_return_val_if_fail (a_entry->extra.email_info, FALSE) ;

  *a_info = a_entry->extra.email_info ;
  return TRUE ;
}

/*</public funcs>*/
