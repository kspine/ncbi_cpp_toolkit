# $Id$

APP = odbc95_transaction
SRC = transaction common

CPPFLAGS = -DHAVE_CONFIG_H=1 $(FTDS95_INCLUDE) $(ODBC_INCLUDE) $(ORIG_CPPFLAGS)
LIB      = odbc_ftds95$(STATIC) tds_ftds95$(STATIC) odbc_ftds95$(STATIC)
LIBS     = $(FTDS95_CTLIB_LIBS) $(NETWORK_LIBS) $(RT_LIBS) $(C_LIBS)
LINK     = $(C_LINK)

CHECK_CMD  = test-odbc95.sh --no-auto odbc95_transaction
CHECK_COPY = test-odbc95.sh odbc.ini

CHECK_REQUIRES = in-house-resources

WATCHERS = ucko