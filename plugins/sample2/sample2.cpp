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
#include <fstream>
#include <iostream>
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
#include "sample2.h"
#include "generic_plugin.h"
#include "generic_server.h"

#ifdef DLLAPI
extern "C" __declspec(dllexport) sample2_plugin *create_instance(void);
#else
extern "C" sample2_plugin *create_instance(void);
#endif

sample2_plugin::sample2_plugin(void) : generic_plugin()
{
    ftp_dir = "";
}
/*********************************************************************************************/

sample2_plugin::sample2_plugin(char *plugin_name,int plugin_port) : generic_plugin(plugin_name,plugin_port)
{
    ftp_dir = "";
}
/*********************************************************************************************/

#ifdef DLLAPI
extern "C" __declspec(dllexport) sample2_plugin *create_instance(void)
#else
extern "C" sample2_plugin *create_instance(void)
#endif
{
    return(new(SAMPLE2));
}
/*********************************************************************************************/

int sample2_plugin::plugin_init(int thread_no)
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

sample2_plugin &sample2_plugin::operator=(const sample2_plugin &rhs)
{
    if(this != &rhs)
    {
        generic_plugin::operator= (rhs);
        ftp_dir = rhs.ftp_dir;
    }
    return(*this);
}
/*********************************************************************************************/

int sample2_plugin::shutdown_plugin(void)
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

int sample2_plugin::server_init(void)
{
    ostringstream oss;

    try
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " Successfully initialized sample2 server.";
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

int sample2_plugin::server_shutdown(void)
{
    ostringstream oss;

    try
    {
        oss << " Thr.ID: MAIN Plugin:" << plugin_name << " Successfully shutdown sample2 server.";
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

int sample2_plugin::process_request(void *buffer, void *out_buff,unsigned int &size)
{
    generic_server *fwork = (generic_server *) pinstance;
    ostringstream oss;
	unsigned char *req_type;
	// Simple file transfer messaging
	// REQUEST_TYPE     - 1 byte 
	// PLUGIN_DATA		- variable/optional,based on REQUEST_TYPE
	// REQUEST_TYPE:
	// 1 - set file name; plugin will create a file on server on receipt of this request
	// Example: 1file_to_transfer.jpg
	// 2 - file data; plugin will append data to file created earlier
	// Example: 2file_data
	// 3 - close file; plugin will close file created

	req_type = static_cast<unsigned char *>(buffer);
    switch(*req_type)
    {
        case SET_FILENAME:
            fname = ftp_dir + "/" + string((char *)buffer+1,size-1);
			new_file.open(fname.c_str(),ios::binary);
			if(!new_file.rdbuf()->is_open())
			{
				size = 0;
				return(TERMINATE_SESSION);
			}
			oss.clear();
			oss.str("");
		 	oss << " Thr.ID:" << thread_id << " Plugin:" << plugin_name << " Creating new file: " << fname;
    		log(LOG_LOW,oss.str());
			size = 0;
    		return(CONTINUE_SESSION);

        case FILE_DATA:
			new_file << string((char *)buffer+1,size-1);
    		return(CONTINUE_SESSION);

        case CLOSE_FILE:
			new_file.close();
			oss.clear();
			oss.str("");
		 	oss << " Thr.ID:" << thread_id << " Plugin:" << plugin_name << " Closing file: " << fname;
    		log(LOG_LOW,oss.str());
			size = 0;
			return(TERMINATE_SESSION);

        default:
			return(TERMINATE_SESSION);
    }
return(CONTINUE_SESSION);
}
/**********************************************************************************************/

string sample2_plugin::get_plugin_version(void)
{
        return(PLUGIN_VERSION);
}
/**********************************************************************************************/

int sample2_plugin::get_plugin_params(string line)
{
    generic_plugin::get_plugin_params(line);
    if (line.size() > 8 && line.compare(0,8,"FTP_DIR=") == 0)
    {
        ftp_dir = line.substr(8);
        return(0);
    }
    return(1);
}
/*********************************************************************************************/

