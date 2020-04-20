/*
 * Copyright (C) 2019, Sam Thursfield <sam@afuera.me.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config-miners.h"

#include <glib.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-extract/tracker-module-manager.h>

#define assert_path_basename(path, cmp, expected_value) {    \
	g_autofree gchar *basename = g_path_get_basename (path); \
	g_assert_cmpstr (basename, cmp, expected_value);         \
}

static gchar *
get_test_rules_dir (void)
{
	return g_build_path (G_DIR_SEPARATOR_S, TOP_SRCDIR, "tests", "libtracker-extract", "test-extract-rules", NULL);
}

static void
init_module_manager (void) {
	gboolean success;
	g_autofree gchar *test_rules_dir = NULL;

	test_rules_dir = get_test_rules_dir ();
	g_setenv ("TRACKER_EXTRACTOR_RULES_DIR", test_rules_dir, TRUE);

	success = tracker_extract_module_manager_init ();
	g_assert (success);
}

static void
test_extract_rules (void)
{
	GList *l;

	// The audio/* rule should match this, but the image/* rule should not.
	l = tracker_extract_module_manager_get_matching_rules("audio/mpeg");

	g_assert_cmpint (g_list_length (l), ==, 1);
	assert_path_basename (l->data, ==, "90-audio-generic.rule");

	// The image/* rule should match this, but the audio/* rule should not.
	l = tracker_extract_module_manager_get_matching_rules("image/png");

	g_assert_cmpint (g_list_length (l), ==, 1);
	assert_path_basename (l->data, ==, "90-image-generic.rule");

	// No rule should match this.
	l = tracker_extract_module_manager_get_matching_rules("text/generic");
	g_assert_cmpint (g_list_length (l), ==, 0);

	// The image/x-blocked MIME type is explicitly blocked, so no rule should match.
	l = tracker_extract_module_manager_get_matching_rules("image/x-blocked");
	g_assert_cmpint (g_list_length (l), ==, 0);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);
	init_module_manager ();

	g_test_add_func ("/libtracker-extract/module-manager/extract-rules",
	                 test_extract_rules);
	return g_test_run ();
}
