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
 * \file generic_plugin.h
 * \brief This is the base class for all plugins. All plug-ins should derive from this class.
 * This class has a bunch of virtual functions that all plug-in could/should implement.
 * In addition to virtual functions, this class also provides a lot of utility functions
 * for all plug-ins.
 * 
 * \author Suraj Vijayan
 *
 * \date : 2013/07/01
 *
 * Contact: suraj@broadcom.com
 *
 */
#ifndef GENPLUGIN_H_
#define GENPLUGIN_H_
#include <iostream>
#include <string>
using namespace std;
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifdef CRYPTOPP
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptlib.h"
#include "smartptr.h"
#include "algparam.h"
#include "hex.h"
#include "files.h"
#include "hex.h"
#include "base32.h"
#include "base64.h"
#include "modes.h"
#include "cbcmac.h"
#include "dmac.h"
#include "idea.h"
#include "des.h"
#include "rc2.h"
#include "arc4.h"
#include "rc5.h"
#include "blowfish.h"
#include "3way.h"
#include "safer.h"
#include "gost.h"
#include "shark.h"
#include "cast.h"
#include "square.h"
#include "seal.h"
#include "rc6.h"
#include "mars.h"
#include "rijndael.h"
#include "twofish.h"
#include "serpent.h"
#include "skipjack.h"
#include "shacal2.h"
#include "camellia.h"
#include "osrng.h"
#include "zdeflate.h"
#include "cpu.h"
#include "crc.h"
using namespace CryptoPP;
#endif
using namespace std;
#ifndef WINDOWS

#define SOCKET int
#define sprintf_s snprintf
#define _stat stat
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SD_SEND 2
#endif
#define CONTINUE_SESSION 0xFF
#define TERMINATE_SESSION 0x00
#define PLUGIN_SIGNATURE 0xDEDDAECF
#define HEX 0
#define BIN 1

#ifdef WINDOWS
#pragma warning(disable:4251)
#define GEN_PLUGIN_MUTEX HANDLE
#define INIT_MUTEX CreateMutex( NULL, FALSE, NULL );
#else
#define GEN_PLUGIN_MUTEX pthread_mutex_t
#define INIT_MUTEX PTHREAD_MUTEX_INITIALIZER;
#endif
/// This component provides functionality that are common across plug-ins. Framework would instantiate and load objects of type GENERIC_PLUGIN to framework system from plugin shared library.
#ifdef WINDOWS
class LIBTYPE generic_plugin
{
friend LIBTYPE ostream& operator<<(ostream& output, generic_plugin& p);
#else
class generic_plugin
{
friend ostream& operator<<(ostream& output, generic_plugin& p);
#endif
protected:
/**	socket used by framework to LISTEN/SELECT clients.	
*/
    SOCKET server_socket;
/**	socket passed on to plug-ins by framework after a successful client connect.
*/
    SOCKET client_socket;
    int port,tls_enabled,thread_id;
    char *local_ip_address;
    string plugin_type;
    string plugin_name;
    string plugin_number;
    string plugin_lib_path;
    unsigned int networking,verbose_level;
/** Flag to indcate whether framework should authorize any client request to this plug-in.
*/
    bool validate_plugin;
/** 
*	A plug-in can have one or more 'alias' plug-ins. Essentially, one port,plug-in_type get
*	multi-plexed for all alias plug-ins.
*/
    vector<string> v_plugin_aliases;
    int log(unsigned int,char *);
    int log(unsigned int,string );

public:
/// Pointer to 'framework' object 
    unsigned long pinstance; 
    string plugin_conf_file;
    string generic_plugin_version();
    generic_plugin(void);
    generic_plugin(char *,int);
    generic_plugin(char *,char *,int);
/** OS agnostic wrapper API to create mutex
*/
    GEN_PLUGIN_MUTEX create_mutex(void);
/** OS agnostic wrapper API to lock mutex
*/
    int lock_mutex(GEN_PLUGIN_MUTEX *);
/** OS agnostic wrapper API to release mutex
*/
    int rel_mutex(GEN_PLUGIN_MUTEX *);
    bool get_validate_plugin(void);
    virtual unsigned short get_session(void) {return(0);};
/**
*   Initialize plug-in
*
*	This method gets invoked whenever a new client session gets initiated.
*   \param[in]    thread_no  The thread number assigned to plug-in.
*   \return       1 on success, 0 on failure.
*/
    virtual int plugin_init(int thread_no) {return(0);};
/**
*   Read plug-in configuration data
*
*	Framework presents configuration information from plug-in conf file to be processed.
*	This method will be invoked once per line in plug-in conf file.
*   \param[in]  line One line from plug-in conf file.
*   \return       1 on success, 0 on failure.
*/
    virtual int get_plugin_params(string line);
/**
*   Handle client request 
*
*	Framework presents client input data to plug-in to process.
*   \param[in]  buffer  Request sent by client.
*   \param[out] outbuff Response to be sent back to client.
*   \param[inout]  send_bytes  Size of input buffer sent by client, function sets this to size of outbuff.
*   \return     TERMINATE_SESSION to indicate plug-in done with the session with client or
*				CONTINUE_SESSION to indicate plug-in want to continue session with client and process more requests.
*/
    virtual int process_request(void *buffer, void *out_buff,unsigned int &size) {return(0);};
/**
*	This method gets invoked by the framework once per plug-in type when it is getting started. 
*	Please note, this method is invoked only ONCE for a particular plug-in type.
*	If there are three plug-ins in generic_server conf file, all of them belonging to same
*	plug-in_type, then this method gets called only once when the first plug-in is loaded.
*	Please note, this method gets invoked during framework start-up in the MAIN thread.
*	This would be the place to do stuff like:
*
*\li	Any one-time initialization (not per session) required by one or plug-in(s) belonging to
*       same plug-in type
*\li	Store any information required to be shared across multiple instances of a 
*		plug-in into plug-in object as 'static' variable(s)
*/
    virtual int server_init(void){return(1);};
/**
*	This is the opposite of 'server_init()', framework invokes this method when a plug-in
*	is removed from generic_server conf file. Similar to 'server_init()', 'server_shutdown()' 
*	gets invoked only once when the last plug-in of a plug-in_type is removed.
*/
   virtual int server_shutdown(void){return(1);};
   virtual string get_plugin_version(void) {return("UNKNOWN");};
/**
*   Terminate plug-in session
*
*	This method gets invoked whenever a client session gets terminated.
*   \return       1 on success, 0 on failure.
*/
/**
*	This is the opposite of 'plugin_init()', framework invokes this method when a client session
*	is getting terminated.
*/
    virtual int shutdown_plugin(void) {return(1);};
/**
*	GENERIC_SERVER framework provides a feature where plug-ins could request framework to call one or more bootstrap methods
*	when framework is being bootsrapped. Any plug-in could implement 'bootstrap' methods.
*	'bootstrap_name' is the name associated with a specific 'bootstrap' method. Plug-ins should be co-operative
*   and set differnet bootstrap_names to identify and implement different tasks. Under no circumstances 
*   should different plug-ins define the same 'bootstrap_name', if such a case should ever occur, framework will ignore
*   the latter defined and implemented bootstrap method.
*/
virtual string bootstrap_name(void);
/**
*	This method gets invoked by the framework when it is being booted up.
*	This method is invoked only once per 'bootstrap_name', each time framework is started up.
*	If there are three plug-ins in generic_server conf file, all of them belonging to different
*	plug-in types and each of them defines the same 'bootstrap_name', then this method gets called 
*   only once when the first plug-in is loaded
*	Please note, this method gets invoked during framework start-up
*	time - not during client session initiation. This would be the place to do stuff
*	like:
*\li	Any one-time initialization (not per session) required by plug-ins belonging to different plug-in types
*\li	Dynamically load any shared library required by any plug-in
*/
	virtual bool bootstrap_init(string);
/**
*	This is the opposite of 'bootstrap_init()', framework invokes this method once for a 'bootstrap_name'
*	when framework is being terminated. 
*/
	virtual bool bootstrap_terminate(string){return(true);};
    SOCKET get_socket();
    int get_port();
    int get_thread_id();
    int get_tls_flag();
    string get_plugin();
    string get_plugin_name();
    string get_plugin_number();
    string get_plugin_path();
    int set_plugin(string);
    int set_plugin_name(string);
    int set_plugin_number(string);
    int set_port(int);
    int set_tls_flag(int);
    int set_thread_id(int);
    int set_socket(SOCKET);
    int set_plugin_path(string);
    int add_plugin_alias(string);
    int clear_aliases();
    int aliases_count();
    string pop_alias();
    bool find_plugin_alias(string);
/// Binds socket to TCP port, set options on new socket and do socket listen.
    SOCKET initialize_socket(string port);
    int set_client_socket(SOCKET);
    int set_conf_file(string);
    string get_conf_file();
    SOCKET get_client_socket(void);
    generic_plugin &operator=(const generic_plugin &);
    bool operator<(generic_plugin a)
    {
        return this->get_port() < a.get_port();
    }
    bool operator==(generic_plugin a)
    {
        return this->get_port() == a.get_port();
    }
    virtual ~generic_plugin() {};
};
typedef generic_plugin GENERIC_PLUGIN;

#endif 
