/*
 * Copyright (C) 2009, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Authors: Philip Van Hoof <philip@codeminded.be>
 */

#include "config-miners.h"

#include <locale.h>
#include <string.h>
#include <math.h>

#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>

#include <glib-object.h>
#include <gio/gio.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-ontologies.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-writeback-file.h"

#define TRACKER_TYPE_WRITEBACK_XMP (tracker_writeback_xmp_get_type ())

typedef struct TrackerWritebackXMP TrackerWritebackXMP;
typedef struct TrackerWritebackXMPClass TrackerWritebackXMPClass;

struct TrackerWritebackXMP {
	TrackerWritebackFile parent_instance;
};

struct TrackerWritebackXMPClass {
	TrackerWritebackFileClass parent_class;
};

static GType                tracker_writeback_xmp_get_type     (void) G_GNUC_CONST;
static gboolean             writeback_xmp_write_file_metadata  (TrackerWritebackFile  *writeback_file,
                                                                GFile                 *file,
                                                                TrackerResource       *resource,
                                                                GCancellable          *cancellable,
                                                                GError               **error);
static const gchar * const *writeback_xmp_content_types        (TrackerWritebackFile  *writeback_file);

G_DEFINE_DYNAMIC_TYPE (TrackerWritebackXMP, tracker_writeback_xmp, TRACKER_TYPE_WRITEBACK_FILE);

static void
tracker_writeback_xmp_class_init (TrackerWritebackXMPClass *klass)
{
	TrackerWritebackFileClass *writeback_file_class = TRACKER_WRITEBACK_FILE_CLASS (klass);

	xmp_init ();

	writeback_file_class->write_file_metadata = writeback_xmp_write_file_metadata;
	writeback_file_class->content_types = writeback_xmp_content_types;
}

static void
tracker_writeback_xmp_class_finalize (TrackerWritebackXMPClass *klass)
{
	xmp_terminate ();
}

static void
tracker_writeback_xmp_init (TrackerWritebackXMP *wbx)
{
}

static const gchar * const *
writeback_xmp_content_types (TrackerWritebackFile *wbf)
{
	static const gchar *content_types[] = {
		"image/png",   /* .png files */
		"sketch/png",  /* .sketch.png files on Maemo*/
		"image/jpeg",  /* .jpg & .jpeg files */
		"image/tiff",  /* .tiff & .tif files */
		"video/mp4",   /* .mp4 files */
		"video/3gpp",  /* .3gpp files */
                "image/gif",   /* .gif files */
		NULL
	};

	/* "application/pdf"                  .pdf files
	   "application/rdf+xml"              .xmp files
	   "application/postscript"           .ps files
	   "application/x-shockwave-flash"    .swf files
	   "video/quicktime"                  .mov files
	   "video/mpeg"                       .mpeg & .mpg files
	   "audio/mpeg"                       .mp3, etc files */

	return content_types;
}

static void
write_gps_coord (XmpPtr       xmp,
                 const gchar *label,
                 gdouble      coord,
                 gchar        more,
                 gchar        less)
{
	double degrees, minutes;
	gchar *val;

	minutes = modf (coord, &degrees);

	val = g_strdup_printf ("%3d,%f%c",
	                       (int) fabs (degrees),
	                       minutes,
	                       coord >= 0 ? more : less);

	xmp_delete_property (xmp, NS_EXIF, label);
	xmp_set_property (xmp, NS_EXIF, label, val, 0);
	g_free (val);
}

static gboolean
writeback_xmp_write_file_metadata (TrackerWritebackFile  *wbf,
                                   GFile                 *file,
                                   TrackerResource       *resource,
                                   GCancellable          *cancellable,
                                   GError               **error)
{
	GList *properties, *l;
	gchar *path;
	XmpFilePtr xmp_files;
	XmpPtr xmp;
#ifdef DEBUG_XMP
	XmpStringPtr str;
#endif

	path = g_file_get_path (file);

	xmp_files = xmp_files_open_new (path, XMP_OPEN_FORUPDATE);

	if (!xmp_files) {
		g_set_error (error,
		             G_IO_ERROR,
		             G_IO_ERROR_FAILED,
		             "Can't open '%s' for update with Exempi (Exempi error code = %d)",
		             path,
		             xmp_get_error ());
		g_free (path);
		return FALSE;
	}

	xmp = xmp_files_get_new_xmp (xmp_files);

	if (!xmp) {
		xmp = xmp_new_empty ();
	}

#ifdef DEBUG_XMP
	str = xmp_string_new ();
	g_print ("\nBEFORE: ---- \n");
	xmp_serialize_and_format (xmp, str, 0, 0, "\n", "\t", 1);
	g_print ("%s\n", xmp_string_cstr (str));
	xmp_string_free (str);
#endif

	properties = tracker_resource_get_properties (resource);

	for (l = properties; l; l = l->next) {
		const gchar *prop = l->data;

		if (g_strcmp0 (prop, "nie:title") == 0) {
			const gchar *title;

			title = tracker_resource_get_first_string (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "Title");
			xmp_set_property (xmp, NS_EXIF, "Title", title, 0);
			xmp_delete_property (xmp, NS_DC, "title");
			xmp_set_property (xmp, NS_DC, "title", title, 0);
		}

		if (g_strcmp0 (prop, "nco:creator") == 0) {
			TrackerResource *creator;
			const gchar *name = NULL;

			creator = tracker_resource_get_first_relation (resource, prop);

			if (creator) {
				name = tracker_resource_get_first_string (creator,
				                                          "nco:fullname");
			}

			if (name) {
				xmp_delete_property (xmp, NS_DC, "creator");
				xmp_set_property (xmp, NS_DC, "creator", name, 0);
			}
		}

		if (g_strcmp0 (prop, "nco:contributor") == 0) {
			TrackerResource *contributor;
			const gchar *name = NULL;

			contributor = tracker_resource_get_first_relation (resource, prop);

			if (contributor) {
				name = tracker_resource_get_first_string (contributor,
				                                          "nco:fullname");
			}

			if (name) {
				xmp_delete_property (xmp, NS_DC, "contributor");
				xmp_set_property (xmp, NS_DC, "contributor", name, 0);
			}
		}

		if (g_strcmp0 (prop, "nie:description") == 0) {
			const gchar *description;

			description = tracker_resource_get_first_string (resource, prop);
			xmp_delete_property (xmp, NS_DC, "description");
			xmp_set_property (xmp, NS_DC, "description", description, 0);
		}

		if (g_strcmp0 (prop, "nie:copyright") == 0) {
			const gchar *copyright;

			copyright = tracker_resource_get_first_string (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "Copyright");
			xmp_set_property (xmp, NS_EXIF, "Copyright", copyright, 0);
		}

		if (g_strcmp0 (prop, "nie:comment") == 0) {
			const gchar *comment;

			comment = tracker_resource_get_first_string (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "UserComment");
			xmp_set_property (xmp, NS_EXIF, "UserComment", comment, 0);
		}

		if (g_strcmp0 (prop, "nie:keyword") == 0) {
			GList *keywords, *k;
			GString *keyword_str = NULL;

			keywords = tracker_resource_get_values (resource, prop);

			for (k = keywords; k; k = k->next) {
				GValue *value;

				value = k->data;

				if (G_VALUE_HOLDS_STRING (value)) {
					const gchar *str = g_value_get_string (value);

					if (!keywords)
						keyword_str = g_string_new (str);
					else
						g_string_append_printf (keyword_str, ", %s", str);
				}
			}

			if (keyword_str->len > 0) {
				xmp_delete_property (xmp, NS_DC, "subject");
				xmp_set_property (xmp, NS_DC, "subject", keyword_str->str, 0);
			}

			g_string_free (keyword_str, TRUE);
			g_list_free (keywords);
		}

		if (g_strcmp0 (prop, "nie:contentCreated") == 0) {
			const gchar *created;

			created = tracker_resource_get_first_string (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "Date");
			xmp_set_property (xmp, NS_EXIF, "Date", created, 0);
			xmp_delete_property (xmp,  NS_DC, "date");
			xmp_set_property (xmp,  NS_DC, "date", created, 0);
		}

		if (g_strcmp0 (prop, "nfo:orientation") == 0) {
			const gchar *orientation;

			orientation = tracker_resource_get_first_uri (resource, prop);

			xmp_delete_property (xmp, NS_EXIF, "Orientation");

			if (g_strcmp0 (orientation, "nfo:orientation-top") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "top - left", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-top-mirror") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "top - right", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-bottom") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "bottom - left", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-bottom-mirror") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "bottom - right", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-left-mirror") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "left - top", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-right") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "right - top", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-right-mirror") == 0) {
					xmp_set_property (xmp, NS_EXIF, "Orientation", "right - bottom", 0);
			} else if (g_strcmp0 (orientation, "nfo:orientation-left") == 0) {
				xmp_set_property (xmp, NS_EXIF, "Orientation", "left - bottom", 0);
			}
		}

#ifdef SET_TYPICAL_CAMERA_FIELDS
		/* Default we don't do this, we shouldn't overwrite fields that are
		 * typically set by the camera itself. What do we know (better) than
		 * the actual camera did, anyway? Even if the user overwrites them in
		 * the RDF store ... (does he know what he's doing anyway?) */

		if (g_strcmp0 (prop, "nmm:meteringMode") == 0) {
			const gchar *metering;

			metering = tracker_resource_get_first_uri (resource, prop);

			xmp_delete_property (xmp, NS_EXIF, "MeteringMode");

			/* 0 = Unknown
			   1 = Average
			   2 = CenterWeightedAverage
			   3 = Spot
			   4 = MultiSpot
			   5 = Pattern
			   6 = Partial
			   255 = other  */

			if (g_strcmp0 (metering, "nmm:metering-mode-center-weighted-average") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "0", 0);
			} else if (g_strcmp0 (metering, "nmm:metering-mode-average") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "1", 0);
			} else if (g_strcmp0 (metering, "nmm:metering-mode-spot") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "3", 0);
			} else if (g_strcmp0 (metering, "nmm:metering-mode-multispot") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "4", 0);
			} else if (g_strcmp0 (metering, "nmm:metering-mode-pattern") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "5", 0);
			} else if (g_strcmp0 (metering, "nmm:metering-mode-partial") == 0) {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "6", 0);
			} else {
				xmp_set_property (xmp, NS_EXIF, "MeteringMode", "255", 0);
			}
		}

		if (g_strcmp0 (prop, "nmm:whiteBalance") == 0) {
			const gchar *balance;

			balance = tracker_resource_get_first_uri (resource, prop);

			xmp_delete_property (xmp, NS_EXIF, "WhiteBalance");

			if (g_strcmp0 (balance, "nmm:white-balance-auto") == 0) {
				/* 0 = Auto white balance
				 * 1 = Manual white balance */
				xmp_set_property (xmp, NS_EXIF, "WhiteBalance", "0", 0);
			} else {
				xmp_set_property (xmp, NS_EXIF, "WhiteBalance", "1", 0);
			}
		}

		if (g_strcmp0 (prop, "nmm:flash") == 0) {
			const gchar *flash;

			flash = tracker_resource_get_first_uri (resource, prop);

			xmp_delete_property (xmp, NS_EXIF, "Flash");

			if (g_strcmp0 (flash, "nmm:flash-on") == 0) {
				/* 0 = Flash did not fire
				 * 1 = Flash fired */
				xmp_set_property (xmp, NS_EXIF, "Flash", "1", 0);
			} else {
				xmp_set_property (xmp, NS_EXIF, "Flash", "0", 0);
			}
		}

		/* TODO: Don't write value as-is here, read xmp_specification.pdf,
		   page 84 (bottom). */
		if (g_strcmp0 (prop, "nmm:focalLength") == 0) {
			gdouble focal_length;

			focal_length = tracker_resource_get_first_double (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "FocalLength");
			xmp_set_property (xmp, NS_EXIF, "FocalLength", focal_length, 0);
		}

		if (g_strcmp0 (prop, "nmm:exposureTime") == 0) {
			gdouble exposure;

			exposure = tracker_resource_get_first_double (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "ExposureTime");
			xmp_set_property (xmp, NS_EXIF, "ExposureTime", exposure, 0);
		}

		if (g_strcmp0 (prop, "nmm:isoSpeed") == 0) {
			gdouble speed;

			speed = tracker_resource_get_first_double (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "ISOSpeedRatings");
			xmp_set_property (xmp, NS_EXIF, "ISOSpeedRatings", speed, 0);
		}

		if (g_strcmp0 (prop, "nmm:fnumber") == 0) {
			gdouble fnumber;

			fnumber = tracker_resource_get_first_double (resource, prop);
			xmp_delete_property (xmp, NS_EXIF, "FNumber");
			xmp_set_property (xmp, NS_EXIF, "FNumber", fnumber, 0);
		}

		if (g_strcmp0 (prop, "nfo:equipment") == 0) {
			TrackerResource *equipment;
			const gchar *maker, *model;

			equipment = tracker_resource_get_first_relation (resource, prop);

			if (equipment) {
				maker = tracker_resource_get_first_string (equipment,
				                                           "nfo:manufacturer");
				model = tracker_resource_get_first_string (equipment,
				                                           "nfo:model");
			}

			if (maker) {
				xmp_delete_property (xmp, NS_EXIF, "Make");
				xmp_set_property (xmp, NS_EXIF, "Make", maker, 0);
			}

			if (model) {
				xmp_delete_property (xmp, NS_EXIF, "Model");
				xmp_set_property (xmp, NS_EXIF, "Model", model, 0);
			}
		}
#endif /* SET_TYPICAL_CAMERA_FIELDS */

		if (g_strcmp0 (prop, "nfo:heading") == 0) {
			gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
			gdouble heading;

			heading = tracker_resource_get_first_double (resource, prop);
			g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, heading);
			xmp_delete_property (xmp, NS_EXIF, "GPSImgDirection");
			xmp_set_property (xmp, NS_EXIF, "GPSImgDirection", buf, 0);
		}

		if (g_strcmp0 (prop, "slo:location") == 0) {
			TrackerResource *location;
			gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
			const gchar *str;
			gdouble value;

			location = tracker_resource_get_first_relation (resource, prop);

			/* TODO: A lot of these location fields are pretty vague and ambigious.
			 * We should go through them one by one and ensure that all of them are
			 * used sanely */
			str = tracker_resource_get_first_string (location, "nco:locality");
			xmp_delete_property (xmp, NS_IPTC4XMP, "City");
			xmp_delete_property (xmp, NS_PHOTOSHOP, "City");
			if (str != NULL) {
				xmp_set_property (xmp, NS_IPTC4XMP, "City", str, 0);
				xmp_set_property (xmp, NS_PHOTOSHOP, "City", str, 0);
			}

			str = tracker_resource_get_first_string (location, "nco:region");
			xmp_delete_property (xmp, NS_IPTC4XMP, "State");
			xmp_delete_property (xmp, NS_IPTC4XMP, "Province");
			xmp_delete_property (xmp, NS_PHOTOSHOP, "State");
			if (str != NULL) {
				xmp_set_property (xmp, NS_IPTC4XMP, "State", str, 0);
				xmp_set_property (xmp, NS_IPTC4XMP, "Province", str, 0);
				xmp_set_property (xmp, NS_PHOTOSHOP, "State", str, 0);
			}

			str = tracker_resource_get_first_string (location, "nco:streetAddress");
			xmp_delete_property (xmp, NS_IPTC4XMP, "SubLocation");
			xmp_delete_property (xmp, NS_PHOTOSHOP, "Location");
			if (str != NULL) {
				xmp_set_property (xmp, NS_IPTC4XMP, "SubLocation", str, 0);
				xmp_set_property (xmp, NS_PHOTOSHOP, "Location", str, 0);
			}

			str = tracker_resource_get_first_string (location, "nco:country");
			xmp_delete_property (xmp, NS_PHOTOSHOP, "Country");
			xmp_delete_property (xmp, NS_IPTC4XMP, "Country");
			xmp_delete_property (xmp, NS_IPTC4XMP, "PrimaryLocationName");
			xmp_delete_property (xmp, NS_IPTC4XMP, "CountryName");
			if (str != NULL) {
				xmp_set_property (xmp, NS_PHOTOSHOP, "Country", str, 0);
				xmp_set_property (xmp, NS_IPTC4XMP, "Country", str, 0);
				xmp_set_property (xmp, NS_IPTC4XMP, "PrimaryLocationName", str, 0);
				xmp_set_property (xmp, NS_IPTC4XMP, "CountryName", str, 0);
			}

			value = tracker_resource_get_first_double (location, "slo:altitude");
			xmp_delete_property (xmp, NS_EXIF, "GPSAltitude");
			g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
			xmp_set_property (xmp, NS_EXIF, "GPSAltitude", buf, 0);

			value = tracker_resource_get_first_double (location, "slo:longitude");
			write_gps_coord (xmp, "GPSLongitude", value, 'E', 'W');

			value = tracker_resource_get_first_double (location, "slo:latitude");
			write_gps_coord (xmp, "GPSLatitude", value, 'N', 'S');
		}
	}

#ifdef DEBUG_XMP
	g_print ("\nAFTER: ---- \n");
	str = xmp_string_new ();
	xmp_serialize_and_format (xmp, str, 0, 0, "\n", "\t", 1);
	g_print ("%s\n", xmp_string_cstr (str));
	xmp_string_free (str);
	g_print ("\n --------- \n");
#endif

	if (xmp_files_can_put_xmp (xmp_files, xmp)) {
		xmp_files_put_xmp (xmp_files, xmp);
	}

	/* Note: We don't currently use XMP_CLOSE_SAFEUPDATE because it uses
	 * a hidden temporary file in the same directory, which is then
	 * renamed to the final name. This triggers two events:
	 *  - DELETE(A) + MOVE(.hidden->A)
	 * and we really don't want the first DELETE(A) here
	 */
	xmp_files_close (xmp_files, XMP_CLOSE_NOOPTION);

	xmp_free (xmp);
	xmp_files_free (xmp_files);
	g_free (path);
	g_list_free (properties);

	return TRUE;
}

TrackerWriteback *
writeback_module_create (GTypeModule *module)
{
	tracker_writeback_xmp_register_type (module);

	return g_object_new (TRACKER_TYPE_WRITEBACK_XMP, NULL);
}

const gchar * const *
writeback_module_get_rdf_types (void)
{
	static const gchar *rdf_types[] = {
		TRACKER_PREFIX_NFO "Image",
		TRACKER_PREFIX_NFO "Audio",
		TRACKER_PREFIX_NFO "Video",
		NULL
	};

	return rdf_types;
}
