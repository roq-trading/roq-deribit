TESTS = roq-deribit-benchmark
bin_PROGRAMS = roq-deribit-benchmark
roq_deribit_benchmark_LDADD = \
	$(top_builddir)/src/roq/deribit/fix/libroq-deribit-fix.la \
	$(top_builddir)/src/roq/deribit/json/libroq-deribit-json.la \
	$(DATE_LIBS) \
	$(ROQ_CORE_LIBS) \
	$(ROQ_API_LIBS) \
	$(ROQ_LOGGING_LIBS) \
	$(FMT_LIBS) \
	$(BENCHMARK_LIBS)  \
	$(RT_LIBS) \
	-lpthread
roq_deribit_benchmark_CFLAGS = -pthread
roq_deribit_benchmark_CPPFLAGS = \
	-I$(top_builddir)/src \
	$(ROQ_LOGGING_CFLAGS) \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CORE_CFLAGS) \
	$(BENCHMARK_CFLAGS) \
	$(FMT_CFLAGS) \
	$(DATE_CFLAGS)
nodist_roq_deribit_benchmark_SOURCES = \
	fix_execution_report.cpp \
	fix_heartbeat.cpp \
	fix_logon.cpp \
	fix_logout.cpp \
	fix_market_data_incremental_refresh.cpp \
	fix_market_data_request_reject.cpp \
	fix_market_data_snapshot_full_refresh.cpp \
	fix_new_order_single.cpp \
	fix_order_cancel_reject.cpp \
	fix_order_cancel_replace_request.cpp \
	fix_order_cancel_request.cpp \
	fix_position_report.cpp \
	fix_reject.cpp \
	fix_security_list.cpp \
	fix_user_response.cpp \
	main.cpp
