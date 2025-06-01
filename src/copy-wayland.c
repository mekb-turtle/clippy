#include "util.h"
#include "copy-wayland.h"

#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-client.h>

/*
Most of this code is from https://github.com/noocsharp/wayclip

Copyright 2022 Nihal Jere <nihal@nihaljere.xyz>

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#include "protocol/wlr-data-control-unstable-v1-client.h"

static struct wl_seat *seat = NULL;
static uint32_t seat_name;

static struct zwlr_data_control_manager_v1 *data_control_manager = NULL;
static uint32_t data_control_manager_name;

static void registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	(void) data;
	(void) version;
	// Bind seat and data control manager
	if (strcmp(interface, "wl_seat") == 0) {
		seat_name = name;
		seat = wl_registry_bind(registry, name, &wl_seat_interface, 2);
	} else if (strcmp(interface, "zwlr_data_control_manager_v1") == 0) {
		data_control_manager_name = name;
		data_control_manager = wl_registry_bind(registry, name, &zwlr_data_control_manager_v1_interface, 2);
	}
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
	(void) data;
	(void) registry;
	(void) name;
	// Destroy seat and data control manager
	if (name == seat_name && seat) {
		wl_seat_destroy(seat);
		seat = NULL;
	}
	if (name == data_control_manager_name && data_control_manager) {
		zwlr_data_control_manager_v1_destroy(data_control_manager);
		data_control_manager = NULL;
	}
}

static bool running = true;

void data_source_send(void *d, struct zwlr_data_control_source_v1 *source, const char *requested_mime, int32_t fd) {
	(void) source;
	struct copy_data *data = (struct copy_data *) d;
	for (struct copy_data *d = data; d->mime && d->data; ++d) {
		// Find copy data with correct MIME type
		if (strcmp(d->mime, requested_mime) == 0) {
			write(fd, d->data, d->size);
			break;
		}
	}
	close(fd);
}

void data_source_cancelled(void *d, struct zwlr_data_control_source_v1 *source) {
	(void) d;
	(void) source;
	// Quit listening once another program copies content
	running = false;
}

int copy_wayland(char *display_name, bool do_fork, bool primary, struct copy_data *data, char **msg) {
	struct wl_display *display = wl_display_connect(display_name);
	if (!display) ERR(2, "Failed to open Wayland display");

	struct wl_registry *registry = wl_display_get_registry(display);
	if (!registry) ERR(3, "Failed to open Wayland registry");

	static const struct wl_registry_listener registry_listener = {
	        .global = registry_global,
	        .global_remove = registry_global_remove,
	};
	wl_registry_add_listener(registry, &registry_listener, NULL);

	// Wait until requests are processed
	wl_display_roundtrip(display);

	if (!seat) ERR(4, "Failed to bind to seat interface");
	if (!data_control_manager) ERR(5, "Failed to bind to seat interface");

	struct zwlr_data_control_device_v1 *device = zwlr_data_control_manager_v1_get_data_device(data_control_manager, seat);
	if (!device) ERR(6, "Failed to get data device");

	struct zwlr_data_control_source_v1 *source = zwlr_data_control_manager_v1_create_data_source(data_control_manager);
	if (!source) ERR(6, "Failed to create data source");

	// Offer all MIME types
	for (struct copy_data *d = data; d->mime && d->data; ++d)
		zwlr_data_control_source_v1_offer(source, d->mime);

	static const struct zwlr_data_control_source_v1_listener data_source_listener = {
	        .send = data_source_send,
	        .cancelled = data_source_cancelled,
	};
	zwlr_data_control_source_v1_add_listener(source, &data_source_listener, data);

	// Primary or clipboard selection
	if (primary)
		zwlr_data_control_device_v1_set_primary_selection(device, source);
	else
		zwlr_data_control_device_v1_set_selection(device, source);

	if (do_fork) FORK();

	while (wl_display_dispatch(display) >= 0) {
		if (!running) return 0;
	}

	ERR(5, "Failed to dispatch display");
}
