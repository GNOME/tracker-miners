/*
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@gnome.org>
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
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

#include <upower.h>

#include "tracker-power.h"

#define GET_PRIV(obj) (tracker_power_get_instance_private (TRACKER_POWER (obj)))

typedef struct {
	UpClient  *client;
	UpDevice  *composite_device;
	gboolean   on_battery;
	gboolean   on_low_battery;
} TrackerPowerPrivate;

static void     tracker_power_initable_iface_init (GInitableIface *iface);
static void     tracker_power_finalize            (GObject         *object);
static void     tracker_power_get_property        (GObject         *object,
                                                   guint            param_id,
                                                   GValue                  *value,
                                                   GParamSpec      *pspec);

enum {
	PROP_0,
	PROP_ON_BATTERY,
	PROP_ON_LOW_BATTERY
};

G_DEFINE_TYPE_WITH_CODE (TrackerPower, tracker_power, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (TrackerPower)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, tracker_power_initable_iface_init));

static void
tracker_power_class_init (TrackerPowerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize     = tracker_power_finalize;
	object_class->get_property = tracker_power_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_ON_BATTERY,
	                                 g_param_spec_boolean ("on-battery",
	                                                       "Battery in use",
	                                                       "Whether the battery is being used",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_ON_LOW_BATTERY,
	                                 g_param_spec_boolean ("on-low-battery",
	                                                       "Battery low",
	                                                       "Whether the battery is low",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
on_on_battery_changed (UpClient     *client,
                       GParamSpec   *pspec,
                       TrackerPower *power)
{
	TrackerPowerPrivate *priv = GET_PRIV (power);
	gboolean on_battery;

	on_battery = up_client_get_on_battery (priv->client);
	if (on_battery != priv->on_battery) {
		priv->on_battery = on_battery;
		g_object_notify (G_OBJECT (power), "on-battery");
	}
}

static void
on_warning_level_changed (UpDevice     *device,
                          GParamSpec   *pspec,
                          TrackerPower *power)
{
	TrackerPowerPrivate *priv = GET_PRIV (power);
	UpDeviceLevel warning_level;
	gboolean on_low_battery;

	g_object_get (priv->composite_device, "warning-level", &warning_level, NULL);
	on_low_battery = warning_level >= UP_DEVICE_LEVEL_LOW;
	if (on_low_battery != priv->on_low_battery) {
		priv->on_low_battery = on_low_battery;
		g_object_notify (G_OBJECT (power), "on-low-battery");
	}
}

static void
tracker_power_init (TrackerPower *power)
{
}
static gboolean
tracker_power_initable_init (GInitable     *initable,
                             GCancellable  *cancellable,
                             GError       **error)
{
	TrackerPower *power;
	TrackerPowerPrivate *priv;

	power = TRACKER_POWER (initable);
	priv = GET_PRIV (power);

	/* connect to a UPower instance */
	priv->client = up_client_new ();

	if (priv->client == NULL) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR, "Unable to connect to UPower");
		return FALSE;
	}

	g_signal_connect (priv->client, "notify::on-battery",
	                  G_CALLBACK (on_on_battery_changed), power);
	on_on_battery_changed (priv->client, NULL, power);
	priv->composite_device = up_client_get_display_device (priv->client);
	g_signal_connect (priv->composite_device, "notify::warning-level",
		              G_CALLBACK (on_warning_level_changed), power);
	on_warning_level_changed (priv->composite_device, NULL, power);

	return TRUE;
}

static void
tracker_power_finalize (GObject *object)
{
	TrackerPowerPrivate *priv;

	priv = GET_PRIV (object);

	g_clear_object (&priv->composite_device);
	g_clear_object (&priv->client);

	(G_OBJECT_CLASS (tracker_power_parent_class)->finalize) (object);
}

static void
tracker_power_get_property (GObject    *object,
                            guint       param_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	TrackerPowerPrivate *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ON_BATTERY:
		g_value_set_boolean (value, priv->on_battery);
		break;
	case PROP_ON_LOW_BATTERY:
		g_value_set_boolean (value, priv->on_low_battery);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
tracker_power_initable_iface_init (GInitableIface *iface)
{
	iface->init = tracker_power_initable_init;
}

/**
 * tracker_power_new:
 *
 * Creates a new instance of #TrackerPower.
 *
 * Returns: The newly created #TrackerPower, or %NULL if there was an error.
 **/
TrackerPower *
tracker_power_new ()
{
	GError *error = NULL;
	TrackerPower *object;

	object = g_initable_new (TRACKER_TYPE_POWER, NULL, &error, NULL);

	if (error) {
		g_warning ("%s", error->message);
	}

	return object;
}

/**
 * tracker_power_get_on_battery:
 * @power: A #TrackerPower.
 *
 * Returns whether the computer battery (if any) is currently in use.
 *
 * Returns: #TRUE if the computer is running on battery power.
 **/
gboolean
tracker_power_get_on_battery (TrackerPower *power)
{
	TrackerPowerPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_POWER (power), TRUE);

	priv = GET_PRIV (power);

	return priv->on_battery;
}

/**
 * tracker_power_get_on_low_battery:
 * @power: A #TrackerPower
 *
 * Returns whether the computer has batteries.
 *
 * Returns: #TRUE if the computer has batteries available.
 **/
gboolean
tracker_power_get_on_low_battery (TrackerPower *power)
{
	TrackerPowerPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_POWER (power), TRUE);

	priv = GET_PRIV (power);

	return priv->on_low_battery;
}
