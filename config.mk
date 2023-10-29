PREFIX		?= /usr/local

OBJ		:= obj

WEBKIT_INCS	:= $(shell pkg-config --cflags webkitgtk-6.0 webkitgtk-web-process-extension-6.0)
WEBKIT_LIBS	:= $(shell pkg-config --libs webkitgtk-6.0 webkitgtk-web-process-extension-6.0)

WARNINGS	:= -Wall -Wextra -Wpedantic -Werror -Wno-extra-semi

CFLAGS		:= -std=c17 $(WARNINGS) -Og -g
CPPFLAGS	:= $(WEBKIT_INCS)
LDFLAGS		:= $(WEBKIT_LIBS)
