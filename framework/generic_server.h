/*
   Copyright 2013 Broadcom Corporation

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 2.1  as published by the Free Software Foundation.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


/**
 * \file generic_server.h
 * \brief Singleton class to manage framework state and provide utility functions.
 * This class stores master Vector, where all plug-ins are stored. It provides mechanisms
 * to access all loaded plug-ins, loads and unloads plug-ins and provides TCP socket and TLS
 * functionality to plug-ins.
 *
 * \author Suraj Vijayan
 *
 * \date : 2011/12/27 14:16:20
 *
 * Contact: suraj@broadcom.com
 *
 */
#ifndef GENERIC_SERVER_H
#define GENERIC_SERVER_H

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
#ifndef WINDOWS
#include <pthread.h>
#else
#include "XEventLog.h"
#endif
#include "sslserver.h"
#define NO_LOG      0
#define LOG_LOW     1
#define LOG_MEDIUM  2
#define LOG_HI      3
#define MAX_THREADS 100
#define PLUGIN_NAME        0
#define PLUGIN_TYPE        1
#define PORT               2
#define PLUGIN_NUMBER      3
#define SHARED_LIB_PATH    4
#define TLS_FLAG           5
#define PLUGIN_CONF        6
#define INIT               1
#define RELOAD             2
#define THREAD_FREE        0
#define THREAD_COMPLETED   2
#define THREAD_ALLOCATED   1
#define MAX_SZ             10000
#define MIN_REQUEST_NO     0
#define MAX_REQUEST_NO     100
/// OS agnostic thread handle.
typedef struct thread_info
{
    int state;
#ifdef WINDOWS
        HANDLE th;
#else
        pthread_t th;
#endif
} THREAD_INFO;
typedef GENERIC_PLUGIN* (*create_fp)();    
/// This is a singleton class and provides framework functionality.
#ifdef WINDOWS
class LIBTYPE generic_server
#else
class generic_server
#endif
{
public:
    /// Instantiate tis singleton class only once and return pointer to instantiated object later on..
    static generic_server *instance(void); 
    /** Reads master configuration file, loads plug-in shared libraries and initializes master Vector
        to store plug-ins.
    */
    int init();
    /// enables logging to framework managed log file.
    int log(unsigned int,unsigned int,char *);
    /// enables logging to framework managed log file.
    int log(unsigned int,unsigned int,string );
    int get_log_size(void);
    /// Rolls over log files, configured in Master configuation file.
    int rollover_logs(void);
    int close_log(void);
    /// Really useful only for Windows, just invokes WSAStartup API.
    int sock_init(void);
    /// A simple utility to swap upper / lower 8 bytes of a 16 bytes buffer.
    int swap_bytes(unsigned char *key);
    /**
    Asynchronous socket read utility.

    \param[out]   sav_buffer Buffer to store read data.
    \param[in]    length     Maximum number of bytes to read from socket.
    \return Number of bytes actually read, or zero on error. failure to read any data.
    */
    int bb_getline(char *sav_buffer,size_t length,size_t conn_s);
    int bb_getline(char *sav_buffer,size_t length,size_t conn_s,int);
    int get_status(unsigned char *buff);
    string get_cur_time(void);
    string trim(const std::string&);
    int socket_close(SOCKET);
	int close_thread_ids(void);
    /**
    Creates a 'listen'ing socket for every plugin, sets socket to 'nonblocking' mode.

    \param[in]  port TCP port for socket to bind.
    \return socket descriptor on success or zero on error.
    */
    SOCKET create_socket(int);
    bool get_verify_client(void);
    int set_verify_client(bool);
    int check_enabled_plugin(string);
    int check_allocated_port(string in_plugin_id,string in_plugin_port);
    int set_conf_file(char *cfile);
    int set_process_name(char *cfile);
    int reload_conf_file(void); 
    int get_command_port(void);
    int plugin_read_conf_file(GENERIC_PLUGIN *,int thread_no);
    int syslog_msg(unsigned int debug_level,unsigned int verbose_level,string data);
    string get_framework_version(void) {return("0_0_0_9");};
    int print_key_message();
    int get_thread_id();
    int rel_thread_id(int);
    /**
    Checks for 'alias' plug-ins. An 'alias' is any plug-in that uses the same TCP port of 
    primary plug-in. Framework would use one TCP port/socket to service all 'aliased' plug-ins.
    \param[in] &alias Reference to alias Vector within every 
    */
    bool check_plugin_aliases(vector<string> &alias);
    int set_plugins_path(vector<string> &plugin_parms);
    string get_plugins_path();
    int setnonblocking(SOCKET);
    int setblocking(SOCKET);
    int  get_thread_sync_mutex(void);
    int  rel_thread_sync_mutex(void);
    int ssl_async_write(SSL *ssl,char *buf,int size);
    int ssl_async_read(SSL *ssl,char *buf,int size);
	int ssl_async_accept(SSL *ssl);
#ifdef WINDOWS
    BOOL get_exe_directory(TCHAR *obuf, size_t osize);
#endif
    vector<string> split(const string& s, const string& delim, const bool keep_empty);
/**
    Dynamically loads plug-in shared library.

    \param[in] library_image Path to shared library image.
    \param[in] plugin_type   PLUGIN_TYPE to load.
    \return function pointer to 'create_instance()' factory method of loaded plug-in. NULL on error.
*/
    GENERIC_PLUGIN *load_shared_lib(string library_image,string);
    int unload_shared_lib(string plugin_type);
    vector <GENERIC_PLUGIN *>::iterator get_start_iterator(void);
    vector <GENERIC_PLUGIN *>::iterator get_end_iterator(void);
    vector <GENERIC_PLUGIN *>::iterator itr;
    /// Vector to hold all plug-ins.
    vector <GENERIC_PLUGIN *> v;

    unsigned int timeout_seconds;
    int max_threads,command_port;
    ofstream log_file;
    string logname;
#ifdef WINDOWS
    map <string,HANDLE> dll_handles;
    HANDLE hThread;
#else
    map <string,void *> dll_handles;
#endif
    /// Map to store all currently running threads.
    map <int,THREAD_INFO> thread_ids;
    /// Pointer to singleton framework instance.
    static generic_server *pinstance;
    /// Map that has function pointers to 'create_instance()' factory method for all plug-in libraries.
    map <string,create_fp> factory;
    SSLServer sslserver;
    bool syslog_flag;
	// STL to hold do-only-once bootstapping across plugins
	map <string,bool> bootstrap_jobs;
    /// TLS certificate stuff
    string server_cert,rsa_private_key,ca_certificate;
private:
    generic_server(void);
    int  get_log_file_mutex(void);
    int  rel_log_file_mutex(void);
    int get_server_params(string,int);
    /// Initializes TLS.
    int tls_setup(void);
    /// This is invoked when any plug-in entry gets removed from Master configuration file and framework gets signaled.
    int remove_devices(map <string,int> &new_ports);
    int  remove_all_aliases();
/**
*    Update plug-in object with plug-in specific configuration parameters from plug-in conf file.
*
*    \param[in] plugin_parms Vector that holds plug-in configuration details.
*    \param[in] plugin       Plug-in object to be updated.
*    \return    Updated plugin object or NULL on error.
*/
    GENERIC_PLUGIN *initialize_plugin_object(vector<string> plugin_parms,GENERIC_PLUGIN *plugin);
    /// Validates generic_server framework and CA certificates, calls tls_setup().
    int tls_initialize();
    int set_access_control(char daemon_flag);
    unsigned int iResult,tls_setup_reqd;
    string conf_file,process_name,plugins_path;
#ifdef WINDOWS
	CXEventLog el;
    HANDLE log_file_mutex,thread_cnt_mutex,thread_sync_mutex;
#else
    pthread_mutex_t log_file_mutex,thread_cnt_mutex,thread_sync_mutex;
#endif
    unsigned long max_log_size;
    int max_logs_saved;
    bool verify_client;
};
/// Every plug-in should have a function with following prototype. This is the factory method: 'create_instance()' within shared library.
typedef GENERIC_PLUGIN* (*create_fp)();
#endif 
