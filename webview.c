#include "browse.h"

struct browse_client {
	GtkWindow *window;
	GtkEventController *event_handler;

	GdkClipboard *clipboard;

	WebKitWebView *webview;
	WebKitNetworkSession *webnetsession;

	char title[BROWSE_WINDOW_TITLE_MAX];
	char url[BROWSE_WINDOW_URL_MAX];

	union browse_prop props[_BROWSE_PROP_TYPE_COUNT];
};

static gboolean
event_controller_on_key_pressed(GtkEventController *controller, guint keyval, guint keycode,
				GdkModifierType state, struct browse_client *client);

static void
window_on_destroy(GtkWindow *window, struct browse_client *client);

static void
webview_on_load_changed(WebKitWebView *webview, WebKitLoadEvent ev, struct browse_client *client);

static gboolean
webview_on_decide_policy(WebKitWebView *webview, WebKitPolicyDecision *decision,
			 WebKitPolicyDecisionType type, struct browse_client *client);

struct browse_client *
browse_new(char const *uri, struct browse_client *root)
{
	assert(uri);

	struct browse_client *self = malloc(sizeof *self);
	if (!self) return NULL;

	self->window = GTK_WINDOW(gtk_window_new());
	self->event_handler = gtk_event_controller_key_new();

	gtk_widget_add_controller(GTK_WIDGET(self->window), self->event_handler);

	g_signal_connect(self->event_handler, "key-pressed", G_CALLBACK(event_controller_on_key_pressed), self);
	g_signal_connect(self->window, "destroy", G_CALLBACK(window_on_destroy), self);

	self->clipboard = gtk_widget_get_primary_clipboard(GTK_WIDGET(self->window));

	if (root) {
		self->webview = g_object_new(WEBKIT_TYPE_WEB_VIEW, "related-view", root->webview, NULL);
	} else {
		self->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
	}

	self->webnetsession = webkit_web_view_get_network_session(self->webview);

	g_signal_connect(self->webview, "load-changed", G_CALLBACK(webview_on_load_changed), self);
	g_signal_connect(self->webview, "decide-policy", G_CALLBACK(webview_on_decide_policy), self);

	webkit_web_view_set_settings(self->webview, browse.webkit_settings);

	gtk_window_set_child(self->window, GTK_WIDGET(self->webview));

	browse_load_uri(self, uri);
	browse_update_title(self, NULL);

	gtk_window_present(self->window);

	browse.clients++;

	return self;
}

void
browse_update_title(struct browse_client *self, char const *uri)
{
	if (uri) snprintf(self->title, sizeof self->title, "%s", uri);

	gtk_window_set_title(self->window, self->title);
}

void
browse_load_uri(struct browse_client *self, char const *uri)
{
	assert(uri);

	if (!g_str_has_prefix(uri, "http://") && !g_str_has_prefix(uri, "https://") &&
	    !g_str_has_prefix(uri, "file://") && !g_str_has_prefix(uri, "about:")) {
		snprintf(self->url, sizeof self->url, search_page, uri);
		uri = self->url;
	}

	webkit_web_view_load_uri(self->webview, uri);
}

static gboolean
download_on_decide_destination(WebKitDownload *download, gchar const *suggested_filename, struct browse_client *client);

static void
download_on_received_data(WebKitDownload *download, guint64 length, struct browse_client *client);

static void
download_on_failed(WebKitDownload *download, WebKitDownloadError error, struct browse_client *client);

void
browse_download_uri(struct browse_client *self, char const *uri)
{
	WebKitDownload *download = webkit_web_view_download_uri(self->webview, uri);

	g_signal_connect(download, "decide-destination", G_CALLBACK(download_on_decide_destination), self);
	g_signal_connect(download, "received-data", G_CALLBACK(download_on_received_data), self);
	g_signal_connect(download, "failed", G_CALLBACK(download_on_failed), self);

	// TODO: open new window to display download state
}

static void
window_on_destroy(GtkWindow *window, struct browse_client *client)
{
	g_object_unref(window);

	free(client);

	browse.clients--;
	browse.shutdown = browse.clients == 0;
}

static gboolean
event_controller_on_key_pressed(GtkEventController *controller, guint keyval, guint keycode,
				GdkModifierType state, struct browse_client *client)
{
	(void) controller;
	(void) keycode;

	for (size_t i = 0; i < ARRLEN(keybinds); i++) {
		if ((state & GDK_MODIFIER_MASK) == keybinds[i].mod && keyval == keybinds[i].key) {
			keybinds[i].handler(client, &keybinds[i].arg);
			return TRUE;
		}
	}

	return FALSE;
}

static void
webview_on_load_changed(WebKitWebView *webview, WebKitLoadEvent ev, struct browse_client *client)
{
	(void) ev;

	char const *uri = webkit_web_view_get_uri(webview);
	browse_update_title(client, uri);
}

static gboolean
webview_on_decide_policy(WebKitWebView *webview, WebKitPolicyDecision *decision,
			 WebKitPolicyDecisionType type, struct browse_client *client)
{
	(void) webview;

	switch (type) {
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION: {
		WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));

		if (webkit_navigation_action_get_frame_name(action)) {
			webkit_policy_decision_ignore(decision);
		} else {
			webkit_policy_decision_use(decision);
		}
	} break;

	case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION: {
		WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));

		switch (webkit_navigation_action_get_navigation_type(action)) {
		case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:
		case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED:
		case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:
		case WEBKIT_NAVIGATION_TYPE_RELOAD:
		case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED: {
			char const *uri = webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));
			browse_new(uri, client);
		} break;

		default:
			break;
		}

		webkit_policy_decision_ignore(decision);
	} break;

	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE: {
		WebKitResponsePolicyDecision *response_decision = WEBKIT_RESPONSE_POLICY_DECISION(decision);
		WebKitURIResponse *response = webkit_response_policy_decision_get_response(response_decision);

		char const *uri = webkit_uri_response_get_uri(response);

		if (g_str_has_suffix(uri, "/favicon.ico")) {
			webkit_policy_decision_ignore(decision);
			break;
		}

		if (webkit_response_policy_decision_is_mime_type_supported(response_decision)) {
			webkit_policy_decision_use(decision);
		} else {
			webkit_policy_decision_ignore(decision);

			browse_download_uri(client, uri);
		}
	} break;

	default:
		fprintf(stderr, "Unknown policy decision type: %d, ignoring\n", type);
		webkit_policy_decision_ignore(decision);
		break;
	}

	return TRUE;
}

static gboolean
download_on_decide_destination(WebKitDownload *download, gchar const *suggested_filename,
			       struct browse_client *client)
{
	(void) client;

	char fpath[PATH_MAX];
	snprintf(fpath, sizeof fpath, download_fpath_fmt, suggested_filename);
	webkit_download_set_destination(download, fpath);

	return TRUE;
}

static void
download_on_received_data(WebKitDownload *download, guint64 length, struct browse_client *client)
{
	(void) length;
	(void) client;

	fprintf(stderr, "download %.03lf%% complete\n", 100 * webkit_download_get_estimated_progress(download));

	// TODO: graphical progress bar
}

static void
download_on_failed(WebKitDownload *download, WebKitDownloadError error, struct browse_client *client)
{
	(void) download;
	(void) client;

	fprintf(stderr, "download error: ");
	switch (error) {
	case WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER: fprintf(stderr, "CANCELLED_BY_USER"); break;
	case WEBKIT_DOWNLOAD_ERROR_DESTINATION: fprintf(stderr, "DESTINATION"); break;
	case WEBKIT_DOWNLOAD_ERROR_NETWORK: fprintf(stderr, "NETWORK"); break;
	default: fprintf(stderr, "UNKNOWN"); break;
	}
	fprintf(stderr, "\n");

	// TODO: graphical error
}
