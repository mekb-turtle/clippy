#include "copy-wayland.h"
#include "util.h"

#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

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

#include "wlr-data-control-unstable-v1-client-protocol.h"

struct wayland_context {
	struct wl_display *display;
	struct wl_registry *registry;
	struct zwlr_data_control_device_v1 *device;
	struct zwlr_data_control_source_v1 *source;
	struct wl_seat *seat;
	uint32_t seat_name;
	struct zwlr_data_control_manager_v1 *dcm;
	uint32_t dcm_name;
};

static void registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	struct wayland_context *c = data;
	(void) version;
	// Bind seat and data control manager
	if (strcmp(interface, "wl_seat") == 0) {
		if (c->seat) wl_seat_destroy(c->seat);
		c->seat_name = name;
		c->seat = wl_registry_bind(registry, name, &wl_seat_interface, 2);
	} else if (strcmp(interface, "zwlr_data_control_manager_v1") == 0) {
		if (c->dcm) zwlr_data_control_manager_v1_destroy(c->dcm);
		c->dcm_name = name;
		c->dcm = wl_registry_bind(registry, name, &zwlr_data_control_manager_v1_interface, 2);
	}
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
	struct wayland_context *c = data;
	(void) registry;
	(void) name;
	if (name == c->seat_name && c->seat) wl_seat_destroy(c->seat);
	if (name == c->dcm_name && c->dcm) zwlr_data_control_manager_v1_destroy(c->dcm);
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

int copy_wayland(char *display_name, bool primary, struct copy_data *data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
	int res = 0;
	struct wayland_context c = {0};

	c.display = wl_display_connect(display_name);
	if (!c.display) ERR(2, "Failed to open Wayland display");

	c.registry = wl_display_get_registry(c.display);
	if (!c.registry) ERR(3, "Failed to open Wayland registry");

	static const struct wl_registry_listener registry_listener = {
	        .global = registry_global,
	        .global_remove = registry_global_remove,
	};
	wl_registry_add_listener(c.registry, &registry_listener, &c);

	// Wait until requests are processed
	wl_display_roundtrip(c.display);

	if (!c.seat) ERR(4, "Failed to bind to seat interface");
	if (!c.dcm) ERR(5, "Failed to bind to data control manager interface");

	c.device = zwlr_data_control_manager_v1_get_data_device(c.dcm, c.seat);
	if (!c.device) ERR(6, "Failed to get data device");

	c.source = zwlr_data_control_manager_v1_create_data_source(c.dcm);
	if (!c.source) ERR(7, "Failed to create data source");

	// Offer all MIME types
	for (struct copy_data *d = data; d->mime && d->data; ++d)
		zwlr_data_control_source_v1_offer(c.source, d->mime);

	static const struct zwlr_data_control_source_v1_listener data_source_listener = {
	        .send = data_source_send,
	        .cancelled = data_source_cancelled,
	};
	zwlr_data_control_source_v1_add_listener(c.source, &data_source_listener, data);

	// Primary or clipboard selection
	if (primary)
		zwlr_data_control_device_v1_set_primary_selection(c.device, c.source);
	else
		zwlr_data_control_device_v1_set_selection(c.device, c.source);

	if (pre_loop_callback) {
		res = pre_loop_callback(ctx);
		if (res) goto cleanup;
	}

	while (wl_display_dispatch(c.display) >= 0) {
		if (!running) goto cleanup;
	}

	ERR(8, "Failed to dispatch display");

cleanup:
	if (c.device) zwlr_data_control_device_v1_destroy(c.device);
	if (c.source) zwlr_data_control_source_v1_destroy(c.source);
	if (c.seat) wl_seat_destroy(c.seat);
	if (c.dcm) zwlr_data_control_manager_v1_destroy(c.dcm);
	if (c.registry) wl_registry_destroy(c.registry);
	if (c.display) wl_display_disconnect(c.display);
	return res;
}