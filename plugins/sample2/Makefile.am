libsample2_LTLIBRARIES = libsample2.1.0.la
libsample2_1_0_la_SOURCES = sample2.cpp ../../framework/XEventLog.cpp sample2.h sample2.conf 
sample2_1_0_la_LD_FLAGS = -avoid-version -module -shared -export-dynamic

libsample2dir = ${prefix}/plugins/sample2
libsample2_DATA = sample2.conf
AM_CXXFLAGS = -I../../framework -I../../generic_plugin
