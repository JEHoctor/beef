
/* Beef -- Flexible Brainfuck interpreter
 * Copyright (C) 2005-2011  Andrea Bolognani <eof@kiyuko.org>
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
 *
 * Homepage: http://kiyuko.org/software/beef
 */

#include <glib-object.h>
#include <cattle/cattle.h>
#include "options.h"

/**
 * parse_store:
 *
 * Parse the argument of the --store option.
 */
static gboolean
parse_store (const gchar  *option_name,
             const gchar  *value,
             gpointer      data,
             GError      **error)
{
	CattleConfiguration *configuration;

	configuration = CATTLE_CONFIGURATION (data);

	if (g_utf8_collate (value, "zero") == 0) {

		cattle_configuration_set_on_eof_action (configuration,
		                                        CATTLE_ON_EOF_STORE_ZERO);
	}
	else if (g_utf8_collate (value, "eof") == 0) {

		cattle_configuration_set_on_eof_action (configuration,
		                                        CATTLE_ON_EOF_STORE_EOF);
	}
	else if (g_utf8_collate (value, "same") == 0) {

		cattle_configuration_set_on_eof_action (configuration,
		                                        CATTLE_ON_EOF_DO_NOTHING);
	}
	else {

		g_set_error (error,
		             G_OPTION_ERROR,
		             G_OPTION_ERROR_FAILED,
		             "Invalid argument %s for %s",
		             value,
		             option_name);

		return FALSE;
	}

	return TRUE;
}

/**
 * parse_debugging:
 *
 * Parse the argument of the --enable-debugging option.
 */
static gboolean
parse_debugging (const gchar *option_name,
                 const gchar  *value,
                 gpointer      data,
                 GError      **error)
{
	CattleConfiguration *configuration;

	configuration = CATTLE_CONFIGURATION (data);

	if (value == NULL) {

		cattle_configuration_set_debug_is_enabled (configuration,
		                                           TRUE);
	}
	else if (g_utf8_collate (value, "yes") == 0 ||
	         g_utf8_collate (value, "true") == 0) {

		cattle_configuration_set_debug_is_enabled (configuration,
		                                           TRUE);
	}
	else if (g_utf8_collate (value, "no") == 0 ||
	         g_utf8_collate (value, "false") == 0) {

		cattle_configuration_set_debug_is_enabled (configuration,
		                                           FALSE);
	}
	else {

		g_set_error (error,
		             G_OPTION_ERROR,
		             G_OPTION_ERROR_FAILED,
		             "Invalid argument %s for %s",
		             value,
		             option_name);

		return FALSE;
	}

	return TRUE;
}

/**
 * entries:
 *
 * Commandline options definition.
 */
GOptionEntry entries[N_OPTIONS + 1] =
{
	{
		"enable-debugging",
		'd',
		G_OPTION_FLAG_OPTIONAL_ARG,
		G_OPTION_ARG_CALLBACK,
		parse_debugging,
		"Enable debugging support",
		NULL
	},
	{
		"store",
		's',
		0,
		G_OPTION_ARG_CALLBACK,
		parse_store,
		"What to store when an EOF is read from the input",
		"zero|eof|same"
	},
	{NULL}
};
