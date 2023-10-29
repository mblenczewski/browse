#ifndef BROWSE_H
#define BROWSE_H

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif /* _XOPEN_SOURCE */

#define _XOPEN_SOURCE 700

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define ARRLEN(arr) (sizeof (arr) / sizeof (arr)[0])

#define BROWSE_WINDOW_TITLE_MAX 256
#define BROWSE_WINDOW_URL_MAX 1024

struct browse_window;
struct browse_window {
	GtkWindow *window;
	GtkEventController *event_handler;

	GdkClipboard *clipboard;

	WebKitWebView *webview;

	char title[BROWSE_WINDOW_TITLE_MAX];
	char url[BROWSE_WINDOW_URL_MAX];

	struct browse_window *next, *prev;
};

struct browse_ctx {
	GtkSettings *gtk_settings;
	WebKitSettings *webkit_settings;

	struct browse_window *root;

	bool shutdown;
};

struct browse_gtk_setting {
	char const *name;
	union { gboolean b; guint u; gchar const *s; } v;
};

struct browse_webkit_setting {
	char const *name;
	union { gboolean b; guint u; gchar const *s; } v;
};

union browse_keybind_arg {
	int i;
	char const *s;
};

typedef void (*browse_keybind_fn)(struct browse_window *ctx, union browse_keybind_arg const *arg);

struct browse_keybind {
	GdkModifierType mod;
	guint key;
	browse_keybind_fn handler;
	union browse_keybind_arg arg;
};

extern void
stopload(struct browse_window *ctx, union browse_keybind_arg const *arg);

extern void
reload(struct browse_window *ctx, union browse_keybind_arg const *arg);

extern void
navigate(struct browse_window *ctx, union browse_keybind_arg const *arg);

extern void
clipboard(struct browse_window *ctx, union browse_keybind_arg const *arg);

extern void
javascript(struct browse_window *ctx, union browse_keybind_arg const *arg);

extern void
search(struct browse_window *ctx, union browse_keybind_arg const *arg);

#endif /* BROWSE_H */
