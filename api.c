#include "browse.h"

static char *
shellcmd(char const *cmd)
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
stopload(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);

	(void) arg;

	webkit_web_view_stop_loading(client->webview);
}

void
reload(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	if (arg->i) {
		webkit_web_view_reload_bypass_cache(client->webview);
	} else {
		webkit_web_view_reload(client->webview);
	}
}

void
navigate(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	if (arg->i < 0) {
		webkit_web_view_go_back(client->webview);
	} else if (arg->i > 0) {
		webkit_web_view_go_forward(client->webview);
	}
}

static void
clipboard_cb(GObject *src, GAsyncResult *res, void *user_data)
{
	(void) src;

	struct browse_client *client = user_data;

	char *text;
	if ((text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(src), res, NULL))) {
		browse_load_uri(client, text);
	}
}

void
clipboard(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	if (arg->i) {
		gdk_clipboard_read_text_async(client->clipboard, NULL, clipboard_cb, client);
	} else {
		gdk_clipboard_set_text(client->clipboard, webkit_web_view_get_uri(client->webview));
	}
}

void
javascript(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	webkit_web_view_evaluate_javascript(client->webview, arg->s, -1,
					    NULL, /* world */
					    NULL, /* source_uri */
					    NULL, /* cancellable */
					    NULL, /* callback */
					    NULL  /* user data */);
}

void
search(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	char *uri = shellcmd(arg->s);
	if (!uri) return;

	browse_load_uri(client, uri);

	free(uri);
}

void
toggle(struct browse_client *client, union browse_keybind_arg const *arg)
{
	assert(client);
	assert(arg);

	union browse_prop *prop = &client->props[arg->i];

	switch (arg->i) {
	case BROWSE_PROP_STRICT_TLS:
		prop->strict_tls.enabled = !prop->strict_tls.enabled;
		webkit_network_session_set_tls_errors_policy(client->webnetsession,
				prop->strict_tls.enabled ? WEBKIT_TLS_ERRORS_POLICY_FAIL
							 : WEBKIT_TLS_ERRORS_POLICY_IGNORE);
		break;
	}

	webkit_web_view_reload(client->webview);
}
