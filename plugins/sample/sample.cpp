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
#include <map>
using namespace std;

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <process.h>
#include "XEventLog.h"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#endif
using namespace std;
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "sample.h"
#include "generic_plugin.h"
#include "generic_server.h"

#ifdef DLLAPI
extern "C" __declspec(dllexport) sample_plugin *create_instance(void);
#else
extern "C" sample_plugin *create_instance(void);
#endif

sample_plugin::sample_plugin(void) : generic_plugin()
{
    db_name = "";
    db_passwd = "";
}
/*********************************************************************************************/

sample_plugin::sample_plugin(char *plugin_name,int plugin_port) : generic_plugin(plugin_name,plugin_port)
{
    db_name = "";
    db_passwd = "";
}
/*********************************************************************************************/

#ifdef DLLAPI
extern "C" __declspec(dllexport) sample_plugin *create_instance(void)
#else
extern "C" sample_plugin *create_instance(void)
#endif
{
    return(new(SAMPLE));
}
/*********************************************************************************************/

int sample_plugin::plugin_init(int thread_no)
{
    ostringstream oss;

    try
    {
        oss << " Thr.ID:" << thread_no << " Plugin:" << plugin_name << " Successfully initialized session.";
        log(LOG_HI,oss.str());
        return(1);
    }
    catch(const exception& er)
    {
        oss << " Thr.ID:" << thread_no << " Plugin:" << plugin_name << " " << er.what();
        log(LOG_HI,oss.str());
        return(0);
    }
}
/*********************************************************************************************/

sample_plugin &sample_plugin::operator=(const sample_plugin &rhs)
{
    if(this != &rhs)
    {
        generic_plugin::operator= (rhs);
        db_name = rhs.db_name;
        db_passwd = rhs.db_passwd;
    }
    return(*this);
}
/*********************************************************************************************/

int sample_plugin::shutdown_plugin(void)
{
    ostringstream oss;
    generic_server *fwork = (generic_server *) pinstance;    

    try
    {
//	Do plug-in cleanup here..
        oss << " Thr.ID:" << thread_id << " Plugin:" << plugin_name << " Successfully shutdown session.";
        log(LOG_HI,oss.str());
    }
    catch(const exception& er)
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " " << er.what();
        log(LOG_HI,oss.str());
        return 0;
    }
	return(1);
}
/*********************************************************************************************/

int sample_plugin::server_init(void)
{
    ostringstream oss;

    try
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " Successfully initialized sample server.";
        log(LOG_HI,oss.str());
        return(1);
    }
    catch(const exception& er)
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " " << er.what();
        log(LOG_HI,oss.str());
        return(0);
    }
}
/*********************************************************************************************/

int sample_plugin::server_shutdown(void)
{
    ostringstream oss;

    try
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " Successfully shutdown sample server.";
        log(LOG_HI,oss.str());
        return(1);
    }
    catch(const exception& er)
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " " << er.what();
        log(LOG_HI,oss.str());
        return(0);
    }
}
/*********************************************************************************************/

int sample_plugin::process_request(void *buffer, void *out_buff,unsigned int &size)
{
    generic_server *fwork = (generic_server *) pinstance;
    ostringstream oss;

	memset(out_buff,0,MAX_SZ);
    oss << " Thr.ID:" << thread_id << " Plugin:" << plugin_name << " Received: " << string((char *)buffer,size);
    log(LOG_LOW,oss.str());
	if(!memcmp(buffer,"QUIT",4))
	{
		size = 12;
		memcpy((char *)out_buff,"00 TERMINATE",size);
		return(TERMINATE_SESSION);
	}
	size = 22;
	memcpy((char *)out_buff,"response from plugin..",size);
    return(CONTINUE_SESSION);
}
/**********************************************************************************************/

string sample_plugin::get_plugin_version(void)
{
        return(PLUGIN_VERSION);
}
/**********************************************************************************************/

int sample_plugin::get_plugin_params(string line)
{
    generic_plugin::get_plugin_params(line);
    if (line.size() > 8 && line.compare(0,8,"DB_NAME=") == 0)
    {
        db_name = line.substr(8);
        return(0);
    }
    if (line.size() > 10 && line.compare(0,10,"DB_PASSWD=") == 0)
    {
        db_passwd = line.substr(10);
        return(0);
    }
    return(1);
}
/*********************************************************************************************/

string sample_plugin::bootstrap_name(void)
{
	return(string("INIT_SMART_CARD"));
}
/*****************************************************************************************************/

bool sample_plugin::bootstrap_init(string bootstrap_name)
{
	// Do all bootstrapping stuff here. Load dlls,shared libs, initialize 3rd party apps etc
	// Don't bother about thread sync here, Framework will take care of it!
	return(true);
}
/*****************************************************************************************************/

bool sample_plugin::bootstrap_terminate(string bootstrap_name)
{
    // Do all bootstrapping cleanup stuff here. Unload dlls,shared libs,3rd party apps etc
    // Don't bother about thread sync here, Framework will take care of it!
    return(true);
}
/*****************************************************************************************************/