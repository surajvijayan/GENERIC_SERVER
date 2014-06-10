AM_CXXFLAGS = -DLINUX -I../generic_plugin -g
bin_PROGRAMS = generic_server
generic_server_SOURCES = generic_server.cpp control.cpp sslserver.cpp XEventLog.cpp service.cpp generic_server.h control.h sslserver.h mainpage.h XEventLog.h generic_plugin.h
AM_LDFLAGS = -Wl,-rpath,${prefix}/lib -lgeneric_plugin.1.0 -lpthread -L../generic_plugin
