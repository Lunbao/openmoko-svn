CFLAGS = `pkg-config --cflags edje glib-2.0 pygtk-2.0` `python-config --includes`
LDFLAGS = `pkg-config --libs edje glib-2.0 pygtk-2.0` `python-config --libs`

EDJE_PATH = $(E_PATH)/edje/src/lib
LIB_NAME = edje.so

.PHONY: all clean examples

all: $(LIB_NAME)

$(LIB_NAME): edje.o edjemodule.o
	$(CC) $(LDFLAGS) -shared $^ -o $@

edje.c: edje.defs edje.override
	$(PYTHON) $(CODEGEN_PATH)/codegen.py --prefix edje --override edje.override edje.defs > $@

edje.defs: $(CODEGEN_PATH)/h2def.py $(EDJE_PATH)/Edje.h $(EDJE_PATH)/Edje_Edit.h
	$(CODEGEN_PATH)/h2def.py $(EDJE_PATH)/Edje.h > edje.defs
	$(CODEGEN_PATH)/h2def.py $(EDJE_PATH)/Edje_Edit.h >> edje.defs

clean:
	rm -f *.o *~ edje.defs edje.c $(LIB_NAME) $(EXAMPLE_PATH)/$(LIB_NAME)

examples: $(LIB_NAME) $(EXAMPLE_PATH)/$(LIB_NAME)

$(EXAMPLE_PATH)/$(LIB_NAME): $(LIB_NAME)
	cp $(LIB_NAME) $(EXAMPLE_PATH)
