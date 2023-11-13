#ifndef BROWSE_H
#define BROWSE_H

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif /* _XOPEN_SOURCE */

#define _XOPEN_SOURCE 700

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdatomic.h>
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

struct browse_ctx {
	GtkSettings *gtk_settings;
	WebKitSettings *webkit_settings;

	atomic_uint clients;
	atomic_bool shutdown;
};

extern struct browse_ctx browse;

struct browse_client;

extern struct browse_client *
browse_new(char const *uri, struct browse_client *root);

extern void
browse_update_title(struct browse_client *self, char const *uri);

extern void
browse_load_uri(struct browse_client *self, char const *uri);

extern void
browse_download_uri(struct browse_client *self, char const *uri);

enum browse_prop_type {
	BROWSE_PROP_STRICT_TLS,
	_BROWSE_PROP_TYPE_COUNT,
};

union browse_prop {
	struct {
		bool enabled;
	} strict_tls;
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

typedef void (*browse_keybind_fn)(struct browse_client *client, union browse_keybind_arg const *arg);

struct browse_keybind {
	GdkModifierType mod;
	guint key;
	browse_keybind_fn handler;
	union browse_keybind_arg arg;
};

extern void
stopload(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
reload(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
navigate(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
clipboard(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
javascript(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
search(struct browse_client *client, union browse_keybind_arg const *arg);

extern void
toggle(struct browse_client *client, union browse_keybind_arg const *arg);

#endif /* BROWSE_H */
