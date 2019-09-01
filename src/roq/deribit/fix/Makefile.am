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
	execution_report.cpp \
	heartbeat.cpp \
	logon.cpp \
	logout.cpp \
	market_data_incremental_refresh.cpp \
	market_data_request_reject.cpp \
	market_data_snapshot_full_refresh.cpp \
	parser.cpp \
	position_report.cpp \
	resend_request.cpp \
	security_list.cpp \
	test_request.cpp \
	utils.cpp
