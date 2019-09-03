TESTS = roq-deribit-api2-benchmark
bin_PROGRAMS = roq-deribit-api2-benchmark
roq_deribit_api2_benchmark_LDADD = \
	$(top_builddir)/src/roq/deribit/conf/libroq-deribit-conf.la \
	$(top_builddir)/src/roq/deribit/fix/libroq-deribit-fix.la \
	$(top_builddir)/src/roq/deribit/json/libroq-deribit-json.la \
	$(top_builddir)/src/roq/deribit/api/libroq-deribit-api.la \
	$(DATE_LIBS) \
	$(ROQ_API_LIBS) \
	$(ROQ_CORE_LIBS) \
	$(FMT_LIBS) \
	$(BENCHMARK_LIBS)  \
	-lpthread
roq_deribit_api2_benchmark_CFLAGS = -pthread
roq_deribit_api2_benchmark_CPPFLAGS = \
	-I$(top_builddir)/src \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CORE_CFLAGS) \
	$(BENCHMARK_CFLAGS) \
	$(FMT_CFLAGS) \
	$(DATE_CFLAGS)
nodist_roq_deribit_api2_benchmark_SOURCES = \
	fix_logon.cpp \
	main.cpp
