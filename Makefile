.PHONY: all build clean dist distclean install uninstall

include config.mk

TARGET		:= browse

SOURCES		:= browse.c
OBJECTS		:= $(SOURCES:%=$(OBJ)/%.o)
OBJDEPS		:= $(OBJECTS:%.o=%.d)

ARCHIVE		:= $(TARGET).tar

AUX		:= Makefile config.mk config.def.h browse.h browse.c

all: build

build: $(TARGET)

clean:
	rm -rf $(OBJ) $(TARGET)

dist: build $(AUX)
	tar cf $(ARCHIVE) $(TARGET) $(AUX)

distclean: clean
	rm -rf $(ARCHIVE) config.h

install: build
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

config.h: config.def.h
	cp $< $@

$(TARGET): $(OBJECTS) | $(BIN)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJECTS): config.h

$(OBJECTS): $(OBJ)/%.c.o: %.c | $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -MMD -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

-include $(OBJDEPS)

$(OBJ):
	mkdir $@
