#include "browse.h"

static char const start_page[] = "https://searx.mblenczewski.com";
static char const search_page[] = "https://searx.mblenczewski.com/search?q=%s";

static char const download_fpath_fmt[] = "/tmp/%s";

// https://docs.gtk.org/gtk4/class/.Settings.html#Properties
static const struct browse_gtk_setting gtk_settings[] = {
	{ .name = "gtk-enable-animations", .v = { false }, },
	{ .name = "gtk-application-prefer-dark-theme", .v = { true }, },
};

// https://webkitgtk.org/reference/webkit2gtk/stable/class.Settings.html#Properties
static const struct browse_webkit_setting webkit_settings[] = {
	{ .name = "allow-file-access-from-file-urls", .v = { true }, },
	{ .name = "enable-developer-extras", .v = { true }, },
	{ .name = "enable-webgl", .v = { true }, },
	{ .name = "enable-smooth-scrolling", .v = { true }, },
};

static const union browse_prop window_default_props[_BROWSE_PROP_TYPE_COUNT] = {
	[BROWSE_PROP_STRICT_TLS] = { .b = true, },
};

#define BOOKMARK_FILE "$HOME/.browse/bookmarks"

#define SEARCH_PROC { .s = "cat " BOOKMARK_FILE " | bemenu -l 10 -p 'Search: '", }

#define MODKEY GDK_CONTROL_MASK

static const struct browse_keybind keybinds[] = {
	/* modifier			keyval		handler		argument */
	{ 0,				GDK_KEY_Escape,	stopload,	{ 0 }, },
	{ MODKEY,			GDK_KEY_c,	stopload,	{ 0 }, },

	{ MODKEY|GDK_SHIFT_MASK,	GDK_KEY_R,	reload,		{ .i = 1, }, },
	{ MODKEY,			GDK_KEY_r, 	reload,		{ .i = 0, }, },

	{ MODKEY|GDK_SHIFT_MASK,	GDK_KEY_H, 	navigate,	{ .i = -1, }, },
	{ MODKEY|GDK_SHIFT_MASK, 	GDK_KEY_L, 	navigate,	{ .i = +1, }, },

	{ MODKEY,			GDK_KEY_h,	javascript,	{ .s = "window.scrollBy(-100, 0);", }, },
	{ MODKEY,			GDK_KEY_j,	javascript,	{ .s = "window.scrollBy(0, +100);", }, },
	{ MODKEY,			GDK_KEY_k,	javascript,	{ .s = "window.scrollBy(0, -100);", }, },
	{ MODKEY,			GDK_KEY_l,	javascript,	{ .s = "window.scrollBy(+100, 0);", }, },

	{ MODKEY, 			GDK_KEY_y, 	clipboard,	{ .i = 0, }, },
	{ MODKEY, 			GDK_KEY_p, 	clipboard,	{ .i = 1, }, },

	{ MODKEY,			GDK_KEY_g,	search,		SEARCH_PROC, },

	{ MODKEY|GDK_SHIFT_MASK,	GDK_KEY_T,	toggle,		{ .i = BROWSE_PROP_STRICT_TLS, }, },
};
