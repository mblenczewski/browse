#include "browse.h"

#include "config.h"

struct browse_ctx browse;

int
main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	memset(&browse, 0, sizeof browse);

	gtk_init();

	browse.gtk_settings = gtk_settings_get_default();
	for (size_t i = 0; i < ARRLEN(gtk_settings); i++) {
		g_object_set(G_OBJECT(browse.gtk_settings), gtk_settings[i].name, gtk_settings[i].v, NULL);
	}

	browse.webkit_settings = webkit_settings_new();
	for (size_t i = 0; i < ARRLEN(webkit_settings); i++) {
		g_object_set(G_OBJECT(browse.webkit_settings), webkit_settings[i].name, webkit_settings[i].v, NULL);
	}

	if (!browse_new(start_page, NULL)) {
		fprintf(stderr, "Failed to allocate root window\n");
		exit(EXIT_FAILURE);
	}

	while (!browse.shutdown) g_main_context_iteration(NULL, TRUE);

	exit(EXIT_SUCCESS);
}

#include "webview.c"
#include "api.c"
