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
	array.cpp \
	buffer.cpp \
	deribit.cpp \
	execution_report.cpp \
	heartbeat.cpp \
	instrument.cpp \
	logon.cpp \
	logout.cpp \
	market_data_incremental_refresh.cpp \
	market_data_request_reject.cpp \
	market_data_snapshot_full_refresh.cpp \
	order_cancel_reject.cpp \
	parser.cpp \
	position_qty.cpp \
	position_report.cpp \
	reject.cpp \
	resend_request.cpp \
	security_list.cpp \
	test_request.cpp \
	user_response.cpp \
	utils.cpp
