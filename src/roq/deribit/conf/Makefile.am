noinst_LTLIBRARIES = libroq-deribit-conf.la
libroq_deribit_conf_la_LDFLAGS = \
	-static \
	$(ROQ_CORE_LIBS) \
	$(ROQ_LIBS)
libroq_deribit_conf_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(ROQ_CORE_CFLAGS) \
	$(ROQ_CFLAGS)
libroq_deribit_conf_la_SOURCES = \
	account.cpp \
	config.cpp \
	connection.cpp
