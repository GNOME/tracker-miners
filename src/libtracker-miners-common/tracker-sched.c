/*
 * Copyright (C) 2011, Nokia <ivan.frade@nokia.com>
 * Copyright (C) 2020, Sam Thursfield <sam@afuera.me.uk>
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

#ifdef __linux__

#include <pthread.h>

#include "tracker-sched.h"

#include "tracker-debug.h"

/* Sets the priority of the current thread to SCHED_IDLE.
 *
 * Threads spawned from a SCHED_IDLE thread will inherit the same priority,
 * so you just need to call this function when the main thread starts to
 * set priority for the whole process.
 */
gboolean
tracker_sched_idle (void)
{
	struct sched_param sp;
	int result, policy;

	result = pthread_getschedparam (pthread_self (), &policy, &sp);

	if (result == 0) {
		result = pthread_setschedparam (pthread_self(), SCHED_IDLE, &sp);
	}

	if (result == 0) {
		TRACKER_NOTE (CONFIG, g_message ("Set scheduler policy to SCHED_IDLE"));

		return TRUE;
	} else {
		const gchar *str = g_strerror (result);

		g_message ("Error setting scheduler policy: %s", str ? str : "no error given");

		return FALSE;
	}
}

#else /* __linux__ */

/* Although pthread_setschedparam() should exist on any POSIX compliant OS,
 * the SCHED_IDLE policy is Linux-specific. The POSIX standard only requires
 * the existence of realtime and 'other' policies.
 *
 * We could set the priority to 0. On FreeBSD the default priority is already
 * 0, and this may be true on other platforms, so we currently don't bother.
 * See https://gitlab.gnome.org/GNOME/tracker-miners/merge_requests/140 for
 * more discussion.
 */
#include <glib.h>

#include "tracker-sched.h"

gboolean
tracker_sched_idle (void)
{
	return TRUE;
}

#endif /* __linux__ */
