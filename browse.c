#include "browse.h"

#include "config.h"

static struct browse_window *
browse_new_window(struct browse_ctx *ctx, char const *uri, struct browse_window *root);

static void
browse_del_window(struct browse_window *window);

static void
browse_update_title(struct browse_window *ctx, char const *uri);

static void
browse_load_uri(struct browse_window *ctx, char const *uri);

static void
browse_on_window_destroy(GtkWindow *window, struct browse_window *ctx);

static gboolean
browse_on_key_pressed(GtkEventController *controller, guint keyval, guint keycode,
		      GdkModifierType state, struct browse_window *ctx);

static void
browse_on_load_changed(WebKitWebView *webview, WebKitLoadEvent ev, struct browse_window *ctx);

static struct browse_ctx ctx;

int
main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	memset(&ctx, 0, sizeof ctx);

	gtk_init();

	ctx.gtk_settings = gtk_settings_get_default();
	for (size_t i = 0; i < ARRLEN(gtk_settings); i++) {
		g_object_set(G_OBJECT(ctx.gtk_settings), gtk_settings[i].name, gtk_settings[i].v, NULL);
	}

	ctx.webkit_settings = webkit_settings_new();
	for (size_t i = 0; i < ARRLEN(webkit_settings); i++) {
		g_object_set(G_OBJECT(ctx.webkit_settings), webkit_settings[i].name, webkit_settings[i].v, NULL);
	}

	ctx.root = browse_new_window(&ctx, start_page, NULL);

	while (!ctx.shutdown)
		g_main_context_iteration(NULL, TRUE);

	exit(EXIT_SUCCESS);
}

static struct browse_window *
browse_new_window(struct browse_ctx *ctx, char const *uri, struct browse_window *root)
{
	assert(ctx);
	assert(uri);

	(void) root;

	struct browse_window *window = malloc(sizeof *window);
	if (!window) return NULL;

	window->window = GTK_WINDOW(gtk_window_new());
	window->event_handler = gtk_event_controller_key_new();

	gtk_widget_add_controller(GTK_WIDGET(window->window), window->event_handler);

	g_signal_connect(window->event_handler, "key-pressed", G_CALLBACK(browse_on_key_pressed), window);
	g_signal_connect(window->window, "destroy", G_CALLBACK(browse_on_window_destroy), window);

	window->clipboard = gtk_widget_get_primary_clipboard(GTK_WIDGET(window->window));

	window->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

	g_signal_connect(window->webview, "load-changed", G_CALLBACK(browse_on_load_changed), window);

	webkit_web_view_set_settings(window->webview, ctx->webkit_settings);

	gtk_window_set_child(window->window, GTK_WIDGET(window->webview));

	browse_load_uri(window, uri);
	browse_update_title(window, NULL);

	window->next = window->prev = NULL;

	gtk_window_present(window->window);

	return window;
}

static void
browse_del_window(struct browse_window *window)
{
	if (window->prev) window->prev->next = window->next;
	if (window->next) window->next->prev = window->prev;

	if (ctx.root == window) ctx.shutdown = true;

	free(window);
}

static void
browse_update_title(struct browse_window *ctx, char const *uri)
{
	if (uri) snprintf(ctx->title, sizeof ctx->title, "%s", uri);

	gtk_window_set_title(ctx->window, ctx->title);
}

static void
browse_load_uri(struct browse_window *ctx, char const *uri)
{
	assert(uri);

	if (g_str_has_prefix(uri, "http://") || g_str_has_prefix(uri, "https://") ||
	    g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:")) {
		webkit_web_view_load_uri(ctx->webview, uri);
	} else {
		snprintf(ctx->url, sizeof ctx->url, search_page, uri);
		webkit_web_view_load_uri(ctx->webview, ctx->url);
	}
}

static gboolean
browse_on_key_pressed(GtkEventController *controller, guint keyval, guint keycode,
		      GdkModifierType state, struct browse_window *ctx)
{
	(void) controller;
	(void) keycode;

	for (size_t i = 0; i < ARRLEN(keybinds); i++) {
		if ((state & GDK_MODIFIER_MASK) == keybinds[i].mod && keyval == keybinds[i].key) {
			keybinds[i].handler(ctx, &keybinds[i].arg);
			return TRUE;
		}
	}

	return FALSE;
}

static void
browse_on_window_destroy(GtkWindow *window, struct browse_window *ctx)
{
	(void) window;

	browse_del_window(ctx);
}

static void
browse_on_load_changed(WebKitWebView *webview, WebKitLoadEvent ev, struct browse_window *ctx)
{
	(void) ev;

	char const *uri = webkit_web_view_get_uri(webview);
	browse_update_title(ctx, uri);
}

void
stopload(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	
	(void) arg;

	webkit_web_view_stop_loading(ctx->webview);
}

void
reload(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	assert(arg);

	if (arg->i) {
		webkit_web_view_reload_bypass_cache(ctx->webview);
	} else {
		webkit_web_view_reload(ctx->webview);
	}
}

void
navigate(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	assert(arg);

	if (arg->i < 0) {
		webkit_web_view_go_back(ctx->webview);
	} else if (arg->i > 0) {
		webkit_web_view_go_forward(ctx->webview);
	}
}

static void
clipboard_cb(GObject *src, GAsyncResult *res, void *user_data)
{
	(void) src;

	struct browse_window *ctx = user_data;

	char *text;
	if ((text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(src), res, NULL))) {
		browse_load_uri(ctx, text);
	}
}

void
clipboard(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	assert(arg);

	if (arg->i) {
		gdk_clipboard_read_text_async(ctx->clipboard, NULL, clipboard_cb, ctx);
	} else {
		gdk_clipboard_set_text(ctx->clipboard, webkit_web_view_get_uri(ctx->webview));
	}
}

static void
javascript_cb(GObject *src, GAsyncResult *res, void *user_data)
{
	(void) user_data;

	JSCValue *jsres;
	if ((jsres = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(src), res, NULL))) {
		// TODO: how to correctly free jsres, which we apparently own now?
	}
}

void
javascript(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	assert(arg);

	webkit_web_view_evaluate_javascript(ctx->webview, arg->s, -1,
					    NULL, /* world */
					    NULL, /* source_uri */
					    NULL, javascript_cb, NULL);
}

static char *
spawn(char const *cmd)
{
	FILE *pstdout = popen(cmd, "r");
	if (!pstdout) return NULL;

	char *line = NULL;
	size_t len;

	if (getline(&line, &len, pstdout) == -1) {
		pclose(pstdout);
		return NULL;
	}

	pclose(pstdout);

	(void) len;

	return line;
}

void
search(struct browse_window *ctx, union browse_keybind_arg const *arg)
{
	assert(ctx);
	assert(arg);

	char *uri = spawn(arg->s);
	if (!uri) return;

	browse_load_uri(ctx, uri);

	free(uri);
}
