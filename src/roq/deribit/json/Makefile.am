noinst_LTLIBRARIES = libroq-deribit-json.la
libroq_deribit_json_la_LDFLAGS = \
	-static \
	$(LIBCONFIG_LIBS) \
	$(ROQ_LIBS)
libroq_deribit_json_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(LIBCONFIG_CFLAGS) \
	$(ROQ_CFLAGS)
AUTOGEN_CPP = \
	currency.cpp \
	error.cpp \
	field.cpp \
	instrument.cpp \
	kind.cpp \
	method.cpp \
	option_type.cpp \
	order_book.cpp \
	request_type.cpp \
	state.cpp \
	ticker.cpp
AUTOGEN_H = \
	currency.h \
	error.h \
	field.h \
	instrument.h \
	kind.h \
	method.h \
	option_type.h \
	order_book.h \
	request_type.h \
	state.h \
	ticker.h
CLEANFILES = $(AUTOGEN_CPP) $(AUTOGEN_H)
BUILT_SOURCES = $(CLEANFILES) .gitignore
libroq_deribit_json_la_SOURCES = \
	$(AUTOGEN_CPP) \
	message.cpp \
	utils.cpp

.gitignore: Makefile.am
	echo $(AUTOGEN_CPP) $(AUTOGEN_H) | tr " " "\n" | sort > $@

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
