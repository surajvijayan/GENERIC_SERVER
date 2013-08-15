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
 * \file control.h
 * \brief Master program that initiates GENERIC_SERVER,
 * spawns a thread for each plug-in and runs message loop for that plug-in.
 *
 * \author Suraj Vijayan
 *
 * \date : 2013/07/01 14:16:20 
 *    
 * Contact: suraj@broadcom.com
 *
 */ 
#ifndef CTRL_H
#define CTRL_H

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
#ifndef WINDOWS
#include <pthread.h>
#else
#define MSG_NOSIGNAL 0
#endif
#include "sslserver.h"
#define TLS_REQ		1
#define SOCKET_REQ	2

void sig_int(int sig_num);
void do_nothing(int sig_num);

#ifdef WINDOWS
unsigned  _stdcall thread_fn(void *inp_arg);
unsigned  _stdcall ssl_thread_fn(void *inp_arg);
BOOL CtrlHandler( DWORD fdwCtrlType );
#else
void  *thread_fn(void *inp_arg);
void  *ssl_thread_fn(void *inp_arg);
void daemonize();
#define PR_SET_NAME 15
#endif
unsigned int get_data_size(unsigned char *recvbuf);
int add_size(unsigned char *recvbuf,unsigned int size);
string get_input_plugin_name(char *buffer);
/**
* This function gets called in a plug-in specific thread to process request
* sent by client on raw (non TLS) session. Once a valid request is received, 'process_request()' method
* on plug-in is invoked. Message structure:
*\li SIGNATURE        - 4 bytes SIGNATURE for all messages
*\li PAYLOAD_SIZE     - 4 bytes network byte order 
*\li DATA             - PAYLOAD_SIZE data
*\li Data payload could be:
*\li PLUGIN_NAME_LEN  - 4 bytes network byte order
*\li PLUGIN_NAME      - Size as defined by <PLUGIN_NAME_LEN>
*\li APPLICATION_DATA - PAYLOAD_SIZE-(4+PLUGIN_NAME_LEN)
*
* \param[in]   clnt_sock       Client socket to read/write
* \param[in]   plugin_init_state  Flag to indicate return status of plugin_init() callback
* \param[in]   plugin_p        Pointer to plug-in object
* \param[in]   thread_no       Thread number assigned by framework
* \param[in]   framework            Pointer to framework object
* \return 1 on success, 0 on failure
*/
static int process_client_req(SOCKET clnt_sock,int plugin_init_state,GENERIC_PLUGIN *plugin_p,
                              int thread_no,generic_server *framework);
/**
* This function gets called in a plug-in specific thread to process request
* sent by client on TLS enabled session. Once a valid request is received, 'process_request()' method
* on plug-in is invoked. Message structure:
*\li SIGNATURE        - 4 bytes SIGNATURE for all messages
*\li PAYLOAD_SIZE     - 4 bytes network byte order 
*\li DATA             - PAYLOAD_SIZE data
*\li Data payload could be:
*\li PLUGIN_NAME_LEN  - 4 bytes network byte order
*\li PLUGIN_NAME      - Size as defined by <PLUGIN_NAME_LEN>
*\li APPLICATION_DATA - PAYLOAD_SIZE-(4+PLUGIN_NAME_LEN)
*
* \param[in]   clnt_sock       Client socket attached to SSL
* \param[in]   ssl             Pointer to SSL structure
* \param[in]   plugin_init_state  Flag to indicate return status of plugin_init() callback
* \param[in]   thread_no       Thread number assigned by framework
* \param[in]   framework            Pointer to framework object
* \return 1 on success, 0 on failure
*/
static int process_ssl_client_req(SOCKET clnt_sock,SSL *ssl,int plugin_init_state,GENERIC_PLUGIN *plugin_p,
                                  int thread_no,generic_server *framework);

/**
* This function gets called in a plug-in specific thread whenever a
* non-TLS client session is to be terminated.
*
* \param[in]   clnt_socket     Client socket to read/write
* \param[in]   framework            Pointer to framework object
* \param[in]   plugin_p        Pointer to plugin object
* \param[in]   thread_no       Thread number assigned by framework
* \param[in]   sendbuf         NULL terminated ASCII data to send to client
* \return 1 always
*/
static int terminate_client_session(SOCKET clnt_sock,generic_server *framework, GENERIC_PLUGIN *plugin_p,
                                    int thread_no,unsigned char *senduf);

/**
* This function gets called in a plug-in specific thread whenever 
* a TLS client session is to be terminated.
*
* \param[in]   ssl             Pointer to SSL structure
* \param[in]   framework            Pointer to framework object
* \param[in]   plugin_p        Pointer to plugin object
* \param[in]   thread_no       Thread number assigned by framework
* \param[in]   sendbuf         NULL terminated ASCII data to send to client
* \returns 1 always
*/
static int terminate_ssl_client_session(SSL *ssl,generic_server *framework, GENERIC_PLUGIN *plugin_p,
                                        int thread_no,unsigned char *senduf);   
static int print_source_hostname(int clnt_sock,int req_type,generic_server *framework,int thread_no,
                                 GENERIC_PLUGIN *plugin_p);
/// Main loop to 'select()' sockets from multiple clients.
static int process_data(void);
static unsigned int init_sock_fds(fd_set *,SOCKET);
static int chk_command_sock(fd_set *,SOCKET);
/**
* Launches a new thread to run a plug-in. Thread could be for an TLS session or
* a non-TLS session.
*
* \param[in]   plugin_p        Pointer to generic_plugin object
* \param[in]   new_slot        slot position into thread_ids map 
* \return 1 always
*/
static int launch_thread(GENERIC_PLUGIN *generic_plugin,unsigned int new_slot);
static int chk_all_clients(fd_set *socks);
static int read_socket(unsigned char *buffer,SOCKET sock,int size);
#endif 
