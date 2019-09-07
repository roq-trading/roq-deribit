# SO 35998856
check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = \
        $(top_srcdir)/third_party/googletest/googletest/src/gtest-all.cc
libgtest_la_CPPFLAGS = \
        -I$(top_srcdir)/third_party/googletest/googletest/include \
        -I$(top_srcdir)/third_party/googletest/googletest
libgtest_la_LDFLAGS = -pthread

TESTS = roq-deribit-api2-test
check_PROGRAMS = roq-deribit-api2-test

roq_deribit_api2_test_LDADD = \
	libgtest.la \
	$(top_builddir)/src/roq/deribit/conf/libroq-deribit-conf.la \
	$(top_builddir)/src/roq/deribit/fix/libroq-deribit-fix.la \
	$(top_builddir)/src/roq/deribit/json/libroq-deribit-json.la \
	$(top_builddir)/src/roq/deribit/api/libroq-deribit-api.la \
	$(DATE_LIBS) \
	$(ROQ_CORE_LIBS) \
	$(ROQ_API_LIBS) \
	$(FMT_LIBS) \
	$(DATE_LIBS)
roq_deribit_api2_test_CPPFLAGS = \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/third_party/googletest/googletest/include \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CORE_CFLAGS) \
	$(FMT_CFLAGS) \
	$(DATE_CFLAGS)
nodist_roq_deribit_api2_test_SOURCES = \
	fix_heartbeat.cpp \
	fix_logon.cpp \
	fix_logout.cpp \
	fix_market_data_incremental_refresh.cpp \
	json_currencies.cpp \
	json_instruments.cpp \
	json_ticker.cpp \
	main.cpp
