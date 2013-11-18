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
#include "control.h"
#ifdef WINDOWS
BOOL CtrlHandler( DWORD fdwCtrlType);
#endif
/*********************************************************************************************/

#ifdef LINUX
void  *thread_fn(void *inp_arg)
#else
unsigned _stdcall thread_fn(void *inp_arg)
#endif
{
int thread_no,plugin_init_state,iResult;
GENERIC_PLUGIN *plugin_p,*plugin_inp;
SOCKET clnt_sock,nfds;
fd_set socks;
generic_server *fwork = generic_server::pinstance;
ostringstream oss;
    fwork->rel_thread_sync_mutex();
    try
    {
        plugin_inp = dynamic_cast<GENERIC_PLUGIN *>((GENERIC_PLUGIN *)inp_arg);
        plugin_p = (fwork->factory[plugin_inp->get_plugin()])();
        *plugin_p = *plugin_inp;
        delete plugin_inp;
        thread_no = plugin_p->get_thread_id();
        fwork->plugin_read_conf_file(plugin_p,thread_no);
        plugin_init_state = 1;
           clnt_sock = (SOCKET) plugin_p->get_client_socket();
        iResult = plugin_p->plugin_init(thread_no);
        if(iResult == 0)
            plugin_init_state = 0;
        struct timeval timeout;
        // This function gets called after client makes a 'connect' request to framework
        // Loop to read data from one client. One thread -> 1 client
        while(1)
        {
            FD_ZERO(&socks);
            FD_SET (clnt_sock, &socks);
            nfds = clnt_sock;
            timeout.tv_sec = fwork->timeout_seconds;
            timeout.tv_usec = 0;
            iResult = select((int)nfds+1, &socks, NULL, NULL, &timeout);
            if (iResult <= 0) // client did not send any requests within TIMEOUT interval. 
                throw (char *)"97 Thread timeout.";
            else if (iResult > 0 && FD_ISSET (clnt_sock, &socks))
            {
                if(!process_client_req(clnt_sock,plugin_init_state,plugin_p,thread_no,fwork))
                       throw 0;
            }
            else 
                throw (char *)"98 client disconnect.";
        }
    }
    catch(int i)
    {
        terminate_client_session(clnt_sock,fwork,plugin_p,thread_no,(unsigned char *)"00 Terminating.");
    }
    catch(char *cp)
    {
        if(cp != 0)
            terminate_client_session(clnt_sock,fwork,plugin_p,thread_no,(unsigned char *)cp);
	}
#ifdef LINUX
    return(NULL);
#else
    return(0);
#endif
}
/*********************************************************************************************/

#ifdef LINUX
void  *ssl_thread_fn(void *inp_arg)
#else
unsigned _stdcall ssl_thread_fn(void *inp_arg)
#endif
{
    int thread_no,plugin_init_state;
    signed int status;
    GENERIC_PLUGIN *plugin_p,*plugin_inp;
    SOCKET clnt_sock,nfds;
    fd_set socks;
    SSL *ssl;
    ostringstream oss;
    generic_server *fwork = generic_server::pinstance;
    fwork->rel_thread_sync_mutex();
    try
    {
        plugin_inp = dynamic_cast<GENERIC_PLUGIN *>((GENERIC_PLUGIN *)inp_arg);
        plugin_p = (fwork->factory[plugin_inp->get_plugin()])();
        *plugin_p = *plugin_inp;
        delete plugin_inp;    
        ssl = SSL_new(fwork->sslserver.get_ctx());
        thread_no = plugin_p->get_thread_id();
        fwork->plugin_read_conf_file(plugin_p,thread_no);
        plugin_init_state = 1;
        clnt_sock = (SOCKET) plugin_p->get_client_socket();
        status = SSL_set_fd(ssl, clnt_sock);
        if(status == 0)
            throw (char *) "86 SSL_set_fd failed.";
        if(fwork->get_verify_client())
            SSL_set_verify(ssl,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL);
        status = fwork->ssl_async_accept((SSL *)ssl);
        if(status == 0)
            throw (char *) "87 SSL_accept failed.";
        status = plugin_p->plugin_init(thread_no);
        if(status == 0)
            plugin_init_state = 0;
        struct timeval timeout;
        // This function gets called after client makes a 'connect' request to framework
        // Loop to read data from one client. One thread -> 1 client
        while(1)
        {
            FD_ZERO(&socks);
            FD_SET(clnt_sock, &socks);
            nfds = clnt_sock;
            timeout.tv_sec = fwork->timeout_seconds;
            timeout.tv_usec = 0;
            status = select((int)nfds+1, &socks, NULL, NULL, &timeout);
            if (status <= 0) // client did not send any requests within TIMEOUT interval. 
                throw 1;
            else if (status > 0 && FD_ISSET (clnt_sock, &socks))
            {
                if(!process_ssl_client_req(clnt_sock,ssl,plugin_init_state,plugin_p,thread_no,fwork))
                    throw 0;
            }
            else
                throw (char *)"89 client disconnect.";
        }
    }
    catch(int err)
    {
		if(err == 1)
			terminate_ssl_client_session(ssl,fwork,plugin_p,thread_no,(unsigned char *)"88 Thread timeout.");
		if(err == 0)
			terminate_ssl_client_session(ssl,fwork,plugin_p,thread_no,(unsigned char *)"00 Terminating.");
    }
    catch(char *cp)
    {
        if(cp != 0)
            terminate_ssl_client_session(ssl,fwork,plugin_p,thread_no,(unsigned char *)cp);
	}
#ifdef LINUX
	return(NULL);
#else
    return(0);
#endif
}
/*********************************************************************************************/

static int process_ssl_client_req(SOCKET clnt_sock,SSL *ssl,int plugin_init_state,
                                  GENERIC_PLUGIN *plugin_p,int thread_no,generic_server *fwork)
{
unsigned int size,result;
char recvbuf[MAX_SZ];
int len,offset,ssl_response;
string plugin_name;
unsigned char sendbuf[MAX_SZ];
ostringstream os;

    memset(sendbuf,0,MAX_SZ);
    memset(recvbuf,0,MAX_SZ);
	size = 0;
	offset = 0;	
    try
    {
        if(plugin_init_state == 0)  
            throw (char *)" 88 Can't inititalize plug-in.";
        len = recv(SSL_get_fd(ssl), (char*) recvbuf, MAX_SZ, MSG_PEEK);
        if(len <= 0)
            throw (char *)" 89 client disconnected.";
		// MESSAGE HEADER:
		// SIGNATURE        - 4 bytes SIGNATURE for all messages
		// PAYLOAD_SIZE     - 4 bytes network byte order 
		// DATA             - PAYLOAD_SIZE data
		// Data payload:
		// PLUGIN_NAME_LEN  - 4 bytes network byte order (optional)
		// PLUGIN_NAME      - Size as defined by <PLUGIN_NAME_LEN> (optional)
		// APPLICATION_DATA - PAYLOAD_SIZE-([4+PLUGIN_NAME_LEN])
		len = fwork->ssl_async_read((SSL *)ssl, recvbuf, 8);	// Remove SIGNATURE+PAYLOAD_SIZE
 		if(len < 0)	 
		throw (char *)" 90 Invalid data received from client.";
			if(len == 0)
				return(0);
		// check SIGNATURE and get PAYLOAD_SIZE				
		// size is the size_of_data sent by client, this should
		// exclude SIGNATURE
		size = get_data_size((unsigned char *)recvbuf);
		if(size > (unsigned int) MAX_SZ-offset || size == 0)
			throw 2;
		len = fwork->ssl_async_read((SSL *)ssl, recvbuf, size);
 		if(len < 0)	 
			throw (char *)" 90 Invalid data received from client.";
		if(len == 0)
			return(0);
		if(plugin_p->get_validate_plugin())
		{
			offset = 4;	// Remove plugin_name_len
			plugin_name = get_input_plugin_name(recvbuf);
			offset += plugin_name.length();	// Remove plugin_name 
			if(plugin_name != plugin_p->get_plugin_name())
			{
				if(!plugin_p->find_plugin_alias(plugin_name))
					throw 1;
				// Moving primary device name to alias list for this session..
				plugin_p->add_plugin_alias(plugin_p->get_plugin_name());
				// setting alias name as primary Plugin for this thread..
				plugin_p->set_plugin_name(plugin_name);			
			}
		}
		size -= offset;
		result = plugin_p->process_request(recvbuf+offset,sendbuf,size);
		if(result == TERMINATE_SESSION)
			throw 3;
		add_size(sendbuf,size);
		ssl_response = fwork->ssl_async_write((SSL *)ssl, (char *)sendbuf, size+8);
		if(ssl_response < 0)
			throw (char *)" 93 SSL_write error.";
		if(ssl_response == 0)
			throw 3;
		if(!print_source_hostname(clnt_sock,TLS_REQ,fwork,thread_no,plugin_p))
			return(0);
     }
     catch(char *cp)
     {
		os  << " Thr.ID:" <<  thread_no << " Plugin:" <<  plugin_p->get_plugin_name() << cp;
		fwork->log(LOG_LOW,LOG_LOW,os.str());
        return 0;
     }
	 catch(int err)
	 {
		 switch(err)
		 {
			case 1:
				strcpy((char *)sendbuf,"92 Invalid plugin_name sent by client.");
			case 2:
				strcpy((char *)sendbuf,"91 Invalid message.");
			case 3:
				strcpy((char *)sendbuf,"00 Terminating..");
		 }
		 size = strlen((char *)sendbuf);
		 add_size(sendbuf,size);
		 fwork->ssl_async_write((SSL *)ssl, (char *)sendbuf, size+8);
		 return(0);
	 }
	 return(1);
}
/*******************************************************************************************/

int print_source_hostname(int clnt_sock,int req_type,generic_server *fwork,int thread_no,GENERIC_PLUGIN *plugin_p)
{
    struct sockaddr_storage peer;
    struct sockaddr_in *s;
    int peer_len;
    ostringstream os;

    peer_len = sizeof(sockaddr_storage);
    if( (getpeername(clnt_sock, (sockaddr *)&peer,(socklen_t*) &peer_len)) != SOCKET_ERROR)
    {
        s = (sockaddr_in *) &peer;
        os  <<" Thr.ID:" <<  thread_no << " Plugin:" <<  plugin_p->get_plugin_name() << " Request from: ";
        os  << inet_ntoa(s->sin_addr);
    }
    fwork->log(LOG_LOW,LOG_LOW,os.str());
    return(1);
}
/*******************************************************************************************/

static int process_client_req(SOCKET clnt_sock,int plugin_init_state,GENERIC_PLUGIN *plugin_p,int thread_no,generic_server *fwork)
{
unsigned int size,result;
char recvbuf[MAX_SZ];
int len,offset;
bool first;
string plugin_name;
unsigned char sendbuf[MAX_SZ];
ostringstream  os;
    memset(sendbuf,0,MAX_SZ);
    memset(recvbuf,0,MAX_SZ);
	offset = 0;
    try
    {
        if(plugin_init_state == 0)  
            throw (char *)" 88 Can't inititalize plug-in.";
        len = recv(clnt_sock, (char*) recvbuf, MAX_SZ, MSG_PEEK);
        if(len <= 0)
			throw (char *)" 89 client disconnected.";
		len = read_socket((unsigned char*) recvbuf,clnt_sock, 8);
		if(len <= 0)
			throw (char *)" 90 1 Invalid data received from client.";
		// MESSAGE HEADER:
		// SIGNATURE        - 4 bytes SIGNATURE for all messages
		// PAYLOAD_SIZE     - 4 bytes network byte order 
		// DATA             - PAYLOAD_SIZE data
		// Data payload:
		// PLUGIN_NAME_LEN  - 4 bytes network byte order (optional)
		// PLUGIN_NAME      - Size as defined by <PLUGIN_NAME_LEN> (optional)
		// APPLICATION_DATA - PAYLOAD_SIZE-([4+PLUGIN_NAME_LEN])

		// check SIGNATURE and get PAYLOAD_SIZE				
		// size is the size_of_data sent by client, this should
		// exclude SIGNATURE
		size = get_data_size((unsigned char *)recvbuf);
		if(size > (unsigned int) MAX_SZ-offset || size == 0)
			throw 2;
		len = read_socket((unsigned char*) recvbuf,clnt_sock, size);
		if(len <= 0)
			throw (char *)" 90 2 Invalid data received from client.";
		if(plugin_p->get_validate_plugin())
		{
			offset = 4;	// Remove plugin_name_len
			plugin_name = get_input_plugin_name(recvbuf);
			offset += plugin_name.length();	// Remove plugin_name 
			if(plugin_name != plugin_p->get_plugin_name())
			{
				if(!plugin_p->find_plugin_alias(plugin_name))
					throw 1;
				// Moving primary device name to alias list for this session..
				plugin_p->add_plugin_alias(plugin_p->get_plugin_name());
				// setting alias name as primary Plugin for this thread..
				plugin_p->set_plugin_name(plugin_name);			
			}
		}
		size -= offset;
		result = plugin_p->process_request(recvbuf+offset,sendbuf,size);
		if(result == TERMINATE_SESSION)
			throw 3;
		add_size(sendbuf,size);
		result = send(clnt_sock, (char *)sendbuf,size+8,0);
		if(result <= 0)
			throw (char *) " 00 Terminating..";
        if(!print_source_hostname(clnt_sock,SOCKET_REQ,fwork,thread_no,plugin_p))
            return(0);
     }
     catch(char *cp)
     {
		os  << " Thr.ID:" <<  thread_no << " Plugin:" <<  plugin_p->get_plugin_name() << cp;
		fwork->log(LOG_LOW,LOG_LOW,os.str());
        return(0);
     }
	 catch(int err)
	 {
		 switch(err)
		 {
			case 1:
				strcpy((char *)sendbuf,"92 Invalid plugin_name sent by client.");
			case 2:
				strcpy((char *)sendbuf,"91 Invalid message.");
		 }
		 size = strlen((char *)sendbuf);
		 add_size(sendbuf,size);
		 send(clnt_sock, (char *)sendbuf,size+8,0);
		 return(0);
	 }
	 return(1);
}
/*******************************************************************************************/

int terminate_client_session(SOCKET clnt_sock,generic_server *fwork, GENERIC_PLUGIN *plugin_p,int thread_no,unsigned char *sendbuf)
{
    int len;
    ostringstream os;

    len = strlen((char *)sendbuf);
    send(clnt_sock, (char *)sendbuf, (int)len, 0 );
    plugin_p->shutdown_plugin(); // Aborting SESSION..
    os.str("");
    os.clear();
    os  << " Thr.ID:" << thread_no << " Plugin:" << plugin_p->get_plugin_name() << " Quitting thread." << sendbuf;
    fwork->log(LOG_LOW,LOG_LOW,os.str());
	fwork->socket_close(clnt_sock);
    delete plugin_p;
    fwork->rel_thread_id(thread_no);
    return(1);
}
/*********************************************************************************************/

int terminate_ssl_client_session(SSL *ssl,generic_server *fwork, GENERIC_PLUGIN *plugin_p,int thread_no,unsigned char *sendbuf)
{
  	int len,ssl_response,status;
	ostringstream os;
	char buf[100];

    len = strlen((char *)sendbuf);
	status = recv(SSL_get_fd(ssl), (char*) buf, 10, MSG_PEEK);
    if(status > 0)
	{
		ssl_response = fwork->ssl_async_write((SSL *)ssl,  (char *)sendbuf, (int)len);
		if(ssl_response > 0)
		{
			while((SSL_shutdown(ssl)) == 0)
			;
		}
	}
    plugin_p->shutdown_plugin(); 
    os  << " Thr.ID:" << thread_no << " Plugin:" << plugin_p->get_plugin_name() << " Quitting thread." << sendbuf;
    fwork->log(LOG_LOW,LOG_LOW,os.str());
	fwork->socket_close(SSL_get_fd(ssl));
	SSL_free((SSL *)ssl);
	ERR_free_strings();
	ERR_remove_state(0);
    delete plugin_p;
    fwork->rel_thread_id(thread_no);
    return(1);
}
/*******************************************************************************************/

#ifdef LINUX
int main(int argc, char* argv[]) 
#else
VOID start_service(LPSTR* argv)
#endif
{
    char pname[100],daemon_flag,cfile[MAX_SZ];
    generic_server *framework = generic_server::instance();
    ostringstream os;
    daemon_flag = 0;
    memset(cfile,0,MAX_SZ);
#ifdef LINUX
    if(argc < 3)
    {
        cout << "Usage: generic_server.exe <generic_server_name> <conf file>" << endl;
        exit(-1);
    }
    strncpy(cfile,argv[2],strlen(argv[2]));
    if(argc == 4)
        daemon_flag = atoi(argv[3]);
    strcpy(pname,argv[1]);
    for(int i = 0;i < argc; i++)
        memset(argv[i],' ',strlen(argv[i]));
    strcpy(argv[0],pname);
    prctl(PR_SET_NAME,(unsigned long)pname,0,0,0);
#else
    string tmp;
    istringstream iss(GetCommandLine());
    iss >> tmp;
    iss >> tmp;
    if(!tmp.empty())
        strcpy(cfile,(char *)tmp.c_str());
    else
        strcpy(cfile,(char *)"C:\\generic_server\\generic_server.conf");
    strcpy(pname,argv[0]);
#endif
    framework->set_conf_file(cfile);
    framework->set_process_name(pname);
    if(!framework->init())
    {
        framework->log(LOG_LOW,LOG_LOW,(char *)" FATAL error. Please check for configuration errors!.");
        exit(-1);
    }
#ifdef LINUX
    if(!daemon_flag)
        daemonize();
#else
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE);
#endif
    framework->print_key_message();
    // Enter infinite loop and dispatch callbacks
    process_data();
}
/*******************************************************************************************/
#ifndef WINDOWS
void daemonize()
{
    int i,lfp;
    char str[10];

    if(getppid()==1)
        return;                       /* already a daemon                   */
    i=fork();
    if (i < 0)
    {
        cerr << "Unable to fork()..." << endl;
        exit(1);                      /* can not open                       */
    }
    if (i > 0)
        exit(0);                      /* parent exits                       */
                                      /* child (daemon) continues           */
    setsid();                         /* obtain a new process group         */
    umask(022);                       /* set newly created file permissions */
    signal(SIGCHLD,SIG_IGN);          /* ignore child                       */
    signal(SIGTSTP,SIG_IGN);          /* ignore tty signals                 */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);
    signal(SIGHUP,do_nothing);        /* catch hangup signal                */
    signal(SIGTERM,sig_int);          /* catch kill signal                  */
}
/*****************************************************************************************/
#else
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason,    // reason for calling function
    LPVOID lpReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch(fdwReason) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         //sig_int(0);    
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
        //sig_int(0);        // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
/*****************************************************************************************/
#endif

void sig_int(int sig_num)
{
    generic_server *framework = generic_server::pinstance;
    vector <GENERIC_PLUGIN *>::iterator it;
    ostringstream os;

    os  << " Thr.ID:MAIN thread. Signal received to terminate.. " << endl;
    for(it = framework->get_start_iterator(); it != framework->get_end_iterator();it++)
    {
		map<string,bool>::iterator bootstrap_itr = framework->bootstrap_jobs.find((*it)->bootstrap_name());
		// Cleanup bootstrapped stuff..
		if(bootstrap_itr != framework->bootstrap_jobs.end() && (*it)->bootstrap_name() != "")
		{
			os  << "\tThr.ID:MAIN. Invoking bootstrap terminate: " << (*it)->bootstrap_name() << " on plug-in: " << (*it)->get_plugin_name() << endl;
			if(!(*it)->bootstrap_terminate((*it)->bootstrap_name()))
			{
				os  << "\tThr.ID:MAIN failed to terminate bootstrap: " << (*it)->bootstrap_name() << " on plug-in: " << (*it)->get_plugin_name() << endl;
			}
			framework->bootstrap_jobs.erase(bootstrap_itr);
		}
		if( (framework->check_enabled_plugin((*it)->get_plugin())) > 1)
			it = framework->v.erase(it);
		else
		{
			os << "\tShutting down: " << *(*it) << endl;
			(*it)->server_shutdown();
			it = framework->v.erase(it);
		}
    }
    framework->log(LOG_LOW,LOG_LOW,(char *)os.str().c_str());
    framework->close_log();
    exit(0);
}
/*****************************************************************************************/

int process_data(void)
{
    SOCKET command_sock;
    fd_set socks;
    size_t readsock;
    unsigned int nfds;
    generic_server *fwork = generic_server::pinstance;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 20;
	command_sock = fwork->create_socket(fwork->get_command_port());
	if(command_sock == 0)
		return(0);
    while(1)
    {
		nfds = 0;
        FD_ZERO(&socks);
        nfds = init_sock_fds(&socks,command_sock);
        readsock = select(nfds+1,&socks,NULL,NULL,0);
        if(errno == EINTR)
        {
            fwork->reload_conf_file();
            continue;
        }
        if(readsock <= 0)
            continue;
        if(chk_command_sock(&socks,command_sock))
            continue;
        chk_all_clients(&socks);
        if(!fwork->syslog_flag)
            fwork->rollover_logs();
    }
}
/*****************************************************************************************/

unsigned int init_sock_fds(fd_set *socks,SOCKET command_sock)
{
    unsigned int nfds;
    generic_server *fwork = generic_server::pinstance;
    nfds = 0;
    for(fwork->itr = fwork->v.begin(); fwork->itr != fwork->v.end(); ++fwork->itr)
    {
        if(nfds < (*fwork->itr)->get_socket())
            nfds =  (unsigned int)(*fwork->itr)->get_socket();
        FD_SET((*fwork->itr)->get_socket(),socks);
    }
    FD_SET(command_sock,socks);
    return(nfds);
}
/*****************************************************************************************/

int chk_command_sock(fd_set *socks,SOCKET command_sock)
{
    generic_server *fwork = generic_server::pinstance;
    SOCKET clnt_sock;

    if(FD_ISSET(command_sock,socks))
    {
        clnt_sock = accept(command_sock,NULL,NULL);
        if(clnt_sock < 0)
            return(0);
        shutdown(clnt_sock,2);
#ifdef WINDOWS
        closesocket(clnt_sock);
#else
        close(clnt_sock);
#endif
        fwork->reload_conf_file();
        return(1);
    }
    return(0);
}
/*****************************************************************************************/

int launch_thread(GENERIC_PLUGIN *generic_plugin,unsigned int new_slot)
{
    generic_server *fwork = generic_server::pinstance;
#ifdef LINUX
	pthread_attr_t attr; // thread attribute
						 // set thread detachstate attribute to DETACHED 
    fwork->get_thread_sync_mutex();
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#endif
    if(generic_plugin->get_tls_flag())
    {
#ifdef WINDOWS
        fwork->get_thread_sync_mutex();
		fwork->thread_ids[new_slot].th = (HANDLE)_beginthreadex(NULL, 0, &ssl_thread_fn, (void *)generic_plugin, 0, NULL );
#else
        pthread_create(&fwork->thread_ids[new_slot].th,&attr,ssl_thread_fn, (void *)generic_plugin);
#endif
    }
    else
    {
#ifdef WINDOWS
        fwork->get_thread_sync_mutex();
		fwork->thread_ids[new_slot].th = (HANDLE)_beginthreadex(NULL, 0, &thread_fn, (void *)generic_plugin, 0, NULL );
#else
        pthread_create(&fwork->thread_ids[new_slot].th,&attr,thread_fn, (void *)generic_plugin);
#endif
    }
#ifdef LINUX
    pthread_attr_destroy(&attr);
#endif
    fwork->close_thread_ids();
    return(1);
}
/*****************************************************************************************/

int chk_all_clients(fd_set *socks)
{
    generic_server *fwork = generic_server::pinstance;
    SOCKET clnt_sock;
    unsigned int len,new_slot;
    GENERIC_PLUGIN *generic_plugin;
    char sendbuf[MAX_SZ];

    memset(sendbuf,0,MAX_SZ);
    for(fwork->itr = fwork->v.begin(); fwork->itr != fwork->v.end(); ++fwork->itr)
    {
        if(FD_ISSET((*fwork->itr)->get_socket(),socks))
        {
            clnt_sock = accept((*fwork->itr)->get_socket(),NULL,NULL);
            if(clnt_sock < 0)
                continue;
            fwork->setnonblocking(clnt_sock);
            len = recv(clnt_sock, (char*) sendbuf, 10, MSG_PEEK);
            if(len <= 0)
            {
                shutdown(clnt_sock,2);
                fwork->log(LOG_LOW,LOG_LOW,(char *)"85 client disconnected.");
                continue;
            }
            if(fwork->thread_ids.size() >= (unsigned int) fwork->max_threads)
            {
                fwork->log(LOG_LOW,LOG_LOW,(char *)"11 Too many threads running. Try later..");
                shutdown(clnt_sock,2);
#ifdef WINDOWS
                closesocket(clnt_sock);
#else
                close(clnt_sock);
#endif
            }
            else
            {
				fwork->close_thread_ids();
                generic_plugin = new GENERIC_PLUGIN;
                *generic_plugin = (GENERIC_PLUGIN) *(*fwork->itr);
                generic_plugin->set_client_socket(clnt_sock);
                new_slot = fwork->get_thread_id();
                generic_plugin->set_thread_id(new_slot);
                launch_thread(generic_plugin,new_slot);
            }
        }
    }
    return(1);
}
/*****************************************************************************************/
#ifdef WINDOWS
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
        sig_int(fdwCtrlType);
        return(TRUE);
    case CTRL_BREAK_EVENT: 
        return(TRUE);
    default: 
      return FALSE; 
  } 
}
/*****************************************************************************************/
#endif

void do_nothing(int sig_num)
{
}
/*********************************************************************************************/

unsigned int get_data_size(unsigned char *recvbuf)
{
	unsigned int size,plugin_signature;

	plugin_signature = htonl(PLUGIN_SIGNATURE);
	// check for PLUGIN_SIGNATURE
	if(memcmp(recvbuf,(char *)&plugin_signature,4))
		return(0);
	memcpy(&size,recvbuf+4,4);
	size = ntohl(size);
	return(size);
}
/*******************************************************************************************/

int add_size(unsigned char *recvbuf,unsigned int size)
{
	int sav_size;
	unsigned int plugin_signature;
	char buffer[MAX_SZ+8];

	plugin_signature = htonl(PLUGIN_SIGNATURE);
	sav_size = size;
	size = htonl(size);
	memcpy(buffer,(char *)&plugin_signature,4);
	memcpy(buffer+4,&size,4);
	memcpy(buffer+8,recvbuf,sav_size);
	memcpy(recvbuf,buffer,sav_size+8);
	return(1);
}
/*******************************************************************************************/

string get_input_plugin_name(char *buffer)
{
	string plugin_name;
	unsigned int size;

	memcpy((char *)&size,buffer,4);
	size = ntohl(size);
	plugin_name = string(buffer+4,size);
	return(plugin_name);
}
/*******************************************************************************************/

static int read_socket(unsigned char *buffer,SOCKET sock,int size)
{
    fd_set socks;
    struct timeval timeout;
    char str[MAX_SZ];
    size_t readsock;
    int t,bytes_recvd;
    bytes_recvd = 0;

    memset(buffer,0,sizeof(buffer));
    memset(str,0,MAX_SZ);
    while(bytes_recvd < size)
    {
		FD_ZERO(&socks);
		FD_SET(sock,&socks);
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		readsock = select((int)sock+1,&socks,NULL,NULL,&timeout);
		if(readsock <= 0)
    		return(0);
		if(FD_ISSET(sock,&socks))
		{
			t = recv(sock,str,size, 0);
           	if(t > 0)
			{
            	memcpy(buffer+bytes_recvd,str,t);
               	bytes_recvd += t;
               	if(bytes_recvd >= size)
               		break;
			}
		}
	}
	return(bytes_recvd);
}
/**************************************************************************************/
