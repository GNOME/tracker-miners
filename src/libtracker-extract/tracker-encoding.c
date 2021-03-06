/*
 * Copyright (C) 2011 Nokia <ivan.frade@nokia.com>
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
#include "tracker-encoding.h"

#ifdef HAVE_ENCA
#include "tracker-encoding-enca.h"
#endif

#ifdef HAVE_LIBICU_CHARSET_DETECTION
#include "tracker-encoding-libicu.h"
#endif

gboolean
tracker_encoding_can_guess (void)
{
#if defined (HAVE_ENCA) || defined (HAVE_LIBICU_CHARSET_DETECTION)
	return TRUE;
#else
	return FALSE;
#endif
}

gchar *
tracker_encoding_guess (const gchar *buffer,
                        gsize        size,
                        gdouble     *confidence)
{
	gchar *encoding = NULL;
	gdouble conf = 1;

#ifdef HAVE_LIBICU_CHARSET_DETECTION
	encoding = tracker_encoding_guess_icu (buffer, size, &conf);
#endif /* HAVE_LIBICU_CHARSET_DETECTION */

#ifdef HAVE_ENCA
	if (!encoding || conf < 0.5) {
		conf = 1;
		g_free (encoding);
		encoding = tracker_encoding_guess_enca (buffer, size);
	}
#endif /* HAVE_ENCA */

	if (confidence)
		*confidence = conf;

	return encoding;
}
