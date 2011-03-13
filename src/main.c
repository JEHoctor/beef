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

#include <glib.h>
#include <glib-object.h>
#include <cattle/cattle.h>
#include <config.h>
#include "io.h"
#include "options.h"

/**
 * display_error:
 * @program: program path
 * @file: file path
 * @error: error message
 *
 * Display an error encountered by @program while processing @file.
 */
void
display_error (const gchar *program,
               const gchar *file,
               const gchar *error)
{
	g_printerr ("%s: %s: %s\n", program,
	                            file,
	                            error);
}

/**
 * main:
 *
 * Application entry point.
 */
gint
main (gint    argc,
      gchar **argv)
{
	CattleInterpreter *interpreter;
	CattleProgram *program;
	CattleConfiguration *configuration;
	GFile *file;
	GOptionContext *context;
	GOptionGroup *group;
	GError *error;
	gchar *program_name;
	gchar *contents;
	gboolean success;

	g_set_prgname ("beef");
	g_type_init ();

	program_name = argv[0];

	/* Set up a configuration group for commanline options */
	configuration = cattle_configuration_new ();
	context = g_option_context_new ("FILE - Flexible Brainfuck interpreter");
	group = g_option_group_new ("",
	                            "",
	                            "",
	                            configuration,
	                            NULL);
	g_option_group_add_entries (group, entries);
	g_option_context_set_main_group (context, group);

	/* Parse commandline options */
	error = NULL;
	success = g_option_context_parse (context, &argc, &argv, &error);

	if (!success) {

		display_error (program_name,
		               "",
		               error->message);

		return 1;
	}

	/* Make sure a file has been specified on the commandline */
	if (argc != 2) {

		g_printerr ("Usage:\n  %s [OPTION...] FILE\n", g_get_prgname ());

		return 1;
	}

	/* Load file contents */
	file = g_file_new_for_commandline_arg (argv[1]);

	error = NULL;
	contents = load_file_contents (file,
								   &error);
	g_object_unref (file);

	if (contents == NULL) {

		display_error (program_name,
		               argv[1],
		               error->message);

		return 1;
	}

	interpreter = cattle_interpreter_new ();
	program = cattle_interpreter_get_program (interpreter);
	cattle_interpreter_set_configuration (interpreter, configuration);

	/* Load program */
	error = NULL;
	success = cattle_program_load (program,
	                               contents,
	                               &error);
	g_object_unref (program);
	g_free (contents);

	if (!success) {

		display_error (program_name,
		               argv[1],
		               error->message);

		g_object_unref (interpreter);

		return 1;
	}

	/* Run program */
	error = NULL;
	success = cattle_interpreter_run (interpreter,
	                                  &error);

	if (!success) {

		display_error (program_name,
		               argv[1],
		               error->message);

		g_object_unref (interpreter);

		return 1;
	}

	g_object_unref (interpreter);

	return 0;
}
