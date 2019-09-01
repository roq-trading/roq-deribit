SUBDIRS = api fix json conf

bin_PROGRAMS = roq-deribit-api2
roq_deribit_api2_LDFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(top_builddir)/src/roq/deribit/conf/libroq-deribit-conf.la \
	$(top_builddir)/src/roq/deribit/fix/libroq-deribit-fix.la \
	$(top_builddir)/src/roq/deribit/json/libroq-deribit-json.la \
	$(top_builddir)/src/roq/deribit/api/libroq-deribit-api.la \
	$(ROQ_SERVER_LIBS) \
	$(ROQ_CORE_LIBS) \
	$(ROQ_API_LIBS) \
	$(HTTP_PARSER_LIBS) \
	$(FMT_LIBS) \
	$(LIBUCL_LIBS) \
	$(LIBEVENT_LIBS) \
	-levent_openssl \
	$(OPENSSL_LIBS) \
	-lcrypto \
	$(PTHREAD_LIBS) \
	$(RT_LIBS) \
	$(GFLAGS_LIBS)
roq_deribit_api2_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(PTHREAD_CFLAGS) \
	$(GFLAGS_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(LIBEVENT_CFLAGS) \
	$(LIBUCL_CFLAGS) \
	$(FMT_CFLAGS) \
	$(HTTP_PARSER_CFLAGS) \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CORE_CFLAGS) \
	$(ROQ_SERVER_CFLAGS)
roq_deribit_api2_SOURCES = \
	controller.cpp \
	fix.cpp \
	gateway.cpp \
	random.cpp \
	main.cpp
