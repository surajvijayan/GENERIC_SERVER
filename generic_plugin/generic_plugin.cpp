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
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <dlfcn.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include "generic_plugin.h"
#include "generic_server.h"

/*********************************************************************************************/

ostream& operator<<(ostream& output,  generic_plugin& p)
{
	generic_server *fwork = (generic_server *) p.pinstance;
    vector<string>::iterator itr;
	output.clear();
	output << "(PLUGIN_TYPE: " <<  setw(12) << setfill(' ') << setiosflags(ios::left) <<  p.plugin_type << " ";
	output << " PLUGIN_ID: "   <<  setw(12) << setfill(' ') << setiosflags(ios::left) << p.plugin_name << " ";
	output << " IP: "        <<  setw(12) << p.local_ip_address;
	if(p.tls_enabled)
		output << " TLS_PORT: "  << p.port << ")";
	else
		output << " RAW_PORT: "  << p.port << ")";
    for(itr = p.v_plugin_aliases.begin(); itr != p.v_plugin_aliases.end(); ++itr)
	{
		output << endl;
		output << fwork->get_cur_time();
		output << "(PLUGIN_TYPE: " <<  setw(12) << setfill(' ') << setiosflags(ios::left) <<  p.plugin_type << " ";
		output << " ALIAS PLUGIN_ID: "   <<  setw(12) << setfill(' ') << setiosflags(ios::left) << (*itr) << " ";
		output << " IP: "        <<  setw(12) << p.local_ip_address;
		if(p.tls_enabled)
			output << " TLS_PORT: "  << p.port << ")";
		else
			output << " RAW_PORT: "  << p.port << ")";
	}
    return output; 
}
/*********************************************************************************************/

generic_plugin &generic_plugin::operator=(const generic_plugin &rhs)
{
	if(this != &rhs)
	{
		plugin_type = rhs.plugin_type;
		plugin_number = rhs.plugin_number;
		plugin_name = rhs.plugin_name;
		client_socket = rhs.client_socket;
		server_socket = rhs.server_socket;
		plugin_conf_file = rhs.plugin_conf_file;
		networking = rhs.networking;
		verbose_level = rhs.verbose_level;
		tls_enabled = rhs.tls_enabled;
		thread_id = rhs.thread_id;
		validate_plugin = rhs.validate_plugin;
		port = rhs.port;
		pinstance = rhs.pinstance;
		local_ip_address = rhs.local_ip_address;
		v_plugin_aliases = rhs.v_plugin_aliases;
	}
	return(*this);
}
/*********************************************************************************************/

int generic_plugin::clear_aliases()
{
	this->v_plugin_aliases.clear();
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_plugin(string in_plugin)
{
	this->plugin_type = in_plugin;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_conf_file(string conf_file)
{
        plugin_conf_file = conf_file;
        return(1);
}
/*********************************************************************************************/

int generic_plugin::set_plugin_name(string in_plugin_name)
{
	plugin_name = in_plugin_name;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_plugin_number(string in_plugin_number)
{
	plugin_number = in_plugin_number;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_plugin_path(string plugin_path)
{
    plugin_lib_path = plugin_path;
    return(1);
}
/*********************************************************************************************/

int generic_plugin::set_tls_flag(int in_tls_flag)
{
	tls_enabled = in_tls_flag;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_socket(SOCKET in_socket)
{
	server_socket = in_socket;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_client_socket(SOCKET in_socket)
{
	client_socket = in_socket;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::set_port(int in_port)
{
	port = in_port;
	return(1);
}
/*********************************************************************************************/

int generic_plugin::add_plugin_alias(string plugin_alias)
{
	v_plugin_aliases.push_back(plugin_alias);
	return(1);
}
/*********************************************************************************************/

bool generic_plugin::find_plugin_alias(string alias_name)
{
    vector<string>::iterator itr;
    for(itr = v_plugin_aliases.begin(); itr != v_plugin_aliases.end(); ++itr)
		if((*itr) == alias_name)
			return(true);
    return(false);
}
/*********************************************************************************************/

int generic_plugin::aliases_count()
{
	return(v_plugin_aliases.size());
}
/*********************************************************************************************/

string generic_plugin::pop_alias()
{
	string alias_name;

	alias_name = v_plugin_aliases.front();
	v_plugin_aliases.erase(v_plugin_aliases.begin());
	return(alias_name);
}
/*********************************************************************************************/

int generic_plugin::set_thread_id(int in_thread_id)
{
	thread_id = in_thread_id;
	return(1);
}
/*********************************************************************************************/

string generic_plugin::get_plugin_path()
{
    return(plugin_lib_path);
}
/*********************************************************************************************/

SOCKET generic_plugin::get_socket(void)
{
	return(server_socket);
}
/*********************************************************************************************/

SOCKET generic_plugin::get_client_socket(void)
{
	return(client_socket);
}
/*********************************************************************************************/

string generic_plugin::get_plugin(void)
{
	return(plugin_type);
}
/*********************************************************************************************/

string generic_plugin::get_plugin_name(void)
{
	return(plugin_name);
}
/*********************************************************************************************/

string generic_plugin::get_conf_file(void)
{
        return(plugin_conf_file);
}
/*********************************************************************************************/

string generic_plugin::get_plugin_number(void)
{
	return(plugin_number);
}
/*********************************************************************************************/

int generic_plugin::get_port(void)
{
	return(port);
}
/*********************************************************************************************/

int generic_plugin::get_thread_id(void)
{
	return(thread_id);
}
/*********************************************************************************************/

int generic_plugin::get_tls_flag(void)
{
	return(tls_enabled);
}
/*********************************************************************************************/

bool generic_plugin::get_validate_plugin(void)
{
        return(validate_plugin);
}
/*********************************************************************************************/

generic_plugin::generic_plugin(void)
{
	verbose_level = 2;
	networking = 1;
	tls_enabled = 0;
	local_ip_address = NULL;
	validate_plugin = true;
	plugin_name = "";
	plugin_conf_file = "";
	plugin_number = "";
	plugin_type = "";
	client_socket = 0;
	server_socket = 0;
	tls_enabled = 0;
	pinstance = 0;
	thread_id = 0;
}
/*********************************************************************************************/

generic_plugin::generic_plugin(char *plugin_name,int plugin_port)
{
	port = plugin_port;
	plugin_type = plugin_name;
	networking = 1;
	tls_enabled = 0;
	verbose_level = 2;
	pinstance = 0;
	local_ip_address = NULL;
	validate_plugin = true;
}
/*********************************************************************************************/

generic_plugin::generic_plugin(char *plugin_name,char *plugin_id,int plugin_port)
{
	port = plugin_port;
	plugin_type = plugin_name;
	plugin_name = plugin_id;
	networking = 1;
	tls_enabled = 0;
	verbose_level = 2;
	local_ip_address = NULL;
	validate_plugin = true;
}
/*********************************************************************************************/

SOCKET generic_plugin::initialize_socket(string port)
{
	SOCKET ListenSocket, on;
	struct addrinfo *result = NULL;
	struct sockaddr_in local_address;
	struct sockaddr_in *sa;
	struct addrinfo hints;
	ostringstream os;
	int iResult;
	generic_server *fwork = (generic_server *) pinstance;

	memset(&local_address,0,sizeof(local_address));
    memset(&hints, 0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (iResult != 0)
    {
        os.str("");
        os.clear();
        os << fwork->get_cur_time() << " Thr.ID:MAIN getaddrinfo failed.";
        fwork->log(LOG_LOW,verbose_level,os.str());
	}
    // Create a SOCKET for one device/plugin
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        os.str("");
        os.clear();
        os << fwork->get_cur_time() << " Thr.ID:MAIN socket creation failed.";
        fwork->log(LOG_LOW,verbose_level,os.str());
        freeaddrinfo(result);
        return 0;
    }
	on = 1;
#ifdef WINDOWS
	setsockopt(ListenSocket,SOL_SOCKET, SO_EXCLUSIVEADDRUSE,(char *) &on, sizeof (on));
#endif
	setsockopt(ListenSocket,SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if(!networking)
	{
		local_address.sin_family = AF_INET;
		local_address.sin_addr.s_addr = inet_addr("127.0.0.1");
		local_address.sin_port = htons(atoi(port.c_str()));
    	iResult = bind(ListenSocket, (sockaddr *) &local_address, (int)result->ai_addrlen);
		sa = (struct sockaddr_in *) result->ai_addr;
		local_ip_address = inet_ntoa(local_address.sin_addr);
	}
	else
	{
		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		sa = (struct sockaddr_in *) result->ai_addr;
		local_ip_address = inet_ntoa(sa->sin_addr);
	}
    if (iResult == SOCKET_ERROR)
    {
        os.str("");
        os.clear();
        os << fwork->get_cur_time() << " Thr.ID:MAIN socket bind failed for port: " << port;
        log(LOG_LOW,os.str());
        freeaddrinfo(result);
        shutdown(ListenSocket,2);
#ifdef WINDOWS
		closesocket(ListenSocket);
#else
		close(ListenSocket);
#endif
        return 0;
    }
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        os.str("");
        os.clear();
        os << fwork->get_cur_time() << " Thr.ID:MAIN socket listen failed for port: " << port;
        log(LOG_LOW,os.str());
        shutdown(ListenSocket,2);
#ifdef WINDOWS
		closesocket(ListenSocket);
#else
		close(ListenSocket);
#endif
        return 0;
    }
	fwork->setblocking(ListenSocket);
    freeaddrinfo(result);
	return(ListenSocket);
}
/*****************************************************************************************************/

string generic_plugin::generic_plugin_version()
{
    return("0_0_0_9");
}
/*****************************************************************************************************/

int generic_plugin::get_plugin_params(string line)
{
    generic_server *fwork = (generic_server *) pinstance;

    if(line.size() > 11 && line.compare(0,11,"NETWORKING=") == 0)
    {
        networking = atoi(line.substr(11).c_str());
        return(0);
    }
    if(line.size() > 14 && line.compare(0,14,"VERBOSE_LEVEL=") == 0)
    {
        verbose_level = atoi(line.substr(14).c_str());
        return(0);
    }
	if (line.size() > 16 && line.compare(0,16,"VALIDATE_PLUGIN=") == 0)
    {
		if(atoi(line.substr(16).c_str()) )
			validate_plugin = true;
		else
			validate_plugin = false;
        return(0);
    }
	return(1);
}
/*****************************************************************************************************/

GEN_PLUGIN_MUTEX generic_plugin::create_mutex(void)
{
	GEN_PLUGIN_MUTEX mutex;
#ifdef WINDOWS
	mutex = CreateMutex( NULL, FALSE, NULL);
#else
	pthread_mutex_init(&mutex,NULL);
#endif
	return(mutex);
}
/*****************************************************************************************************/

int generic_plugin::lock_mutex(GEN_PLUGIN_MUTEX *mutex)
{// not using pthread_cond variable, since Windows does not support cond. variables
#ifdef WINDOWS
DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(*mutex,INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
    return(1);
}
/*****************************************************************************************************/

int  generic_plugin::rel_mutex(GEN_PLUGIN_MUTEX *mutex)
{
#ifdef WINDOWS
    ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
    return(1);
}
/*****************************************************************************************************/

int   generic_plugin::log(unsigned int debug_level,string data)
{
	generic_server *fwork = (generic_server *) pinstance;

	return(fwork->log(debug_level,verbose_level,data));
}
/*****************************************************************************************************/

int   generic_plugin::log(unsigned int debug_level,char *data)
{
	generic_server *fwork = (generic_server *) pinstance;

	return(fwork->log(debug_level,verbose_level,(char *) data));
}
/*****************************************************************************************************/

string generic_plugin::bootstrap_name(void)
{
	return("");
}
/*****************************************************************************************************/

bool generic_plugin::bootstrap_init(string str)
{
	return(true);
}
/*****************************************************************************************************/
