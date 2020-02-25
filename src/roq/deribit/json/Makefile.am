noinst_LTLIBRARIES = libroq-deribit-json.la
libroq_deribit_json_la_LDFLAGS = \
	-static \
	$(LIBCONFIG_LIBS) \
	$(ROQ_LIBS)
libroq_deribit_json_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(LIBCONFIG_CFLAGS) \
	$(ROQ_CFLAGS)
BUILT_SOURCES = \
	currency.cpp \
	currency.h \
	field.cpp \
	field.h \
	instrument.cpp \
	instrument.h \
	kind.cpp \
	kind.h \
	option_type.cpp \
	option_type.h \
	order_book.cpp \
	order_book.h \
	state.cpp \
	state.h \
	ticker.cpp \
	ticker.h
CLEANFILES = $(BUILT_SOURCES)
libroq_deribit_json_la_SOURCES = \
	currency.cpp \
	instrument.cpp \
	kind.cpp \
	message.cpp \
	option_type.cpp \
	order_book.cpp \
	state.cpp \
	ticker.cpp \
	utils.cpp

.json.h:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit/json \
		--output-type json \
		--file-type h \
		$< > $@

.json.cpp:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit/json \
		--output-type json \
		--file-type cpp \
		$< > $@
