noinst_LTLIBRARIES = libroq-deribit-fix.la
libroq_deribit_fix_la_LDFLAGS = \
	-static \
	$(LIBCONFIG_LIBS) \
	$(ROQ_LIBS)
libroq_deribit_fix_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(LIBCONFIG_CFLAGS) \
	$(ROQ_CFLAGS)
AUTOGEN_CPP = \
	execution_report.cpp \
	fill.cpp \
	heartbeat.cpp \
	instrmt_md_req.cpp \
	instrument.cpp \
	logon.cpp \
	logout.cpp \
	market_data_incremental_refresh.cpp \
	market_data_request.cpp \
	market_data_request_reject.cpp \
	market_data_snapshot_full_refresh.cpp \
	md_full.cpp \
	md_inc.cpp \
	md_req.cpp \
	new_order_single.cpp \
	order_cancel_reject.cpp \
	order_cancel_replace_request.cpp \
	order_cancel_request.cpp \
	order_mass_status_request.cpp \
	position_qty.cpp \
	position_report.cpp \
	reject.cpp \
	request_for_positions.cpp \
	resend_request.cpp \
	security_list.cpp \
	security_list_request.cpp \
	test_request.cpp \
	user_request.cpp \
	user_response.cpp
AUTOGEN_H = \
	execution_report.h \
	fill.h \
	heartbeat.h \
	instrmt_md_req.h \
	instrument.h \
	logon.h \
	logout.h \
	market_data_incremental_refresh.h \
	market_data_request.h \
	market_data_request_reject.h \
	market_data_snapshot_full_refresh.h \
	md_full.h \
	md_inc.h \
	md_req.h \
	new_order_single.h \
	order_cancel_reject.h \
	order_cancel_replace_request.h \
	order_cancel_request.h \
	order_mass_status_request.h \
	position_qty.h \
	position_report.h \
	reject.h \
	request_for_positions.h \
	resend_request.h \
	security_list.h \
	security_list_request.h \
	test_request.h \
	user_request.h \
	user_response.h
BUILT_SOURCES = $(AUTOGEN_CPP) $(AUTOGEN_H)
CLEANFILES = $(BUILT_SOURCES)
libroq_deribit_fix_la_SOURCES = \
	$(AUTOGEN_CPP) \
	adv_order_type.cpp \
	utils.cpp

.json.h:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit/fix \
		--output-type fix \
		--file-type h \
		$< > $@

.json.cpp:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/deribit/fix \
		--output-type fix \
		--file-type cpp \
		$< > $@
