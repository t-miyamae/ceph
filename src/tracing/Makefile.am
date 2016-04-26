EXTRA_DIST += \
	tracing/tracing-common.h

if WITH_LTTNG
tracing/%.h: tracing/%.tp
	$(LTTNG_GEN_TP_PROG) $< -o tracing/$*.h
endif

dist_noinst_DATA += \
	tracing/librados.tp \
	tracing/librbd.tp \
	tracing/oprequest.tp \
	tracing/osd.tp \
	tracing/pg.tp \
	tracing/objectstore.tp \
	tracing/rgw_op.tp \
	tracing/rgw_process.tp \
	tracing/rgw_rados.tp

if WITH_LTTNG
libosd_tp_la_SOURCES = \
	tracing/oprequest.c \
	tracing/osd.c \
	tracing/pg.c
nodist_libosd_tp_la_SOURCES = \
	tracing/oprequest.h \
	tracing/osd.h \
	tracing/pg.h
endif
libosd_tp_la_LIBADD = -ldl -llttng-ust
libosd_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
libosd_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
librados_tp_la_SOURCES = \
	tracing/librados.c
nodist_librados_tp_la_SOURCES = \
	tracing/librados.h
endif
librados_tp_la_LIBADD = -ldl -llttng-ust
librados_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
librados_tp_la_LDFLAGS = -version-info 2:0:0

if WITH_LTTNG
librbd_tp_la_SOURCES = \
	tracing/librbd.c
nodist_librbd_tp_la_SOURCES = \
	tracing/librbd.h
endif
librbd_tp_la_LIBADD = -ldl -llttng-ust
librbd_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
librbd_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
libos_tp_la_SOURCES = \
	tracing/objectstore.c
nodist_libos_tp_la_SOURCES = \
	tracing/objectstore.h
endif
libos_tp_la_LIBADD = -ldl -llttng-ust
libos_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
libos_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
librgw_op_tp_la_SOURCES = \
        tracing/rgw_op.c
nodist_librgw_op_tp_la_SOURCES = \
        tracing/rgw_op.h
endif
librgw_op_tp_la_LIBADD = -ldl -llttng-ust
librgw_op_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
librgw_op_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
librgw_process_tp_la_SOURCES = \
        tracing/rgw_process.c
nodist_librgw_process_tp_la_SOURCES = \
        tracing/rgw_process.h
endif
librgw_process_tp_la_LIBADD = -ldl -llttng-ust
librgw_process_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
librgw_process_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
librgw_rados_tp_la_SOURCES = \
        tracing/rgw_rados.c
nodist_librgw_rados_tp_la_SOURCES = \
        tracing/rgw_rados.h
endif
librgw_rados_tp_la_LIBADD = -ldl -llttng-ust
librgw_rados_tp_la_CFLAGS = -I$(top_srcdir)/src/tracing -I$(top_srcdir)/src $(AM_CFLAGS) -fpic
librgw_rados_tp_la_LDFLAGS = -version-info 1:0:0

if WITH_LTTNG
lib_LTLIBRARIES += \
	libosd_tp.la \
	libos_tp.la \
	librados_tp.la \
	librbd_tp.la \
	librgw_op_tp.la \
	librgw_process_tp.la \
	librgw_rados_tp.la

BUILT_SOURCES += \
	tracing/librados.h \
	tracing/librbd.h \
	tracing/objectstore.h \
	tracing/oprequest.h \
	tracing/osd.h \
	tracing/pg.h \
	tracing/rgw_op.h \
	tracing/rgw_process.h \
	tracing/rgw_rados.h
endif

CLEANFILES += \
	$(BUILT_SOURCES)
