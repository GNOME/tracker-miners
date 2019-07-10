/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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
 */

#include "config-miners.h"

#include <string.h>

#include <gexiv2/gexiv2.h>

#include <libtracker-extract/tracker-extract.h>
#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-main.h"

#define CMS_PER_INCH            2.54
#define EXIF_DATE_FORMAT        "%Y:%m:%d %H:%M:%S"

enum {
	EXIF_FLASH_NONE = 0x0000,
	EXIF_FLASH_FIRED_MISSING_STROBE = 0x0005,
	EXIF_FLASH_DID_NOT_FIRE_COMPULSORY_ON = 0x0008,
	EXIF_FLASH_DID_NOT_FIRE_COMPULSORY_OFF = 0x0010,
	EXIF_FLASH_DID_NOT_FIRE_AUTO = 0x0018,
	EXIF_FLASH_DID_NOT_FIRE_AUTO_RED_EYE_REDUCTION = 0x0058,
};

enum {
	EXIF_METERING_MODE_UNKNOWN = 0,
	EXIF_METERING_MODE_AVERAGE = 1,
	EXIF_METERING_MODE_CENTER_WEIGHTED_AVERAGE = 2,
	EXIF_METERING_MODE_SPOT = 3,
	EXIF_METERING_MODE_MULTISPOT = 4,
	EXIF_METERING_MODE_PATTERN = 5,
	EXIF_METERING_MODE_PARTIAL = 6,
	EXIF_METERING_MODE_OTHER = 255,
};

typedef struct {
	gchar *artist;
	gchar *copyright;
	gchar *description;
	gchar *document_name;
	gchar *flash;
	gchar *gps_altitude;
	gchar *gps_direction;
	gchar *gps_latitude;
	gchar *gps_longitude;
	gchar *make;
	gchar *metering_mode;
	gchar *model;
	gchar *time;
	gchar *time_original;
	gchar *user_comment;
	gchar *white_balance;
	gchar *x_resolution;
	gchar *y_resolution;
	gdouble exposure_time;
	gdouble fnumber;
	gdouble focal_length;
	gdouble iso_speed_ratings;
	gint resolution_unit;
} RawExifData;

static void
raw_exif_data_free (RawExifData *ed)
{
	g_return_if_fail (ed != NULL);

	g_free (ed->artist);
	g_free (ed->copyright);
	g_free (ed->description);
	g_free (ed->document_name);
	g_free (ed->flash);
	g_free (ed->gps_altitude);
	g_free (ed->gps_direction);
	g_free (ed->gps_latitude);
	g_free (ed->gps_longitude);
	g_free (ed->make);
	g_free (ed->metering_mode);
	g_free (ed->model);
	g_free (ed->time);
	g_free (ed->time_original);
	g_free (ed->user_comment);
	g_free (ed->white_balance);
	g_free (ed->x_resolution);
	g_free (ed->y_resolution);

	g_free (ed);
}

static RawExifData *
raw_exif_data_new (void)
{
	RawExifData *ed;

	ed = g_new0 (RawExifData, 1);
	ed->exposure_time = -1.0;
	ed->fnumber = -1.0;
	ed->focal_length = -1.0;
	ed->iso_speed_ratings = -1.0;
	ed->resolution_unit = -1;
	return ed;
}

static gchar *
convert_exiv2_orientation_to_nfo (GExiv2Orientation orientation)
{
	gchar *retval = NULL;

	switch (orientation) {
	case GEXIV2_ORIENTATION_NORMAL:
		retval = g_strdup ("nfo:orientation-top");
		break;
	case GEXIV2_ORIENTATION_HFLIP:
		retval = g_strdup ("nfo:orientation-top-mirror");
		break;
	case GEXIV2_ORIENTATION_ROT_180:
		retval = g_strdup ("nfo:orientation-bottom");
		break;
	case GEXIV2_ORIENTATION_VFLIP:
		retval = g_strdup ("nfo:orientation-bottom-mirror");
		break;
	case GEXIV2_ORIENTATION_ROT_90_HFLIP:
		retval = g_strdup ("nfo:orientation-left-mirror");
		break;
	case GEXIV2_ORIENTATION_ROT_90:
		retval = g_strdup ("nfo:orientation-right");
		break;
	case GEXIV2_ORIENTATION_ROT_90_VFLIP:
		retval = g_strdup ("nfo:orientation-right-mirror");
		break;
	case GEXIV2_ORIENTATION_ROT_270:
		retval = g_strdup ("nfo:orientation-left");
		break;
	default:
		retval = g_strdup ("nfo:orientation-top");
		break;
	}

	return retval;
}

static gchar *
parse_flash (gushort flash_value)
{
	gchar *flash = NULL;

	switch (flash_value) {
	case EXIF_FLASH_NONE:
	case EXIF_FLASH_FIRED_MISSING_STROBE:
	case EXIF_FLASH_DID_NOT_FIRE_COMPULSORY_ON:
	case EXIF_FLASH_DID_NOT_FIRE_COMPULSORY_OFF:
	case EXIF_FLASH_DID_NOT_FIRE_AUTO:
	case EXIF_FLASH_DID_NOT_FIRE_AUTO_RED_EYE_REDUCTION:
		flash = g_strdup ("nmm:flash-off");
		break;
	default:
		flash = g_strdup ("nmm:flash-on");
		break;
	}

	return flash;
}

static gchar *
parse_metering_mode (gushort metering_mode_value)
{
	gchar *metering_mode = NULL;

	switch (metering_mode_value) {
	case EXIF_METERING_MODE_AVERAGE:
		metering_mode = g_strdup ("nmm:metering-mode-average");
		break;
	case EXIF_METERING_MODE_CENTER_WEIGHTED_AVERAGE:
		metering_mode = g_strdup ("nmm:metering-mode-center-weighted-average");
		break;
	case EXIF_METERING_MODE_SPOT:
		metering_mode = g_strdup ("nmm:metering-mode-spot");
		break;
	case EXIF_METERING_MODE_MULTISPOT:
		metering_mode = g_strdup ("nmm:metering-mode-multispot");
		break;
	case EXIF_METERING_MODE_PATTERN:
		metering_mode = g_strdup ("nmm:metering-mode-pattern");
		break;
	case EXIF_METERING_MODE_PARTIAL:
		metering_mode = g_strdup ("nmm:metering-mode-partial");
		break;
	case EXIF_METERING_MODE_UNKNOWN:
	case EXIF_METERING_MODE_OTHER:
	default:
		metering_mode = g_strdup ("nmm:metering-mode-other");
		break;
	}

	return metering_mode;
}

static gchar *
parse_white_balance (gushort white_balance_value)
{
	gchar *white_balance = NULL;

	if (white_balance_value == 0)
		white_balance = g_strdup ("nmm:white-balance-auto");
	else
		white_balance = g_strdup ("nmm:white-balance-manual");

	return white_balance;
}

static RawExifData *
parse_exif_data (GExiv2Metadata *metadata)
{
	RawExifData *ed = NULL;
	gchar *time = NULL;
	gchar *time_original = NULL;
	gdouble gps_altitude;
	gdouble gps_latitude;
	gdouble gps_longitude;
	gint exposure_time_den;
	gint exposure_time_nom;
	glong flash = G_MAXLONG;
	glong metering_mode = G_MAXLONG;
	glong white_balance = G_MAXLONG;

	ed = raw_exif_data_new ();

	if (!gexiv2_metadata_has_exif (metadata))
		goto out;

	ed->document_name = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.DocumentName");

	time = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.DateTime");
	if (time != NULL)
		ed->time = tracker_date_format_to_iso8601 (time, EXIF_DATE_FORMAT);

	time_original = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.DateTimeOriginal");
	if (time_original == NULL)
		time_original = gexiv2_metadata_get_tag_string (metadata, "Exif.Photo.DateTimeOriginal");
	if (time_original != NULL)
		ed->time_original = tracker_date_format_to_iso8601 (time_original, EXIF_DATE_FORMAT);

	ed->artist = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.Artist");
	ed->user_comment = gexiv2_metadata_get_tag_string (metadata, "Exif.Photo.UserComment");
	ed->description = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.ImageDescription");
	ed->make = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.Make");
	ed->model = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.Model");

	if (gexiv2_metadata_get_exposure_time (metadata, &exposure_time_nom, &exposure_time_den))
		ed->exposure_time = (gdouble) exposure_time_nom / (double) exposure_time_den;

	ed->fnumber = gexiv2_metadata_get_fnumber (metadata);

	if (gexiv2_metadata_has_tag (metadata, "Exif.Image.Flash"))
		flash = gexiv2_metadata_get_tag_long (metadata, "Exif.Image.Flash");
	else if (gexiv2_metadata_has_tag (metadata, "Exif.Photo.Flash"))
		flash = gexiv2_metadata_get_tag_long (metadata, "Exif.Photo.Flash");
	if (flash != G_MAXLONG)
		ed->flash = parse_flash ((gushort) flash);

	ed->focal_length = gexiv2_metadata_get_focal_length (metadata);

	if (gexiv2_metadata_has_tag (metadata, "Exif.Photo.ISOSpeedRatings"))
		ed->iso_speed_ratings = (gdouble) gexiv2_metadata_get_iso_speed (metadata);

	if (gexiv2_metadata_has_tag (metadata, "Exif.Image.MeteringMode"))
		metering_mode = gexiv2_metadata_get_tag_long (metadata, "Exif.Image.MeteringMode");
	else if (gexiv2_metadata_has_tag (metadata, "Exif.Photo.MeteringMode"))
		metering_mode = gexiv2_metadata_get_tag_long (metadata, "Exif.Photo.MeteringMode");
	if (metering_mode != G_MAXLONG)
		ed->metering_mode = parse_metering_mode ((gushort) metering_mode);

	if (gexiv2_metadata_has_tag (metadata, "Exif.Photo.WhiteBalance"))
		white_balance = gexiv2_metadata_get_tag_long (metadata, "Exif.Photo.WhiteBalance");
	if (white_balance != G_MAXLONG)
		ed->white_balance = parse_white_balance ((gushort) white_balance);

	ed->copyright = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.Copyright");

	if (gexiv2_metadata_has_tag (metadata, "Exif.Image.ResolutionUnit"))
		ed->resolution_unit = (gint) gexiv2_metadata_get_tag_long (metadata, "Exif.Image.ResolutionUnit");

	ed->x_resolution = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.XResolution");
	ed->y_resolution = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.YResolution");

	if (gexiv2_metadata_get_gps_altitude (metadata, &gps_altitude))
		ed->gps_altitude = g_strdup_printf ("%f", gps_altitude);

	if (gexiv2_metadata_get_gps_latitude (metadata, &gps_latitude))
		ed->gps_latitude = g_strdup_printf ("%f", gps_latitude);

	if (gexiv2_metadata_get_gps_longitude (metadata, &gps_longitude))
		ed->gps_longitude = g_strdup_printf ("%f", gps_longitude);

	ed->gps_direction = gexiv2_metadata_get_tag_string (metadata, "Exif.GPSInfo.GPSImgDirection");

out:
	g_free (time);
	g_free (time_original);
	return ed;
}

G_MODULE_EXPORT gboolean
tracker_extract_get_metadata (TrackerExtractInfo *info)
{
	GError *error;
	GFile *file;
	GExiv2Metadata *metadata = NULL;
	GExiv2Orientation orientation;
	RawExifData *ed = NULL;
	TrackerResource *resource = NULL;
	gboolean retval = FALSE;
	const gchar *time_content_created;
	gchar *filename = NULL;
	gchar *nfo_orientation = NULL;
	gchar *uri = NULL;
	gint height;
	gint width;

	metadata = gexiv2_metadata_new ();
	file = tracker_extract_info_get_file (info);
	filename = g_file_get_path (file);

	error = NULL;
	if (!gexiv2_metadata_open_path (metadata, filename, &error)) {
		g_warning ("Could not open %s for reading metadata: %s", filename, error->message);
		g_error_free (error);
		goto out;
	}

	resource = tracker_resource_new (NULL);
	tracker_resource_add_uri (resource, "rdf:type", "nfo:Image");
	tracker_resource_add_uri (resource, "rdf:type", "nmm:Photo");

	width = gexiv2_metadata_get_pixel_width (metadata);
	tracker_resource_set_int (resource, "nfo:width", width);
	height = gexiv2_metadata_get_pixel_height (metadata);
	tracker_resource_set_int (resource, "nfo:height", height);

	orientation = gexiv2_metadata_get_orientation (metadata);
	nfo_orientation = convert_exiv2_orientation_to_nfo (orientation);
	tracker_resource_set_uri (resource, "nfo:orientation", nfo_orientation);

	ed = parse_exif_data (metadata);

	if (ed->make != NULL || ed->model != NULL) {
		TrackerResource *equipment;

		equipment = tracker_extract_new_equipment (ed->make, ed->model);
		tracker_resource_set_relation (resource, "nfo:equipment", equipment);
		g_object_unref (equipment);
	}

	uri = g_file_get_uri (file);
	tracker_guarantee_resource_title_from_file (resource, "nie:title", ed->document_name, uri, NULL);

	if (ed->copyright != NULL)
		tracker_resource_set_string (resource, "nie:copyright", ed->copyright);

	if (ed->white_balance != NULL)
		tracker_resource_set_uri (resource, "nmm:whiteBalance", ed->white_balance);

	if (ed->fnumber != -1.0)
		tracker_resource_set_double (resource, "nmm:fnumber", ed->fnumber);

	if (ed->flash != NULL)
		tracker_resource_set_uri (resource, "nmm:flash", ed->flash);

	if (ed->focal_length != -1.0)
		tracker_resource_set_double (resource, "nmm:focalLength", ed->focal_length);

	if (ed->artist != NULL) {
		TrackerResource *artist;

		artist = tracker_extract_new_contact (ed->artist);
		tracker_resource_add_relation (resource, "nco:contributor", artist);
		g_object_unref (artist);
	}

	if (ed->exposure_time != -1.0)
		tracker_resource_set_double (resource, "nmm:exposureTime", ed->exposure_time);

	if (ed->iso_speed_ratings != -1.0)
		tracker_resource_set_double (resource, "nmm:isoSpeed", ed->iso_speed_ratings);

	time_content_created = tracker_coalesce_strip (2, ed->time, ed->time_original);
	tracker_guarantee_resource_date_from_file_mtime (resource, "nie:contentCreated", time_content_created, uri);

	if (ed->description != NULL)
		tracker_resource_set_string (resource, "nie:description", ed->description);

	if (ed->metering_mode != NULL)
		tracker_resource_set_uri (resource, "nmm:meteringMode", ed->metering_mode);

	if (ed->user_comment != NULL)
		tracker_guarantee_resource_utf8_string (resource, "nie:comment", ed->user_comment);

	if (ed->gps_altitude != NULL || ed->gps_latitude != NULL || ed->gps_longitude != NULL) {
		TrackerResource *location;

		location = tracker_extract_new_location (NULL,
		                                         NULL,
		                                         NULL,
		                                         NULL,
		                                         ed->gps_altitude,
		                                         ed->gps_latitude,
		                                         ed->gps_longitude);

		tracker_resource_set_relation (resource, "slo:location", location);
		g_object_unref (location);
	}

	if (ed->gps_direction != NULL)
		tracker_resource_set_string (resource, "nfo:heading", ed->gps_direction);

	if (ed->x_resolution != NULL) {
		gdouble value;

		if (ed->resolution_unit == EXIF_RESOLUTION_UNIT_PER_CENTIMETER)
			value = g_strtod (ed->x_resolution, NULL) * CMS_PER_INCH;
		else
			value = g_strtod (ed->x_resolution, NULL);

		tracker_resource_set_double (resource, "nfo:horizontalResolution", value);
	}

	if (ed->y_resolution != NULL) {
		gdouble value;

		if (ed->resolution_unit == EXIF_RESOLUTION_UNIT_PER_CENTIMETER)
			value = g_strtod (ed->y_resolution, NULL) * CMS_PER_INCH;
		else
			value = g_strtod (ed->y_resolution, NULL);

		tracker_resource_set_double (resource, "nfo:verticalResolution", value);
	}

	tracker_extract_info_set_resource (info, resource);
	retval = TRUE;

out:
	g_clear_object (&metadata);
	g_clear_object (&resource);
	g_clear_pointer (&ed, (GDestroyNotify) raw_exif_data_free);
	g_free (filename);
	g_free (nfo_orientation);
	g_free (uri);
	return retval;
}
