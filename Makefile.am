SUBDIRS = . src test

if ENABLE_BENCHMARK
SUBDIRS += benchmark
endif

dist_doc_DATA = LICENSE README.md

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE@.pc

configdir=$(datadir)/roq/deribit
config_DATA = \
	share/accounts.conf \
	share/fix_api.conf \
	share/master.conf \
	share/rest_api.conf \
	share/symbols.conf \
	share/users.conf \
	share/variables.conf \
	share/websocket_api.conf

.PHONY: lint

lint:
	cpplint --linelength=120 \
		--recursive src/ \
		--recursive test/ \
		--recursive benchmark/

cppcheck:
	cppcheck --language=c++ --std=c++11 src/
