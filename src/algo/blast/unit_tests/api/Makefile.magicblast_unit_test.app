# $Id$

APP = magicblast_unit_test
SRC = magicblast_unit_test 

CPPFLAGS = -DNCBI_MODULE=BLAST $(ORIG_CPPFLAGS) $(BOOST_INCLUDE)
LIB = test_boost $(BLAST_INPUT_LIBS) \
    $(BLAST_LIBS) xobjsimple $(OBJMGR_LIBS:ncbi_x%=ncbi_x%$(DLL))

CHECK_REQUIRES = MT in-house-resources
CHECK_CMD = magicblast_unit_test
CHECK_COPY = data

WATCHERS = boratyng