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
 * \file util.h
 * \brief Bunch of utility functions used by sample_client program
 *
 * \author Suraj Vijayan
 *
 * \date : 2011/12/23 12:16:20
 *
 * Contact: suraj@broadcom.com
 *
 */

#ifndef UTIL_H
#define UTIL_H
#ifdef WINDOWS
#include <ws2tcpip.h>
#include <conio.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <utility>
using namespace std;

struct CRYPTO_dynlock_value
{
#ifdef WINDOWS
    HANDLE mutex;
#else
    pthread_mutex_t mutex;
#endif
};
#ifdef WINDOWS
int  sock_init(void);
static HANDLE *mutex_buf;
#else
static pthread_mutex_t *mutex_buf;
#endif
#ifndef WINDOWS
#define SOCKET int
#define SOCKET_ERROR -1
#endif
#define PLUGIN_SIGNATURE 0xDEDDAECF
#define MSG_HDR_SIZE 8
#define MAX_SZ 10000
void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,const char *file, int line);
void dyn_destroy_function(struct CRYPTO_dynlock_value *l,const char *file, int line);
struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line);
void locking_function(int mode, int n, const char *file, int line);
unsigned long id_function(void);
int ssl_read_socket(unsigned char *line,SOCKET &sock,SSL **ssl_client);
int read_socket(unsigned char *buffer,SOCKET sock);
int ssl_connect(unsigned char *ip_address, unsigned int port_number,SOCKET &sock,SSL **ssl_client,SSL_CTX **ctx);
int sock_connect(unsigned char *ip_address, unsigned int port_number,SOCKET &sock);
/**
*   Initialize TLS. Setup CRYPTO locking callbacks,RNG,mutuxes.
*/
int ssl_init(SSL_CTX **ctx);
int ssl_read(SSL *ssl,char *buf,int size);
int setnonblocking(int sock);
int setblocking(int sock);
/**
*   Construct a GENERIC_SERVER message for a plug-in
*
*	This function will add appropiate headers and construct a GENERIC_SERVER compliant message
*   \param[in]  plug_name to be used for the message
*   \param[in]  message to be created
*   \return     1, will always succeed
*/
int create_message(string plugin_name,string message,char *buff);
int create_message(string plugin_name,unsigned char req_type,string message,char *buff);
#endif
