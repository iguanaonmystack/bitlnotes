/* bitlnotes - buddy notes plug-in for bitlbee.
 * Copyright (c) 2010, 2011 Nick Murdoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "bitlbee.h"
#include "irc.h"

/* called when user connects */
//gboolean notes_irc_new(irc_t *irc);

//void notes_irc_free(irc_t *irc);

/* Called when the user identifies, iff this plugin is mentioned in the user's
   config file. We receive ownership of `data`, which might feasibly be NULL.*/
void notes_load(irc_t *irc, char * data);

/* Called when user saves config ('save').
   We give ownership of the returned char* to bitlbee. */
char * notes_save(irc_t *irc);

// void notes_remove(const char *nick);

/* Called when receiving a message from someone */
//char * notes_filter_msg_in(irc_user_t *iu, char * msg, int flags);

/* Called when sending a message to someone.
   Return NULL to signal that the message should not be sent.
*/
//char * notes_filter_msg_out(irc_user_t *iu, char * msg, int flags) {

gpointer notes_plugindata_new(irc_t *irc);
void notes_plugindata_free(irc_t *irc, gpointer plugindata);



static const struct irc_plugin notes_plugin = {
    .plugin_name = "bitlnotes",
/*    .irc_new = notes_irc_new,
    .irc_free = notes_irc_free,
    .filter_msg_out = notes_filter_msg_out,
    .filter_msg_in = notes_filter_msg_in,*/
    .storage_load = notes_load,
    .storage_save = notes_save,
    //.storage_remove = notes_remove,
    .plugindata_new = notes_plugindata_new,
    .plugindata_free = notes_plugindata_free,
};



void notes_load(irc_t *irc, char * data) {
    GHashTable * users_notes = get_plugindata(irc, &notes_plugin);
    gchar ** lines_v; /* NULL-terminated array of strings */
    gchar ** line_p;
    char * nick = NULL;
    GList * notes = NULL; /* empty list */
    if (data == NULL) return;
    
    lines_v = g_strsplit(data, "\n", 0);
    for (line_p = lines_v; *line_p != 0; line_p++) {
        if (g_strcmp0(*line_p, "") == 0) {
            /* end of block */
            if (nick == NULL)
                continue; /* extraneous blank lines */
            irc_usermsg(irc, "Loading notes for %s", nick);
            irc_user_t *iu = irc_user_by_name(irc, nick);
            if (iu != NULL) {
                irc_usermsg(irc, "Assigning notes for %s", nick);
                g_hash_table_insert(users_notes, iu, notes);
            }
            nick = NULL;
            notes = NULL;
        } else if (nick == NULL) {
            /* first line of block == nick */
            nick = *line_p;
        } else {
            /* non-first line of block == note */
            notes = g_list_append(notes, g_string_new(*line_p));
        }
    }
    g_strfreev(lines_v);
    g_free(data);
}

char * notes_save(irc_t *irc) {
    char * result;
    GString * serialised;
    GHashTable * users_notes = get_plugindata(irc, &notes_plugin);
    GHashTableIter iter;
    gpointer key, value;
    //irc_usermsg(irc, "notes_save");
    
    serialised = g_string_new("");
    g_hash_table_iter_init(&iter, users_notes);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GList * cursor = g_list_first((GList*)value);
        g_string_append_printf(serialised, "%s\n", ((irc_user_t*)key)->nick);
        for (; cursor != NULL; cursor = g_list_next(cursor)) {
            g_string_append_printf(
                serialised, "%s\n", ((GString*)cursor->data)->str);
        }
    }
    
    result = g_string_free(serialised, FALSE); // bitlbee owns result now.
    return result;
}

gpointer notes_plugindata_new(irc_t *irc) {
    GHashTable * users_notes;
    users_notes = g_hash_table_new(NULL, NULL);
    return users_notes;
}

void notes_plugindata_free(irc_t *irc, gpointer plugindata) {
    irc_usermsg(irc, "notes_plugindata_free");
    /* plugindata is users_notes */
    /* TODO - remove individual entries (use delete function in g_hash_table_new_full?) */
    g_hash_table_destroy(plugindata);
}


static void cmd_notes( irc_t *irc, char **cmd ) {
    char * nick = cmd[1];
    char * subcmd = cmd[2]; // TODO - lowercase for consistency
    unsigned i = 2;
    GString* note = NULL;
    irc_user_t *iu = irc_user_by_name(irc, nick);
    GHashTable * users_notes = get_plugindata(irc, &notes_plugin);

    if( !iu || !iu->bu )
    {
        irc_usermsg( irc, "Nick `%s' does not exist", nick );
        return;
    }

    note = g_string_new( "" );
    /* User could put the whole note in quotes but we can be helpful too! */
    while ( ++i && cmd[i] != NULL ) {
        if ( i != 3 )
            g_string_append(note, " ");
        g_string_append(note, cmd[i]);
    }
    if (strcmp(subcmd, "add") == 0) {
        /* Add the note */
        if (note->len > 0) {
            GList * notes = g_hash_table_lookup(users_notes, iu);
            notes = g_list_append(notes, note);
            g_hash_table_insert(users_notes, iu, notes);
            irc_usermsg(irc, "Added note for %s.", nick);
        } else {
            /* Technically there's no reason we can't add an empty
               string, but it'd screw up the save/load functions */
            irc_usermsg(irc, "Cannot add empty note.");
        }
    } else if (strcmp(subcmd, "list") == 0) {
        /* Retrive the notes */
        GList * cursor = NULL;
        GList * notes = g_hash_table_lookup(users_notes, iu);
        if (notes) {
            int n = 0;
            irc_usermsg(irc, "Your notes for %s:", nick);
            for (cursor = g_list_first(notes);
                 cursor != NULL;
                 cursor = g_list_next(cursor)) {
                irc_usermsg(irc, "%d. %s", n, ((GString*)cursor->data)->str);
                ++n;
            }
        } else {
            irc_usermsg(irc, "No notes set for %s.", nick);
        }
    } else if (strcmp(subcmd, "del") == 0) {
        /* Delete the given note (by index number) */
        int n = atoi(note->str);
        GList * notes = g_hash_table_lookup(users_notes, iu);
        GList * cursor = NULL;
        int notes_len = g_list_length(notes);
        if (n < notes_len) {
            cursor = g_list_nth(notes, n);
            /* TODO - free cursor->data (the GString note) */
            notes = g_list_delete_link(notes, cursor);
            g_hash_table_insert(users_notes, iu, notes);
            irc_usermsg(irc, "Deleted note %d for %s.", n, nick);
        } else {
            if (notes_len == 0) {
                irc_usermsg(irc, "There no notes to delete for %s.", nick);
            } else {
                irc_usermsg(irc, "There are only %d notes for %s.",
                    notes_len, nick);
            }
        }
    } else {
        irc_usermsg(irc, "Invalid subcommand for notes: %s.", subcmd);
    }

    //g_string_free(note, free_note_data);
}


void init_plugin(void) {
    root_command_add("notes", 2, cmd_notes, 0);
    register_irc_plugin(&notes_plugin);
}

