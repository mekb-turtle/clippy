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
	long chunk_size;
	Atom selection, targets, incr, utf8;
};

struct x11_context_incr_context {
	XSelectionRequestEvent request;
	size_t pos;
	struct x11_context_incr_context *next;
};

int copy_x11(char *display_name, bool primary, struct copy_data data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
	int res = 0;
	struct x11_context c = {0};

	c.display = XOpenDisplay(display_name);
	if (!c.display) ERR(2, "Failed to open X11 display");

	c.window = XCreateSimpleWindow(c.display, DefaultRootWindow(c.display), 0, 0, 1, 1, 0, 0, 0);
	if (!c.window) ERR(3, "Failed to create X11 window");

	c.selection = XInternAtom(c.display, primary ? "PRIMARY" : "CLIPBOARD", false);
	if (!c.selection) ERR(4, "Failed to get X11 selection");

	c.targets = XInternAtom(c.display, "TARGETS", false);
	if (!c.targets) ERR(5, "Failed to get TARGETS atom");
	c.incr = XInternAtom(c.display, "INCR", false);
	if (!c.incr) ERR(6, "Failed to get INCR atom");
	c.utf8 = XInternAtom(c.display, "UTF8_STRING", false);
	if (!c.utf8) ERR(7, "Failed to get UTF8_STRING atom");

	XSetSelectionOwner(c.display, c.selection, c.window, CurrentTime);
	if (XGetSelectionOwner(c.display, c.selection) != c.window) ERR(8, "Failed to take ownership of selection");

	// must have enough to store the maximum possible number of atoms
	c.mime_atoms = calloc(4, sizeof(Atom));
	c.mime_atoms_len = 0;
	if (!c.mime_atoms) ERR(errno, "Failed to allocate memory");

	c.mime_atoms[c.mime_atoms_len++] = c.targets;
	if (data.type == COPY_TYPE_TEXT_UTF8) c.mime_atoms[c.mime_atoms_len++] = c.utf8;

	const char *mime = copy_get_mime(data.type);
	if (mime) {
		Atom a = XInternAtom(c.display, mime, false);
		if (!a) ERR(9, "Failed to get atom");
		c.mime_atoms[c.mime_atoms_len++] = a;
	}

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

	size_t i = 0;

	struct x11_context_incr_context *incr_head = NULL, *incr_tail = NULL;

	while (1) {
		XEvent event;
		XNextEvent(c.display, &event);

		if (event.type == SelectionRequest) {
			XSelectionRequestEvent *request = &event.xselectionrequest;
			Window requestor = request->requestor;
			if (!requestor) continue;
			if (request->selection != c.selection) continue;
			if (request->display != c.display) continue;

			bool exists = false;
			for (struct x11_context_incr_context *i = incr_head; i; i = i->next) {
				if (i->request.requestor == requestor) {
					exists = true;
					break;
				}
			}
			// prevent creating duplicate context
			if (exists) break;

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
				for (i = 0; i < c.mime_atoms_len; ++i) {
					// find matching target
					if (c.mime_atoms[i] == request->target) {
						if (data.size > chunk_size) {
							// send using INCR
							XChangeProperty(c.display, requestor, property, c.incr, 32, PropModeReplace, 0, 0);
							XSelectInput(c.display, requestor, PropertyChangeMask);

							struct x11_context_incr_context *ctx = calloc(1, sizeof(struct x11_context_incr_context));
							if (!ctx) ERR(10, "Failed to allocate memory for INCR context");
							if (incr_tail) incr_tail->next = ctx;
							incr_tail = ctx;
							if (!incr_head) incr_head = ctx;

							ctx->request = *request;
						} else {
							XChangeProperty(c.display, requestor, property,
							                request->target, 8, PropModeReplace, data.data, data.size);
						}
						break;
					}
				}
				// no matching targets found
			}
			XSendEvent(c.display, requestor, False, 0, (XEvent *) &reply);
		} else if (event.type == SelectionClear) {
			if (event.xselectionclear.selection != c.selection) continue;
			if (event.xselectionclear.display != c.display) continue;
			// Quit once another program copies content
			if (XGetSelectionOwner(c.display, c.selection) != c.window) break;
		} else if (event.type == PropertyNotify) {
			XPropertyEvent *notify = &event.xproperty;
			if (notify->state != PropertyDelete) continue;
			Window requestor = notify->window;
			if (!requestor) continue;
			if (notify->display != c.display) continue;
			// linked list of context
			struct x11_context_incr_context *ctx = NULL, *prev = NULL;
			for (struct x11_context_incr_context *i = incr_head; i; i = i->next) {
				if (i->request.requestor == requestor) {
					ctx = i;
					break;
				}
				prev = i;
			}
			if (!ctx) continue; // no context found
			size_t write_size = chunk_size;
			if (ctx->pos > data.size - chunk_size)
				write_size = data.size - ctx->pos;
			if (ctx->pos < data.size) {
				// write chunk
				XChangeProperty(c.display, requestor, ctx->request.property, ctx->request.target, 8, PropModeReplace, data.data + ctx->pos, write_size);
				ctx->pos += write_size;
			} else {
				// transfer finished
				XChangeProperty(c.display, requestor, ctx->request.property, ctx->request.target, 8, PropModeReplace, 0, 0);
				// remove from linked list
				if (prev) prev->next = ctx->next;
				if (incr_head == ctx) incr_head = ctx->next;
				if (incr_tail == ctx) incr_tail = prev;
				free(ctx);
			}
			XFlush(c.display);
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