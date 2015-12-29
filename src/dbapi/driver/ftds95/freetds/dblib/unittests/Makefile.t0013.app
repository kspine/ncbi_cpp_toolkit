# $Id$

APP = db95_t0013
SRC = t0013 common

CPPFLAGS = -DHAVE_CONFIG_H=1 -DNEED_FREETDS_SRCDIR $(FTDS95_INCLUDE) \
           $(ORIG_CPPFLAGS)
LIB      = sybdb_ftds95$(STATIC) tds_ftds95$(STATIC)
LIBS     = $(FTDS95_CTLIB_LIBS) $(NETWORK_LIBS) $(RT_LIBS) $(C_LIBS)
LINK     = $(C_LINK)

# Needs automatic textptr metadata, gone as of TDS 7.2.
CHECK_CMD  = test-db95.sh --ms-ver 7.1 --no-auto db95_t0013
CHECK_COPY = test-db95.sh t0013.sql data.bin

CHECK_REQUIRES = in-house-resources

WATCHERS = ucko