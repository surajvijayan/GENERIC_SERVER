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
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <utility>
using namespace std;

#ifdef WINDOWS
#include <winsock2.h>
#include <process.h>
#include <ws2tcpip.h>
#include <ctype.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <syslog.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "sslserver.h"
#include "generic_plugin.h"
#include "generic_server.h"

generic_server *generic_server::pinstance = 0;
generic_server::generic_server(void)
{
	pinstance = 0;
	command_port = 10000;
	plugins_path = "";
	max_threads = 100;
	timeout_seconds = 0;
	logname = "";
	server_cert = "";
	ca_certificate = "";
	verify_client = false;
	rsa_private_key = "";
	max_log_size = 10*1000*1000;
	max_logs_saved = 5;
	syslog_flag = false;
#ifdef WINDOWS
	log_file_mutex = CreateMutex( NULL, FALSE, NULL);
	thread_cnt_mutex = CreateMutex( NULL, FALSE, NULL);
	thread_sync_mutex = CreateMutex( NULL, FALSE, NULL);
#else
	pthread_mutex_init(&log_file_mutex,NULL);
	pthread_mutex_init(&thread_cnt_mutex,NULL);
	pthread_mutex_init(&thread_sync_mutex,NULL);
#endif
}
/*********************************************************************************************/

generic_server *generic_server::instance(void)
{
    return pinstance ? pinstance : (pinstance = new generic_server);
}
/*********************************************************************************************/

int  generic_server::log(unsigned int debug_level,unsigned int verbose_level,char *data) 
{

	if(syslog_flag)
		return(syslog_msg(debug_level,verbose_level,data));
	unsigned int msk = 0xFFFFFFFF;
	msk = msk << 4;
	get_log_file_mutex();
	debug_level = debug_level & ~msk; 
	if(verbose_level >= debug_level)
	{
		if(log_file.is_open())
			log_file << get_cur_time() << data << endl;
		else
			cout << get_cur_time() << data << endl;
	}	
	rel_log_file_mutex();
	return(1);
}
/*********************************************************************************************/

int  generic_server::log(unsigned int debug_level,unsigned int verbose_level,string data) 
{
	if(syslog_flag)
        return(syslog_msg(debug_level,verbose_level,data));
	unsigned int msk = 0xFFFFFFFF;

	msk = msk << 4;
	get_log_file_mutex();
	debug_level = debug_level & ~msk; 
	if(verbose_level >= debug_level)
	{
		if(log_file.is_open())
			log_file << get_cur_time() << data << endl;
		else
			cout << get_cur_time() <<  data << endl;
	}	
	rel_log_file_mutex();
	return(1);
}
/*********************************************************************************************/

int  generic_server::get_log_file_mutex(void)
{// not using pthread_cond variable, since Windows does not support cond. variables
#ifdef WINDOWS
DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(log_file_mutex,INFINITE);
#else
	pthread_mutex_lock(&log_file_mutex);
#endif
    return(1);
}
/*********************************************************************************************/

int generic_server::rollover_logs(void)
{
	int i;
	unsigned long size;
	string file_name,fname2;
	struct _stat stFileInfo;
	size = get_log_size();
	map<int,string> log_files;
	map<int,string>::iterator itr;
	ostringstream oss;
	if(size > max_log_size)
	{	
		log(LOG_LOW,LOG_LOW,oss.str());
		get_log_file_mutex();
		if(log_file.is_open())
			log_file.close();
	    for(i = 0;i <= max_logs_saved; i++)
        {
			stringstream oss;
            if(i > 0)
				oss << logname  << "." << i;
            else
				oss << logname;
            file_name = oss.str();
            if(!_stat(file_name.c_str(),&stFileInfo))
                log_files[i] = file_name;
        }
		if(log_files.size() == max_logs_saved+1)
		{
			itr = log_files.end();
			itr--;
			log_files.erase(itr);
		}
		if(log_files.size() == 1)
		{
			fname2 = logname + ".1";
			rename(log_files[0].c_str(),fname2.c_str());
		}
		else
		{	
        	for(i = log_files.size()-1; i >= 0;i--)
			{
				stringstream oss;
				oss << logname  << "." << (i+1);
            	fname2 = oss.str();
				rename(log_files[i].c_str(),fname2.c_str());
			}
		}
		log_file.open(logname.c_str());
		rel_log_file_mutex();
	}
	return(1);
}
/*********************************************************************************************/

int  generic_server::rel_log_file_mutex(void)
{
#ifdef WINDOWS
    ReleaseMutex(log_file_mutex);
#else
	pthread_mutex_unlock(&log_file_mutex);
#endif
    return(1);
}
/*********************************************************************************************/

int  generic_server::set_conf_file(char *cfile)
{
	conf_file = cfile;
    return(1);
}
/*********************************************************************************************/

int  generic_server::set_process_name(char *pname)
{
	process_name = pname;
    return(1);
}
/*********************************************************************************************/

int  generic_server::get_thread_sync_mutex(void)
{
#ifdef WINDOWS
DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(thread_sync_mutex,INFINITE);
#else
	pthread_mutex_lock(&thread_sync_mutex);
#endif
	return(1);    
}
/*********************************************************************************************/

int  generic_server::rel_thread_sync_mutex(void)
{
#ifdef WINDOWS
    ReleaseMutex(thread_sync_mutex);
#else
	pthread_mutex_unlock(&thread_sync_mutex);
#endif
    return(1);
}
/*********************************************************************************************/

int  generic_server::get_command_port(void)
{
    return(command_port);
}
/*********************************************************************************************/

bool  generic_server::get_verify_client(void)
{
    return(verify_client);
}
/*********************************************************************************************/

int  generic_server::check_allocated_port(string in_plugin_id,string in_plugin_port)
{
	vector <GENERIC_PLUGIN *>::iterator it;

	for(it = get_start_iterator(); it != get_end_iterator();it++)
	{
		if((*it)->get_plugin_name() == in_plugin_id && (*it)->get_port() == atoi(in_plugin_port.c_str()))
			return(1);
	}
	return(0);
}
/*********************************************************************************************/
 
int  generic_server::check_enabled_plugin(string in_plugin_type)
{
	int cnt;
	vector <GENERIC_PLUGIN *>::iterator it;

	for(cnt=0,it = get_start_iterator(); it != get_end_iterator();it++)
	{
		if((*it)->get_plugin() == in_plugin_type)
			cnt++;
	}
	return(cnt);
}
/*********************************************************************************************/

bool generic_server::check_plugin_aliases(vector<string> &plugin_parms)
{
	ostringstream oss;
    vector <GENERIC_PLUGIN *>::iterator it;

    for(it = get_start_iterator(); it != get_end_iterator();it++)
    {
        if( (*it)->get_plugin_name()    != plugin_parms.at(PLUGIN_NAME)       &&
            (*it)->get_plugin()      == plugin_parms.at(PLUGIN_TYPE)          &&
            (*it)->get_plugin_path() == plugin_parms.at(SHARED_LIB_PATH)      &&
		    (*it)->get_port()        == atoi(plugin_parms.at(PORT).c_str())   && 
		    (*it)->get_conf_file()   == plugin_parms.at(PLUGIN_CONF)          && 
		    (*it)->get_tls_flag()    == atoi(plugin_parms.at(TLS_FLAG).c_str()) 
		)
		{
			if(!(*it)->find_plugin_alias(plugin_parms.at(PLUGIN_NAME)))
					(*it)->add_plugin_alias(plugin_parms.at(PLUGIN_NAME));
            return(true);
		}
		else
        if( (*it)->get_plugin_name()    != plugin_parms.at(PLUGIN_NAME)       &&
            (*it)->get_plugin()      == plugin_parms.at(PLUGIN_TYPE)          &&
		    (*it)->get_port()        == atoi(plugin_parms.at(PORT).c_str())   && 
		    (*it)->get_tls_flag()    == atoi(plugin_parms.at(TLS_FLAG).c_str()) 
		)
		{
			if( (*it)->get_conf_file() != plugin_parms.at(PLUGIN_CONF) ||
			    (*it)->get_plugin_path() != plugin_parms.at(SHARED_LIB_PATH) )
			{ 
				oss << " Ignoring attempted plugin alias for: " << (*it)->get_plugin_name() << ". check alias configuration.";
				log(LOG_LOW,LOG_LOW,oss.str());
            	return(true);
			}
		}
    }
    return(false);
}
/*********************************************************************************************/

int generic_server::remove_devices(map <string,int> &new_ports)
{
	vector <GENERIC_PLUGIN *>::iterator it;
	map <string,int>::iterator ports_itr;
	int pos;
	string new_pluginid;

	for(pos=0,it = get_start_iterator(); it != get_end_iterator();it++,pos++)
	{
		ports_itr = new_ports.find((*it)->get_plugin_name());
		if(new_ports.empty() || ports_itr == new_ports.end()) // a device / port got removed from conf_file
		{
			if((*it)->aliases_count())				 // Removed device in CONF has some alias devices
			{										 // We'll now make first alias device as the 'new'
				new_pluginid = (*it)->pop_alias();   // primary device
				(*it)->set_plugin_name(new_pluginid);
			}
			else									 // Removed device in CONF does not have any alias devices
			{										 // remove it from global vector
				shutdown((*it)->get_socket(),2);
#ifdef WINDOWS
				closesocket((*it)->get_socket());
#else
				close((*it)->get_socket());
#endif
				if( (check_enabled_plugin((*it)->get_plugin())) == 1)
					(*it)->server_shutdown();
				unload_shared_lib((*it)->get_plugin());
				v.erase(get_start_iterator()+pos);  
				pos = 0;
				it = get_start_iterator();
				continue;
			}
		}
	}
	return(1);
}
/*********************************************************************************************/

int  generic_server::remove_all_aliases()
{
    vector <GENERIC_PLUGIN *>::iterator it;

    for(it = get_start_iterator(); it != get_end_iterator();it++)
		(*it)->clear_aliases();
    return(1);
}
/*********************************************************************************************/

int generic_server::get_server_params(string line,int start_type)
{
	ostringstream os;

	if(line.size() > 9 && line.compare(0,9,"LOG_FILE=") == 0)
    {
        logname = line.substr(9);
		if(logname.compare(0,6,"SYSLOG") != 0)
#ifdef LINUX
		{
			syslog_flag = false;
           	log_file.open(logname.c_str(),ios_base::app);
		}
		else
		{
			syslog_flag = true;
			openlog(process_name.c_str(),LOG_NDELAY,LOG_LOCAL0);
		}
#else
		{
			syslog_flag = false;
			log_file.open(logname.c_str(),ios_base::app);
		}
		else
		{
			syslog_flag = true;
			el.Init(trim(process_name).c_str());
		}
#endif
		if(start_type == INIT)
		{
  			os  << " Starting GENERIC_SERVER as: " << trim(process_name) << " with conf file: " << trim(conf_file);
	    	log(LOG_LOW,LOG_LOW,os.str());
		}
        return(0);
    }
	if(line.size() >  13 && line.compare(0,13,"COMMAND_PORT=") == 0)
    {
        command_port = atoi(line.substr(13).c_str());
        return(0);
    }
	if(line.size() > 19 && line.compare(0,19,"MAX_LOG_SIZE_IN_MB=") == 0)
    {
		max_log_size = atoi(line.substr(19).c_str());
		max_log_size = max_log_size*1000*1000;
        return(0);
    }
	if(line.size() > 15 && line.compare(0,15,"MAX_LOGS_SAVED=") == 0)
    {
		max_logs_saved = atoi(line.substr(15).c_str());
        return(0);
    }
    if (line.size() > 16 && line.compare(0,16,"TIMEOUT_SECONDS=") == 0)
    {
		timeout_seconds = atoi(line.substr(16).c_str());
        return(0);
    }
    if (line.size() >  12 && line.compare(0,12,"MAX_THREADS=") == 0)
    {
		max_threads = atoi(line.substr(12).c_str());
        return(0);
    }
    if (line.size() > 19 && line.compare(0,19,"SERVER_CERTIFICATE=") == 0)
    {
		server_cert = line.substr(19);
        return(0);
    }
    if (line.size() >  16 && line.compare(0,16,"RSA_PRIVATE_KEY=") == 0)
    {
		rsa_private_key = line.substr(16);
        return(0);
    }
    if (line.size() >  15 && line.compare(0,15,"CA_CERTIFICATE=") == 0)
    {
		ca_certificate = line.substr(15);
        return(0);
    }
    if (line.size() > 14 && line.compare(0,14,"VERIFY_CLIENT=") == 0)
    {
		if(atoi(line.substr(12).c_str()))
			verify_client = true;
		else
			verify_client = false;
        return(0);
    }
    if (line.size() >  13 && line.compare(0,13,"PLUGINS_PATH=") == 0)
    {
		plugins_path = line.substr(13);
        return(0);
    }
	return(1);
}
/*********************************************************************************************/

int  generic_server::init() 
{
	int ret;
	GENERIC_PLUGIN *plugin;
	vector<string> plugin_parms;
	string line,delim,library_image;
	ostringstream os;
	
	ret = 0;
#ifdef WINDOWS
	if (!sock_init())
	{
		cout << "Can't init socket library! " << endl;
        return(0);
	}
	TCHAR filebuf[MAX_SZ];
	get_exe_directory(filebuf,MAX_SZ);
	SetCurrentDirectory(filebuf);
#endif
	tls_setup_reqd = 0;
	delim = "|";
	ifstream inf(conf_file.c_str());

    if(!inf)
    {
        cout << " Can't Open file: " << conf_file << endl;
        return(0);
    }   
    while(getline(inf, line))
    {
		line = trim(line);
        if(line.length() == 0 || line.at(0) == '#')
            continue;
		if(!get_server_params(line,INIT))
			continue;
		plugin_parms = split(line,delim,true);
		if(plugin_parms.size() != 7)
			continue;
		set_plugins_path(plugin_parms);
		// returns true if PLUGIN_NAME is a new alias or an existing alias
		// generic_plugin will be updated in case of new alias
		if(check_plugin_aliases(plugin_parms))
			continue;
		plugin = load_shared_lib(plugin_parms.at(SHARED_LIB_PATH),plugin_parms.at(PLUGIN_TYPE));
		if(plugin == NULL)
		{
			os  << " Thr.ID:MAIN load_library failed: " << plugin_parms.at(SHARED_LIB_PATH) << " PLUGIN_TYPE: " << plugin_parms.at(PLUGIN_TYPE) << endl;
			log(LOG_LOW,LOG_LOW,os.str());
        	return 0;
		}
		plugin = initialize_plugin_object(plugin_parms,plugin);
		if(plugin == NULL)
        	return(0);
		v.push_back(plugin);
    }
	inf.close();
	if(!tls_initialize())
		return(0);
return(1);
}
/*****************************************************************************************/

int generic_server::tls_initialize()
{
	ostringstream os;
    if(rsa_private_key != "" && server_cert != ""  && ca_certificate !="")
    {
        if(!tls_setup())
        {
            os  << " Thr.ID:MAIN TLS_init failed.";
            log(LOG_LOW,LOG_LOW,os.str());
            return(0);
        }
    }
    else if(tls_setup_reqd)
    {
        os  << " Thr.ID:MAIN TLS not correctly setup in CONF file. Please check.";
        log(LOG_LOW,LOG_LOW,os.str());
        return(0);
    }
return(1);
}
/*****************************************************************************************/

int generic_server::tls_setup()
{
	ostringstream os;

	sslserver.set_cert_file(server_cert);
	sslserver.set_priv_key(rsa_private_key);
	sslserver.set_ca_cert(ca_certificate);
	sslserver.set_verify_client(verify_client);
	if(!sslserver.tls_init())
	{
        os  << " Thr.ID:MAIN Unable to initiliaze TLS.";
		log(LOG_LOW,LOG_LOW,(char *)os.str().c_str());
		return(0);
	}
	if(!sslserver.CreateCTX())
	{
        os  << " Thr.ID:MAIN Unable to set TLS context.";
		log(LOG_LOW,LOG_LOW,(char *)os.str().c_str());
		return(0);
	}
	if(!sslserver.LoadCerts())
	{
        os  << " Thr.ID:MAIN Unable to load TLS certificates.";
		log(LOG_LOW,LOG_LOW,(char *)os.str().c_str());
		return(0);
	}
    os  << " Thr.ID:MAIN successfully initialized TLS.";
	log(LOG_LOW,LOG_LOW,(char *)os.str().c_str());
	return(1);
}
/*****************************************************************************************/

int generic_server::unload_shared_lib(string plugin_type)
{
#ifdef WINDOWS
	FreeLibrary((HMODULE)dll_handles[plugin_type]);
#else
	dlclose(dll_handles[plugin_type]);
#endif
	return(1);
}
/*****************************************************************************************/

GENERIC_PLUGIN *generic_server::load_shared_lib(string library_image,string plugin_type)
{
	GENERIC_PLUGIN *plugin_p;
	ostringstream os;

#ifndef WINDOWS
	void* handle = dlopen(library_image.c_str(),RTLD_LAZY);
	if(handle == NULL)
	{
        os  << " Thr.ID:MAIN Can't load DLL: " << " " <<  dlerror();
        log(LOG_LOW,LOG_LOW,os.str());
		return(NULL);
	}
	factory[plugin_type] = (create_fp) dlsym(handle, "create_instance");
	dll_handles[plugin_type] =  (void *) handle;
#else
	HMODULE handle;
	handle = LoadLibrary(library_image.c_str());
	if(handle == NULL)
	{
		os.str("");
        os.clear();
        os  << " Thr.ID:MAIN Error loading: " << library_image << " " << GetLastError();
        log(LOG_LOW,LOG_LOW,os.str());
		return(NULL);
	}
	factory[plugin_type] = (create_fp) GetProcAddress(handle, "create_instance");
	dll_handles[plugin_type] = (void *) handle;
#endif
	if(factory[plugin_type] == NULL)
	{
        os  << " Thr.ID:MAIN Can't instantiate PLUGIN_TYPE: " << plugin_type;
        log(LOG_LOW,LOG_LOW,os.str());
		return(NULL);
	}
	plugin_p = (factory[plugin_type])();
    os  << " Thr.ID:MAIN loaded plugin version: " << plugin_p->get_plugin_version() << " for PLUGIN_TYPE: " << plugin_type;
    log(LOG_LOW,LOG_LOW,os.str());
	return(plugin_p);
}
/*****************************************************************************************/

#ifdef WINDOWS
int  generic_server::sock_init(void)
{
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
		return(0);
	return(1);
}
#endif
/*****************************************************************************************/

int generic_server::socket_close(SOCKET sock)
{
int iResult;
    // shutdown the connection since we're done
    iResult = shutdown(sock, 2);
#ifdef WINDOWS
	closesocket(sock);
#else
	close(sock);
#endif
    if (iResult == SOCKET_ERROR) 
	{
#ifdef WINDOWS
        //WSACleanup();
#endif
        return 0;
    }
    return 1;
}
/*****************************************************************************************/

int generic_server::plugin_read_conf_file(GENERIC_PLUGIN *plugin,int thread_no)
{
    ostringstream os; 
    string line,delim,conf_file;
    conf_file = plugin->plugin_conf_file;

    delim = "|";
    ifstream inf(conf_file.c_str());
    if(!inf.rdbuf()->is_open())
    {   
        os  << " Thr.ID:" << thread_no << " Plugin:" << plugin->get_plugin_name() << " 11 cannot read: " << conf_file;
        log(LOG_LOW,LOG_LOW,os.str());
        return(0);
    }
    while(getline(inf, line))
    {
        line = trim(line);
        if(line.length() == 0 || line.at(0) == '#')
            continue;
        if(!plugin->get_plugin_params(line))
            continue;
    }   
    inf.close();
return(1);
}   
/*****************************************************************************************/

string generic_server::trim(const std::string& s)
{
  if(s.length() == 0)
    return s;
  size_t b = s.find_first_not_of(" \t\r\n");
  size_t e = s.find_last_not_of(" \t\r\n");
  if(b == -1) // No non-spaces
    return "";
  return string(s, b, e - b + 1);
}
/*****************************************************************************************/

string generic_server::get_cur_time(void)
{
time_t tim;
struct tm now;
ostringstream os;

    tim=time(NULL);
#ifdef LINUX
    localtime_r(&tim,&now);
#else
    localtime_s(&now,&tim);
#endif
    os << setw(2) << setfill('0') << now.tm_mon+1 << "/" ;
    os << setw(2) << setfill('0') << now.tm_mday  << "/";
    os << setw(2) << setfill('0') << now.tm_year+1900 << " ";
    os << setw(2) << setfill('0') << now.tm_hour << ":";
    os << setw(2) << setfill('0') << now.tm_min  << ":";
    os << setw(2) << setfill('0') << now.tm_sec;
    return(os.str());
}
/*****************************************************************************************/

int generic_server::get_thread_id()
{
	THREAD_INFO th_info;
	int new_thread_id,thread_cnt;
#ifdef WINDOWS
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(thread_cnt_mutex,INFINITE);
#else
	pthread_mutex_lock(&thread_cnt_mutex);
#endif
	map<int,THREAD_INFO>::iterator it;

    for(thread_cnt = 0,it = thread_ids.begin(); it != thread_ids.end();it++,thread_cnt++)
	{
		th_info = (THREAD_INFO) (*it).second;
        if(th_info.state == THREAD_FREE)
		{
			th_info =  (THREAD_INFO) (*it).second;
			th_info.state = THREAD_ALLOCATED;
			thread_ids[thread_cnt] = th_info;
			new_thread_id = (*it).first;
			break;
		}
	}
	if(it == thread_ids.end())
	{
		th_info.state = THREAD_ALLOCATED;
		thread_ids[thread_cnt] = th_info;
		new_thread_id = thread_cnt;
	}
#ifdef WINDOWS
    ReleaseMutex(thread_cnt_mutex);
#else
	pthread_mutex_unlock(&thread_cnt_mutex);
#endif
    return(new_thread_id);
}
/*********************************************************************************************/

int generic_server::rel_thread_id(int thread_id)
{
    int status;
#ifdef WINDOWS
    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(thread_cnt_mutex,INFINITE);
#else
    pthread_mutex_lock(&thread_cnt_mutex);
#endif

	if(thread_ids.find(thread_id) != thread_ids.end())
	{
		thread_ids[thread_id].state = THREAD_COMPLETED;
		status = 1;
	}
	else
		status = 0;
#ifdef WINDOWS
    ReleaseMutex(thread_cnt_mutex);
#else
    pthread_mutex_unlock(&thread_cnt_mutex);
#endif
    return(status);
}
/*********************************************************************************************/

int generic_server::close_thread_ids(void)
{
    THREAD_INFO th_info;
    int thread_cnt;
#ifdef WINDOWS
    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(thread_cnt_mutex,INFINITE);
#else
    pthread_mutex_lock(&thread_cnt_mutex);
#endif
    map<int,THREAD_INFO>::iterator it;

    for(thread_cnt = 0,it = thread_ids.begin(); it != thread_ids.end();it++,thread_cnt++)
    {
        th_info = (THREAD_INFO) (*it).second;
        if(thread_ids[thread_cnt].state == THREAD_COMPLETED)
        {
#ifdef WINDOWS
            CloseHandle(th_info.th);
#endif
            thread_ids[thread_cnt].state = THREAD_FREE;
        }
    }
#ifdef WINDOWS
    ReleaseMutex(thread_cnt_mutex);
#else
    pthread_mutex_unlock(&thread_cnt_mutex);
#endif
    return(1);
}
/*****************************************************************************************/

vector <GENERIC_PLUGIN *>::iterator generic_server::get_start_iterator(void)
{
    vector <GENERIC_PLUGIN *>::iterator it = v.begin();
    return(it);
}
/*********************************************************************************************/

vector <GENERIC_PLUGIN *>::iterator generic_server::get_end_iterator(void)
{
	vector <GENERIC_PLUGIN *>::iterator it = v.end();
	return(it);
}
/*********************************************************************************************/

int generic_server::print_key_message()
{
	vector <GENERIC_PLUGIN *>::iterator it;
	GENERIC_PLUGIN tmp_plugin;
	ostringstream os;

	log(LOG_LOW,LOG_LOW,(char *)" Thr.ID:MAIN thread. Doing PLUGIN_WALK: ");
    for(it = get_start_iterator(); it != get_end_iterator();it++)
	{
		os.str("");
		os.clear();
		os << *(*it);
        log(LOG_LOW,LOG_LOW,os.str());
	}
	os.str("");
	os.clear();
    os  << " RSA: " << rsa_private_key;
    os  << " CERT: " << server_cert;
    os  << " CA-CERT: " << ca_certificate;
	log(LOG_LOW,LOG_LOW,os.str());
	os.str("");
	os.clear();
    os  << " GENERIC_SERVER framework version: " << get_framework_version() << " using GEN_PLUGIN version: ";
	os  << tmp_plugin.generic_plugin_version() << " ready.";
    log(LOG_LOW,LOG_LOW,os.str());

	return(1);
}
/*********************************************************************************************/

int generic_server::set_access_control(char daemon_flag)
{
	ostringstream os;
#ifdef LINUX
    if(getuid() != 0 && !daemon_flag)
    {
        os  << " Thr.ID:MAIN thread. Only 'root'' user can start generic_server.."; 
        log(LOG_LOW,LOG_LOW,os.str());
        cout  << " Thr.ID:MAIN thread. Only 'root' user can start generic_server.." << endl;
		return(0);
    }
#endif
	return(1);
}
/*********************************************************************************************/

int generic_server::swap_bytes(unsigned char *key)
{
	unsigned char buff[25];
	memcpy(buff,key,16);
	memcpy(key,buff+8,8);
	memcpy(key+8,buff,8);
return(1);
}
/*********************************************************************************************/

int generic_server::bb_getline(char *sav_buffer,size_t length,size_t conn_s)
{
	fd_set socks;
	struct timeval timeout;
	char str[MAX_SZ];
	size_t readsock,t;
	size_t bytes_recvd;

    memset(sav_buffer,0,sizeof(sav_buffer));
    memset(str,0,MAX_SZ);
    bytes_recvd = 0;

    while(1)
    {
       FD_ZERO(&socks);
       FD_SET(conn_s,&socks);
       timeout.tv_sec = 0;
       timeout.tv_usec = 50000;    
       readsock = select((int)conn_s+1,&socks,NULL,NULL,&timeout);
       if(bytes_recvd && readsock <= 0)
             return(bytes_recvd);
       if(FD_ISSET(conn_s,&socks))
       {
           if ((t = recv(conn_s,str,sizeof(str), 0)) > 0)
           {
				memcpy(sav_buffer+bytes_recvd,str,t);
               	bytes_recvd += t;
                if(bytes_recvd >= length)
           	    	return(bytes_recvd);
           }
           else
		   {
               return(bytes_recvd);
		   }
       }
    }
}
/*******************************************************************************************/

int generic_server::bb_getline(char *sav_buffer,size_t length,size_t conn_s,int secs)
{
    fd_set socks;
    struct timeval timeout;
    char str[MAX_SZ];
    size_t readsock,t;
	size_t bytes_recvd;
    bytes_recvd = 0;

    memset(sav_buffer,0,sizeof(sav_buffer));
    memset(str,0,MAX_SZ);

    while(1)
    {
       FD_ZERO(&socks);
       FD_SET(conn_s,&socks);
       timeout.tv_sec = secs;
       timeout.tv_usec = 0;
       readsock = select((int)conn_s+1,&socks,NULL,NULL,&timeout);
       if(readsock <= 0)   
             return(bytes_recvd);
       if(FD_ISSET(conn_s,&socks))
       {
           if ((t = recv(conn_s,str,sizeof(str), 0)) > 0)
           {
                memcpy(sav_buffer,str,t);
               	bytes_recvd += t;
                if(bytes_recvd >= length)
           	    	return(bytes_recvd);
           }
           else
             return(bytes_recvd);
       }
    }
}
/******************************************************************************************/

int generic_server::get_status(unsigned char *buff)
{
        return(((buff[0]-48)*10)+(buff[1]-48));
}
/******************************************************************************************/

int generic_server::close_log(void)
{
		if(log_file.is_open())
			log_file.close();
	return(1);
}
/******************************************************************************************/

int generic_server::get_log_size()
{
#ifdef WINDOWS
	struct _stat fileStat; 
#else
	struct stat fileStat;
#endif
	int err;
	
#ifdef WINDOWS
	err = _stat(logname.c_str(),&fileStat); 
#else
	err = stat(logname.c_str(),&fileStat);
#endif
	if (err != 0) 
		return 0; 
	return fileStat.st_size; 
}
/*****************************************************************************************************/

vector<string> generic_server::split(const string& s, const string& delim, const bool keep_empty = true) 
{
    vector<string> result;
    if (delim.empty()) 
	{
        result.push_back(s);
        return result;
    }
    string::const_iterator substart = s.begin(), subend;
    while (true) 
	{
        subend = search(substart, s.end(), delim.begin(), delim.end());
        string temp(substart, subend);
        if (keep_empty || !temp.empty()) 
            result.push_back(temp);
        if (subend == s.end()) 
            break;
        substart = subend + delim.size();
    }
    return result;
}
/*****************************************************************************************************/
#ifdef WINDOWS
BOOL generic_server::get_exe_directory(TCHAR *obuf, size_t osize)  
 {  
     if(!GetModuleFileName(0, obuf, osize))  
     {  
         *obuf = '\0';// insure it's NUL terminated  
         return FALSE;  
     }      
     // run through looking for the *last* slash in this path.  
     // if we find it, NUL it out to truncate the following  
     // filename part.  
     TCHAR*lastslash = 0;  
     for(; *obuf; obuf++)  
     {  
         if (*obuf == '\\' || *obuf == '/')  
             lastslash = obuf;  
     }  
     if(lastslash) 
		*lastslash = '\0';  
     return TRUE;  
}  
/*****************************************************************************************************/
#endif

int  generic_server::reload_conf_file(void) 
{
	GENERIC_PLUGIN *plugin;
	vector <string> plugin_parms;
	map <string,int> new_conf_ports;
	string line,delim,library_image;
	ostringstream os;
	vector <GENERIC_PLUGIN *>::iterator it;

	log(LOG_LOW,LOG_LOW,(char *)" Thr.ID:MAIN thread. SIGNAL received to re-read conf file..");
	delim = "|";
	ifstream inf(conf_file.c_str(),ifstream::in);
    if(!inf)
        return(0);
	close_log();
	remove_all_aliases();
    while(getline(inf, line))
    {
		line = trim(line);
        if(line.length() == 0 || line.at(0) == '#')
            continue;
		if(!get_server_params(line,RELOAD))
			continue;
		if(command_port == 0)
		{
			os << " Thr.ID:MAIN thread. COMMAND_PORT not defined in CONF file." << endl;
			log(LOG_LOW,LOG_LOW,os.str());
			command_port = 10000;
			return(0);
		}
		plugin_parms = split(line,delim,true);
		set_plugins_path(plugin_parms);
		new_conf_ports[plugin_parms.at(PLUGIN_NAME)] = atoi(plugin_parms.at(PORT).c_str());
		if(check_allocated_port(plugin_parms.at(PLUGIN_NAME),plugin_parms.at(PORT)))
			continue;
        // returns true if PLUGIN_NAME is a new alias or an existing alias
        // generic_plugin will be updated in case of new alias
        if(check_plugin_aliases(plugin_parms))
            continue;
		plugin = load_shared_lib(plugin_parms.at(SHARED_LIB_PATH),plugin_parms.at(PLUGIN_TYPE));
		if(plugin == NULL)
		{
			os.str("");
			os.clear();
			os  << " Thr.ID:MAIN thread. load_library failed for: " << plugin_parms.at(SHARED_LIB_PATH) << endl;
			log(LOG_LOW,LOG_LOW,os.str());
       		return 0;
		}
		plugin = initialize_plugin_object(plugin_parms,plugin);
		if(plugin == NULL)
			return(0);
		v.push_back(plugin);
    }
	inf.close();
	remove_devices(new_conf_ports);
	print_key_message();
return(1);
}
/*****************************************************************************************/

int generic_server::set_plugins_path(vector<string> &plugin_parms)
{
	if(plugins_path != "")
	{
		plugin_parms.at(SHARED_LIB_PATH) = plugins_path + "//" + plugin_parms.at(SHARED_LIB_PATH);
		if(plugin_parms.size() > PLUGIN_CONF)
			plugin_parms.at(PLUGIN_CONF) = plugins_path + "//" + plugin_parms.at(PLUGIN_CONF);
	}
	return(1);
}
/*****************************************************************************************/

string generic_server::get_plugins_path()
{
	return(plugins_path);
}
/*****************************************************************************************/

GENERIC_PLUGIN *generic_server::initialize_plugin_object(vector<string> plugin_parms,GENERIC_PLUGIN *plugin)
{
	int ret;
	ostringstream os;
	SOCKET ListenSocket;
	string bootstrap_name;;

	plugin->set_port(atoi(plugin_parms.at(PORT).c_str()));
	plugin->set_plugin(plugin_parms.at(PLUGIN_TYPE));
	plugin->set_plugin_name(plugin_parms.at(PLUGIN_NAME));
	plugin->set_plugin_number(plugin_parms.at(PLUGIN_NUMBER));
	plugin->set_plugin_path(plugin_parms.at(SHARED_LIB_PATH));
	plugin->set_tls_flag(atoi(plugin_parms.at(TLS_FLAG).c_str()));
	memcpy(&plugin->pinstance,&pinstance,4);
	if( (ListenSocket = plugin->initialize_socket(plugin_parms.at(PORT))) == 0)
		return(NULL);
	plugin->set_socket(ListenSocket);
	if(plugin->get_tls_flag())
		tls_setup_reqd = 1;
	if(plugin_parms.size() > PLUGIN_CONF)
	{
		plugin->set_conf_file(plugin_parms.at(PLUGIN_CONF));
		ret = plugin_read_conf_file(plugin,0);
        if(!ret)         // read in plugin_specific config parameters
        {
            os  << " Thr.ID:MAIN plugin_init failed for: " << plugin->get_plugin_name() << endl;
            log(LOG_LOW,LOG_LOW,os.str());
            return(NULL);
		}
	}
	bootstrap_name = plugin->bootstrap_name();
	map<string,bool>::iterator it = bootstrap_jobs.find(bootstrap_name);
	if(it == bootstrap_jobs.end() && bootstrap_name != "")
	{
		os  << " Thr.ID:MAIN Invoking bootstrap: " << bootstrap_name << " on plug-in: " << plugin->get_plugin_name();
		log(LOG_LOW,LOG_LOW,os.str());
		os.str("");
		os.clear();
		if(!plugin->bootstrap_init(bootstrap_name))
		{
			os  << " Thr.ID:MAIN failed to bootstrap: " << bootstrap_name << " on plug-in: " << plugin->get_plugin_name();
            log(LOG_LOW,LOG_LOW,os.str());
            return(NULL);
		}
		bootstrap_jobs[bootstrap_name] = true;
		os  << " Thr.ID:MAIN Successfully bootstrapped: " <<  bootstrap_name << " on plug-in: " << plugin->get_plugin_name();
		log(LOG_LOW,LOG_LOW,os.str());
		os.str("");
		os.clear();
	}
	if(!check_enabled_plugin(plugin_parms.at(PLUGIN_TYPE)))
	{
		if(!plugin->server_init()) // Will do only once per plugin type
		{
			os  << " Thr.ID:MAIN thread. server_init failed for: " << plugin_parms.at(PLUGIN_TYPE) << endl;
			log(LOG_LOW,LOG_LOW,os.str());
			return(NULL);
		}
	}
	setnonblocking(ListenSocket);
	return(plugin);
}
/*****************************************************************************************/

int generic_server::setnonblocking(SOCKET sock)
{
	int iResult;
#ifdef WINDOWS
	u_long iMode = 1;
	iResult = ioctlsocket(sock, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		return(0);
	return(1);
#else
	int	opts = fcntl(sock,F_GETFL);
	if (opts < 0) 
		return(0);
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) 
		return(0);
	return(1);
#endif
}
/*****************************************************************************************/

int generic_server::setblocking(SOCKET sock)
{
    int iResult;
#ifdef WINDOWS
    u_long iMode = 0;
    iResult = ioctlsocket(sock, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
        return(0);
    return(1);
#else
    int opts = fcntl(sock,F_GETFL);
    if (opts < 0)
        return(0);
    opts = (opts | ~O_NONBLOCK);
    if (fcntl(sock,F_SETFL,opts) < 0)
        return(0);
    return(1);
#endif
}
/*****************************************************************************************/

SOCKET generic_server::create_socket(int port)
{
    SOCKET ListenSocket;
    struct addrinfo *result = NULL;
    struct sockaddr_in local_address;
    struct addrinfo hints;
    ostringstream os;
    int iResult;

	os << port;
    memset(&local_address,0,sizeof(local_address));
    memset(&hints, 0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, os.str().c_str(), &hints, &result);
    if (iResult != 0)
    {
        os.str("");
        os.clear();
        os  << " Thr.ID:MAIN getaddrinfo failed.";
        log(LOG_LOW,LOG_LOW,os.str());
    }
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        os.str("");
        os.clear();
        os  << " Thr.ID:MAIN socket creation failed.";
        log(LOG_LOW,LOG_LOW,os.str());
        freeaddrinfo(result);
        return 0;
    }
    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    local_address.sin_port = htons(port);
    iResult = bind(ListenSocket, (sockaddr *) &local_address, (int)result->ai_addrlen);
    if (iResult != 0)
    {
        os.str("");
        os.clear();
        os  << " Thr.ID:MAIN socket bind failed for port: " << port;
        log(LOG_LOW,LOG_LOW,os.str());
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
        os  << " Thr.ID:MAIN socket listen failed for port: " << port;
        log(LOG_LOW,LOG_LOW,os.str());
        shutdown(ListenSocket,2);
#ifdef WINDOWS
        closesocket(ListenSocket);
#else
        close(ListenSocket);
#endif
        return 0;
    }
	setnonblocking(ListenSocket);
    return(ListenSocket);
}
/*********************************************************************************************/

int generic_server::syslog_msg(unsigned int debug_level,unsigned int verbose_level,string data)
{
    unsigned int msk = 0xFFFFFFFF;
    msk = msk << 4;
    debug_level = debug_level & ~msk;
    if (verbose_level >= debug_level)
    {
        switch(debug_level)
        {
#ifdef LINUX
            case LOG_LOW:
                syslog(LOG_INFO,"%s",trim(data).c_str());
                break;
            case LOG_MEDIUM:
                syslog(LOG_NOTICE,"%s",trim(data).c_str());
                break;
            case LOG_HI:
                syslog(LOG_WARNING,"%s",trim(data).c_str());
                break;
#else
            case LOG_LOW:
                el.Write(EVENTLOG_INFORMATION_TYPE, trim(data).c_str());
                break;
            case LOG_MEDIUM:
                el.Write(EVENTLOG_INFORMATION_TYPE, trim(data).c_str());
                break;
            case LOG_HI:
                el.Write(EVENTLOG_WARNING_TYPE, trim(data).c_str());
                break;
#endif
        }
    }

    return(1);
}
/*********************************************************************************************/

int generic_server::ssl_async_write(SSL *ssl,char *buf,int size)
{
	signed int status,status1,ssl_response;
	ostringstream oss;
	ssl_response = 1;
	while(ssl_response == 1)
	{
		oss.str("");
		oss.clear();
		status = SSL_write((SSL *)ssl,buf,size);
		if(status > 0)
			break;
		status1 = SSL_get_error((SSL *)ssl,status);
		switch(status1)
		{
			case SSL_ERROR_WANT_READ:
				oss << " AWRITE: ERROR: SSL_ERROR_WANT_READ ";
				break;

			case SSL_ERROR_WANT_WRITE:
				oss << " AWRITE: ERROR: SSL_ERROR_WANT_WRITE ";
				break;

			case SSL_ERROR_WANT_CONNECT:
				oss << " AWRITE: ERROR: SSL_ERROR_WANT_CONNECT ";
				break;

			case SSL_ERROR_WANT_ACCEPT:
				oss << " AWRITE: ERROR: SSL_ERROR_WANT_ACCEPT ";
				break;

			case SSL_ERROR_SYSCALL:
				oss << " AWRITE: ERROR: SSL_ERROR_SYSCALL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_NONE:
				oss << " AWRITE: ERROR: SSL_ERROR_NONE ";
				ssl_response = 0;
				break;

			case SSL_ERROR_SSL:
				oss << " AWRITE: ERROR: SSL_ERROR_SSL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_ZERO_RETURN:
				oss << " AWRITE ERROR: SSL_ERROR_ZERO_RETURN ";
				ssl_response = 0;
				break;
		}
	}
	if(ssl_response < 0)
	{
		log(LOG_LOW,LOG_LOW,oss.str());
		return(ssl_response);
	}
	if(ssl_response == 0)
	{
		log(LOG_LOW,LOG_LOW,oss.str());
		return(0);
	}
	return(1);
}
/*********************************************************************************************/

int generic_server::ssl_async_read(SSL *ssl,char *buf,int size)
{
	signed int status,status1,ssl_response;
	ostringstream oss;
	ssl_response = 1;
	while(ssl_response == 1)
	{
		oss.str("");
		oss.clear();
		status = SSL_read((SSL *)ssl,buf,size);
		if(status > 0)
			break;
		status1 = SSL_get_error((SSL *)ssl,status);
		switch(status1)
		{
			case SSL_ERROR_WANT_READ:
				oss << " AREAD: ERROR: SSL_ERROR_WANT_READ ";
				break;

			case SSL_ERROR_WANT_WRITE:
				oss << " AREAD: ERROR: SSL_ERROR_WANT_WRITE ";
				break;

			case SSL_ERROR_WANT_CONNECT:
				oss << " AREAD: ERROR: SSL_ERROR_WANT_CONNECT ";
				break;

			case SSL_ERROR_WANT_ACCEPT:
				oss << " AREAD: ERROR: SSL_ERROR_WANT_ACCEPT ";
				break;

			case SSL_ERROR_SYSCALL:
				oss << " AREAD: ERROR: SSL_ERROR_SYSCALL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_NONE:
				oss << " AREAD: ERROR: SSL_ERROR_NONE ";
				ssl_response = 0;
				break;

			case SSL_ERROR_SSL:
				oss << " AREAD: ERROR: SSL_ERROR_SSL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_ZERO_RETURN:
				oss << " AREAD: ERROR: SSL_ERROR_ZERO_RETURN ";
				ssl_response = 0;
				break;
		}
	}
	if(ssl_response < 0)
	{
		log(LOG_LOW,LOG_LOW,oss.str());
		return(ssl_response);
	}
	if(ssl_response == 0)
	{
		log(LOG_LOW,LOG_LOW,oss.str());
		return(0);
	}
	return(status);
}
/*********************************************************************************************/

int generic_server::ssl_async_accept(SSL *ssl)
{
	signed int status,status1,ssl_response;
	ostringstream oss;
	ssl_response = 1;
	while(ssl_response == 1)
	{
		oss.str("");
		oss.clear();
		status = SSL_accept((SSL *)ssl);
		if(status > 0)
			break;
		status1 = SSL_get_error((SSL *)ssl,status);
		switch(status1)
		{
			case SSL_ERROR_WANT_READ:
				oss << " ACCEPT: ERROR: SSL_ERROR_WANT_READ ";
				break;

			case SSL_ERROR_WANT_WRITE:
				oss << " ACCEPT: ERROR: SSL_ERROR_WANT_WRITE ";
				break;

			case SSL_ERROR_WANT_X509_LOOKUP:
				oss << " ACCEPT: ERROR: SSL_ERROR_WANT_CONNECT ";
				break;

			case SSL_ERROR_SYSCALL:
				oss << " ACCEPT: ERROR: SSL_ERROR_SYSCALL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_NONE:
				oss << " ACCEPT: ERROR: SSL_ERROR_NONE ";
				ssl_response = 0;
				break;

			case SSL_ERROR_SSL:
				oss << " ACCEPT: ERROR: SSL_ERROR_SSL ";
				ssl_response = -1;
				break;

			case SSL_ERROR_ZERO_RETURN:
				oss << " ACCEPT: ERROR: SSL_ERROR_ZERO_RETURN ";
				ssl_response = 0;
				break;
		}
	}
	if(ssl_response <= 0)
	{
		log(LOG_LOW,LOG_LOW,oss.str());
		return(0);
	}
	return(status);
}
/*********************************************************************************************/
