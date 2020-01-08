bin_PROGRAMS = roq-deribit-validate
roq_deribit_validate_LDADD = \
	$(PTHREAD_LIBS) \
	$(GFLAGS_LIBS) \
	$(FMT_LIBS) \
	$(ROQ_API_LIBS) \
	$(ROQ_CLIENT_LIBS)
roq_deribit_validate_CPPFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(FMT_CFLAGS) \
	$(GFLAGS_CFLAGS) \
	$(ROQ_API_CFLAGS) \
	$(ROQ_CLIENT_CFLAGS) \
	-I$(top_srcdir)/src
roq_deribit_validate_SOURCES = \
	application.cpp \
	config.cpp \
	options.cpp \
	strategy.cpp \
	main.cpp
