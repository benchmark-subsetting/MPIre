lib_LTLIBRARIES = libmpire_capture.la libmpire_replay.la

libmpire_capture_la_CFLAGS = --std=gnu99 -I../ccan/ -I/usr/lib/openmpi/include/
libmpire_capture_la_SOURCES = mpire_capture.c
libmpire_capture_la_LIBADD = ../ccan/libccan.la

libmpire_replay_la_CFLAGS = --std=gnu99 -I../ccan/ -I/usr/lib/openmpi/include/
libmpire_replay_la_SOURCES = mpire_replay.c
libmpire_replay_la_LIBADD = ../ccan/libccan.la
