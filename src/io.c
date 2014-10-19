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

#include <readline/readline.h>
#include <stdlib.h>
#include "io.h"

/**
 * INPUT_BUFFER_SIZE:
 *
 * Requested size for each read operation.
 */
#define INPUT_BUFFER_SIZE 1024

/**
 * prompt:
 *
 * The last line of output. Must be passed to readline() for line
 * editing to work as expected.
 */
static gchar *prompt = NULL;

/**
 * load_file_contents:
 * @file: a #GFile
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Load the contents of @file, with good error checking and proper
 * handling of magic bytes.
 *
 * Returns: (transfer full): the contents of @file, or %NULL
 */
CattleBuffer*
load_file_contents (GFile   *file,
                    GError **error)
{
	CattleBuffer *buffer;
	GError       *inner_error;
	gchar        *contents;
	gchar        *temp;
	gboolean      success;
	gsize         len;

	g_return_val_if_fail (G_IS_FILE (file), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Load file contents */
	inner_error = NULL;
	success = g_file_load_contents (file,
	                                NULL,
	                                &contents,
	                                &len,
	                                NULL, /* No etag */
	                                &inner_error);

	if (!success) {

		g_propagate_error (error,
		                   inner_error);

		return NULL;
	}

#if 0
	/* Validate the contents as UTF-8 */
	success = g_utf8_validate (contents,
	                           len,
	                           NULL);

	if (!success) {

		g_set_error_literal (error,
		                     G_IO_ERROR,
		                     G_IO_ERROR_FAILED,
		                     "Invalid UTF-8");

		return NULL;
	}
#endif

	/* Detect and handle magic bytes */
	if (g_str_has_prefix (contents, "#!")) {

		/* Look for the beginning of the second line */
		temp = g_utf8_strchr (contents,
		                      len,
		                      (gunichar) '\n');
		temp = g_utf8_next_char (temp);

		/* Strip the first line */
		temp = g_strdup_printf ("%s", temp);
		g_free (contents);
		contents = temp;

		/* */
		len = strlen (contents);
	}

	buffer = cattle_buffer_new (len);
	cattle_buffer_set_contents (buffer, contents);

	g_free (contents);

	return buffer;
}

/**
 * output_handler:
 *
 * Dump interpreter's output to a GOutputStream.
 */
gboolean
output_handler (CattleInterpreter  *interpreter,
                gint8               output,
                gpointer            data,
                GError            **error)
{
	GOutputStream *stream;
	GError        *inner_error;
	gchar         *temp;

	if (G_IS_OUTPUT_STREAM (data)) {

		/* Writing to a file: get the stream */
		stream = G_OUTPUT_STREAM (data);

		inner_error = NULL;
		g_output_stream_write (stream,
		                       &output,
		                       1,
		                       NULL,
		                       &inner_error);

		if (inner_error != NULL) {

			g_propagate_error (error,
			                   inner_error);

			return FALSE;
		}
	}
	else {

		/* Write to standard output */
		g_print ("%c", output);
	}

	/* Create an empty string for prompt */
	if (prompt == NULL) {

		prompt = g_strdup ("");
	}

	/* Not the end of the current line */
	if (output != '\n') {

		/* Append the output character to the prompt string */
		temp = g_strdup_printf ("%s%c", prompt,
		                                output);
		g_free (prompt);
		prompt = temp;
	}
	else {

		/* End of current line: reset the prompt */
		g_free (prompt);
		prompt = NULL;
	}

	return TRUE;
}

/**
 * input_handler:
 *
 * Read interpreter's input from a GInputStream.
 */
gboolean
input_handler (CattleInterpreter  *interpreter,
               gpointer            data,
               GError            **error)
{
	CattleBuffer *input;
	GInputStream *stream;
	GError       *inner_error;
	gchar         buffer[INPUT_BUFFER_SIZE];
	gssize        size;

	stream = G_INPUT_STREAM (data);

	inner_error = NULL;
	size = g_input_stream_read (stream,
	                            buffer,
	                            INPUT_BUFFER_SIZE,
	                            NULL,
	                            &inner_error);

	if (inner_error != NULL)
	{
		g_propagate_error (error,
		                   inner_error);

		return FALSE;
	}

	/* Copy the input to a CattleBuffer */
	input = cattle_buffer_new (size);
	cattle_buffer_set_contents (input, buffer);

	/* Feed the interpreter with the new input */
	cattle_interpreter_feed (interpreter, input);

	g_object_unref (input);

	return TRUE;
}

/**
 * input_handler_interactive:
 *
 * Retrieve input from the user interactively.
 */
gboolean
input_handler_interactive (CattleInterpreter  *interpreter,
                           gpointer            data,
                           GError            **error)
{
	CattleBuffer *input;
	gchar        *buffer;
	gulong        size;

	/* Use readline to fetch user input. readline is notified of
	 * the fact that it should not handle the prompt itself, as there
	 * is actually no prompt, but there could be some program output */
	rl_already_prompted = 1;
	buffer = readline (prompt);

	/* Reset prompt after input, because the cursor is certainly at
	 * the beginning of a new line */
	g_free (prompt);
	prompt = NULL;

	if (buffer == NULL)
	{
		input = cattle_buffer_new (0);
	}
	else
	{
		/* Size of the input */
		size = strlen (buffer) + 1;

		/* Copy the input, overwriting the trailing null byte
		 * with the newline that's been stripped by readline */
		input = cattle_buffer_new (size);
		cattle_buffer_set_contents (input, buffer);
		cattle_buffer_set_value (input, size - 1, '\n');

		free (buffer);
	}

	/* Feed the interpreter with the new input */
	cattle_interpreter_feed (interpreter, input);

	g_object_unref (input);

	return TRUE;
}
