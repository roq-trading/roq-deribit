noinst_LTLIBRARIES = libroq-deribit-fix.la
libroq_deribit_fix_la_LDFLAGS = \
	-static \
	$(LIBCONFIG_LIBS) \
	$(ROQ_LIBS)
libroq_deribit_fix_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(LIBCONFIG_CFLAGS) \
	$(ROQ_CFLAGS)
libroq_deribit_fix_la_SOURCES = \
	heartbeat.cpp \
	logon.cpp \
	logout.cpp \
	parser.cpp \
	utils.cpp
