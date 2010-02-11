/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2009, Nokia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __LIBTRACKER_EXTRACT_XMP_H__
#define __LIBTRACKER_EXTRACT_XMP_H__

#include <glib.h>

#include <libtracker-client/tracker-sparql-builder.h>

G_BEGIN_DECLS

typedef struct {
	/* NS_DC */
	gchar *title;
	gchar *rights;
	gchar *creator;
	gchar *description;
	gchar *date;
	gchar *keywords;
	gchar *subject;

	gchar *publisher;
	gchar *contributor;
	gchar *type;
	gchar *format;
	gchar *identifier;
	gchar *source;
	gchar *language;
	gchar *relation;
	gchar *coverage;

	/* NS_CC */
	gchar *license;

	/* NS_PDF */
	gchar *pdf_title;
	gchar *pdf_keywords;

	/* NS_EXIF */
	gchar *title2;
	gchar *time_original;
	gchar *artist;
	gchar *make;
	gchar *model;
	gchar *orientation;
	gchar *flash;
	gchar *metering_mode;
	gchar *exposure_time;
	gchar *fnumber;
	gchar *focal_length;

	gchar *iso_speed_ratings;
	gchar *white_balance;
	gchar *copyright;

	/* TODO NS_XAP*/
	gchar *rating;

	/* TODO NS_IPTC4XMP */
	/* TODO NS_PHOTOSHOP */
	gchar *address;
	gchar *country;
	gchar *state;
	gchar *city;
} TrackerXmpData;

gboolean tracker_xmp_read  (const gchar          *buffer,
                            size_t                len,
                            const gchar          *uri,
                            TrackerXmpData       *data);
gboolean tracker_xmp_apply (TrackerSparqlBuilder *metadata,
                            const gchar          *uri,
                            TrackerXmpData       *data);

G_END_DECLS

#endif /* __LIBTRACKER_EXTRACT_XMP_H__ */
