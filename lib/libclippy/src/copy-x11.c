#include "../include/copy-x11.h"
#include "util.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

struct x11_context {
	Display *display;
	Window window;
	Atom *mime_atoms;
	size_t mime_atoms_len;
	size_t mime_atoms_actual_len;
	long chunk_size;
	Atom selection, targets, incr, text, utf8;
};

int copy_x11(char *display_name, bool primary, struct copy_data *data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
	int res = 0;
	struct x11_context c = {0};

	c.display = XOpenDisplay(display_name);
	if (!c.display) ERR(2, "Failed to open X11 display");

	c.window = XCreateSimpleWindow(c.display, DefaultRootWindow(c.display), 0, 0, 1, 1, 0, 0, 0);
	if (!c.window) ERR(3, "Failed to create X11 window");

	c.selection = XInternAtom(c.display, primary ? "PRIMARY" : "CLIPBOARD", false);
	if (!c.selection) ERR(4, "Failed to get X11 selection");

	c.targets = XInternAtom(c.display, "TARGETS", false);
	if (!c.targets) ERR(4, "Failed to get TARGETS atom");
	c.incr = XInternAtom(c.display, "INCR", false);
	if (!c.incr) ERR(4, "Failed to get INCR atom");
	c.text = XInternAtom(c.display, "TEXT", false);
	if (!c.text) ERR(4, "Failed to get TEXT atom");
	c.utf8 = XInternAtom(c.display, "UTF8_STRING", false);
	if (!c.utf8) ERR(4, "Failed to get UTF8_STRING atom");

	XSetSelectionOwner(c.display, c.selection, c.window, CurrentTime);
	if (XGetSelectionOwner(c.display, c.selection) != c.window) ERR(1, "Failed to take ownership of selection");

	c.mime_atoms_actual_len = 0;
	for (struct copy_data *d = data; d->mime && d->data; ++d) {
		++c.mime_atoms_actual_len;
	}
	// update this to however many encoding atoms are added + 1
	c.mime_atoms = calloc(c.mime_atoms_actual_len + 4, sizeof(Atom));
	if (!c.mime_atoms) ERR(errno, "Failed to allocate memory");

	if (pre_loop_callback) {
		res = pre_loop_callback(ctx);
		if (res) goto cleanup;
	}

	// get appropriate incremental chunk size
	long chunk_size = XExtendedMaxRequestSize(c.display);
	if (!chunk_size) chunk_size = XMaxRequestSize(c.display);
	if (chunk_size) chunk_size /= 4;
	else
		chunk_size = 4096; // fallback
	chunk_size = 1;        // debug, TODO: remove

	// store index for ISO-8859-1/UTF-8 content
	bool has_utf8 = false;
	size_t utf8_index;
	bool has_xa = false;
	size_t xa_index;

	// detect if UTF-8 content exists, or ISO-8859-1 content exists
	for (size_t i = 0; i < c.mime_atoms_actual_len; ++i) {
		if (data[i].encoding) {
			if (!has_utf8 && (strcasecmp(data[i].encoding, "UTF8") == 0 ||
			                  strcasecmp(data[i].encoding, "UTF-8") == 0)) {
				has_utf8 = true;
				utf8_index = i;
			}
			if (!has_xa && strcasecmp(data[i].encoding, "ISO-8859-1") == 0) {
				has_xa = true;
				xa_index = i;
			}
		}
	}

	size_t i = 0;
	// add atoms for encoding targets
	c.mime_atoms[i++] = c.targets;
	if (has_utf8) c.mime_atoms[i++] = c.utf8;
	if (has_xa) {
		c.mime_atoms[i++] = XA_STRING;
		c.mime_atoms[i++] = c.text;
	}
	size_t mime_start = i;
	for (size_t j = 0; j < c.mime_atoms_actual_len; ++j) {
		c.mime_atoms[i++] = XInternAtom(c.display, data[j].mime, false);
	}
	c.mime_atoms_len = i;

	while (1) {
		XEvent event;
		XNextEvent(c.display, &event);

		if (event.type == SelectionRequest) {
			if (event.xselectionrequest.selection != c.selection) continue;
			if (event.xselectionrequest.display != c.display) continue;
			XSelectionRequestEvent *request = &event.xselectionrequest;
			Window requestor = request->requestor;
			Atom property = request->property;
			XSelectionEvent reply = {
			        .type = SelectionNotify,
			        .selection = c.selection,
			        .display = c.display,
			        .time = request->time,
			        .property = property,
			        .target = request->target,
			        .requestor = requestor,
			};

			if (request->target == c.targets) {
				// send available targets
				XChangeProperty(c.display, requestor, property, XA_ATOM, 32,
				                PropModeReplace, (unsigned char *) c.mime_atoms, c.mime_atoms_len);
			} else {
				if (has_xa) {
					if (request->target == XA_STRING || request->target == c.text) {
						// send the contents of the string type anyway
						i = xa_index;
						goto match;
					}
				}
				if (has_utf8) {
					if (request->target == c.utf8) {
						i = utf8_index;
						goto match;
					}
				}
				for (i = 0; i < c.mime_atoms_actual_len; ++i) {
					// find matching target
					if (c.mime_atoms[i + mime_start] == request->target) {
					match:
						XChangeProperty(c.display, requestor, property,
						                request->target, 8, PropModeReplace, data[i].data, data[i].size);
						goto send_event;
					}
				}
				// no matching targets found
				property = None;
			}
		send_event:
			XSendEvent(c.display, requestor, False, 0, (XEvent *) &reply);
		} else if (event.type == SelectionClear) {
			if (event.xselectionclear.selection != c.selection) continue;
			if (event.xselectionclear.display != c.display) continue;
			// Quit once another program copies content
			if (XGetSelectionOwner(c.display, c.selection) != c.window) break;
		}
	}

cleanup:
	if (c.mime_atoms) free(c.mime_atoms);
	if (c.display) {
		if (c.window) XDestroyWindow(c.display, c.window);
		XCloseDisplay(c.display);
	}
	return res;
}
