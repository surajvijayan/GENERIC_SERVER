libsample_LTLIBRARIES = libsample.1.0.la
libsample_1_0_la_SOURCES = sample.cpp ../../framework/XEventLog.cpp sample.h sample.conf 
sample_1_0_la_LD_FLAGS = -avoid-version -module -shared -export-dynamic

libsampledir = ${prefix}/plugins/sample
libsample_DATA = sample.conf
AM_CXXFLAGS = -I../../framework -I../../generic_plugin
