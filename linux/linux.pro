TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpthread

SOURCES += main.c \
    comm/rs232.c \
    jansson/dump.c \
    jansson/error.c \
    jansson/hashtable.c \
    jansson/hashtable_seed.c \
    jansson/load.c \
    jansson/memory.c \
    jansson/pack_unpack.c \
    jansson/strbuffer.c \
    jansson/strconv.c \
    jansson/utf.c \
    jansson/value.c \
    ringbuf.c

HEADERS += \
    comm/rs232.h \
    jansson/hashtable.h \
    jansson/jansson.h \
    jansson/jansson_config.h \
    jansson/jansson_private.h \
    jansson/lookup3.h \
    jansson/strbuffer.h \
    jansson/utf.h \
    ringbuf.h \
    types_donnees.h
