SUBDIRS = fix json

bin_PROGRAMS = roq-deribit
roq_deribit_LDFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(top_builddir)/src/roq/deribit/fix/libroq-deribit-fix.la \
	$(top_builddir)/src/roq/deribit/json/libroq-deribit-json.la \
	$(ROQ_SERVER_LIBS) \
	$(ROQ_CORE_LIBS) \
	$(ROQ_API_LIBS) \
	$(BROTLI_LIBS) \
	$(HTTP_PARSER_LIBS) \
	$(FMT_LIBS) \
	$(LIBUCL_LIBS) \
	$(LIBEVENT_LIBS) \
	-levent_openssl \
	$(OPENSSL_LIBS) \
	$(CROSSGUID_LIBS) \
	$(CRYPTO_LIBS) \
	$(PTHREAD_LIBS) \
	$(RT_LIBS) \
	$(GFLAGS_LIBS)
roq_deribit_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(PTHREAD_CFLAGS) \
	$(GFLAGS_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(LIBEVENT_CFLAGS) \
	$(LIBUCL_CFLAGS) \
	$(FMT_CFLAGS) \
	$(HTTP_PARSER_CFLAGS) \
	$(BROTLI_CFLAGS) \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CORE_CFLAGS) \
	$(ROQ_SERVER_CFLAGS)
AUTOGEN_CPP = \
	fix_state.cpp \
	web_socket_state.cpp
AUTOGEN_H = \
	fix_state.h \
	web_socket_state.h
CLEANFILES = $(AUTOGEN_CPP) $(AUTOGEN_H)
BUILT_SOURCES = $(CLEANFILES) .gitignore
roq_deribit_SOURCES = \
	$(AUTOGEN_CPP) \
	application.cpp \
	common.cpp \
	config.cpp \
	fix.cpp \
	gateway.cpp \
	options.cpp \
	random.cpp \
	web_socket.cpp \
	main.cpp

.gitignore: Makefile.am
	echo $(AUTOGEN_CPP) $(AUTOGEN_H) | tr " " "\n" | sort > $@

.json.h:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit \
		--output-type state \
		--file-type h \
		$< > $@

.json.cpp:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit \
		--output-type state \
		--file-type cpp \
		$< > $@
