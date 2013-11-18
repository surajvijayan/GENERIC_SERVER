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
#ifdef WINDOWS
#include <winsock2.h>
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
#include "util.h"

int ssl_read_socket(unsigned char *buffer,SOCKET &sock,SSL **ssl_client)
{
    char buf[MAX_SZ];
    fd_set socks;
    int r;
    unsigned int plugin_signature,no_bytes,msg_length;
    struct timeval timeout;
   
	memset(buf,0,MAX_SZ); 
	memset(buffer,0,MAX_SZ);
	FD_ZERO(&socks);
    FD_SET (sock, &socks);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    r = select((int)sock+1, &socks, NULL, NULL, &timeout);
    if (r <= 0)
        return(0);
    else if (r > 0 && FD_ISSET(sock, &socks))
    {
        r = ssl_read((SSL *)*ssl_client,buf,MSG_HDR_SIZE);   // First 4 bytes represent plugin SIGNATURE
		if(r <= 0)
        	return(0);
        memcpy(&plugin_signature,buf,4); 			 		// Next 4 byes represent msg length
        memcpy(&msg_length,buf+4,4);
        msg_length = ntohl(msg_length);
        no_bytes = 0;
        while(no_bytes < msg_length)
        {
            r = ssl_read((SSL *)*ssl_client,buf,msg_length);
            if(r <= 0)
                return(0);
            memcpy(buffer+no_bytes,buf,r);
            no_bytes += r;
        }
    }
    return(no_bytes);

}
/**************************************************************************************/

int ssl_read(SSL *ssl,char *buf,int size)
{
	int len,r;

	while(1)
	{
		r = SSL_read(ssl,buf,size);
		switch(SSL_get_error(ssl,r))
		{
			case SSL_ERROR_NONE:
				len=r;
				break;
			case SSL_ERROR_WANT_READ:
				continue;
			case SSL_ERROR_ZERO_RETURN:
				return(0);
			case SSL_ERROR_SYSCALL:
				return(0);
			default:
				return(0);
		}
		if(len == r)
			break;
	}
	return(len);
}
/**************************************************************************************/

int read_socket(unsigned char *buffer,SOCKET conn_s)
{
	unsigned int plugin_signature,msg_length;
    fd_set socks;
    struct timeval timeout;
    char str[10000];
    size_t readsock,t;
    size_t bytes_recvd;
    bytes_recvd = 0;

    memset(buffer,0,sizeof(buffer));
    memset(str,0,10000);

    FD_ZERO(&socks);
    FD_SET(conn_s,&socks);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    readsock = select((int)conn_s+1,&socks,NULL,NULL,&timeout);
    if(readsock <= 0)
    	return(0);
    if(FD_ISSET(conn_s,&socks))
    {
    	t = recv(conn_s,str,MSG_HDR_SIZE, 0);	 // First 4 bytes represent plugin SIGNATURE
		if(t < MSG_HDR_SIZE)
			return(0);
       	memcpy(&plugin_signature,str,4); 		 // Next 4 byes represent msg length
       	memcpy(&msg_length,str+4,4);
       	msg_length = ntohl(msg_length);
       	bytes_recvd = 0;
       	while(bytes_recvd < msg_length)
       	{
			t = recv(conn_s,str,10000, 0);
           	if(t > 0)
			{
            	memcpy(buffer+bytes_recvd,str,t);
               	bytes_recvd += t;
               	if(bytes_recvd >= msg_length)
               		break;
			}
			else
           		break;
		}
	}
	return(bytes_recvd);
}
/**************************************************************************************/
/*
    Open socket to the given IP Address and Port
    returns 0 for success
*/
int ssl_connect(unsigned char *ip_address, unsigned int port_number,SOCKET &sock,SSL **ssl_client,SSL_CTX **ctx)
{
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        cout << "Failed to create socket: " << sock << endl;
        return(0);
    }
	setblocking(sock);
    sockaddr.sin_family = AF_INET;                               /* Internet/IP */
    sockaddr.sin_addr.s_addr = inet_addr((const char *)ip_address);            /* IP address */
    sockaddr.sin_port = (u_short)htons((u_short)port_number);    /* server port */
    int i =  connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (i == SOCKET_ERROR)
    {
        cout << "Failed to connect with server: " << i << " " << ip_address << " at port: " << port_number << endl;
        return(0);
    }
    *ssl_client = SSL_new(*ctx);
    SSL_set_fd(*ssl_client,sock);
    SSL_set_connect_state(*ssl_client);
//  SSL_set_verify(ssl,SSL_VERIFY_PEER,NULL);
    if(SSL_connect(*ssl_client) <= 0)
    {
        cout << "Failed to successfully connect to SSL server: " << ip_address << " at port: " << port_number;
        return(0);
    }
    return(1);
}
/*****************************************************************************************************/

/**
 * OpenSSL uniq id function.
 *
 * @return    thread id
 */
unsigned long id_function(void)
{
#ifdef WINDOWS
    return(unsigned long) GetCurrentThreadId();
#else
    return ((unsigned long) pthread_self());
#endif
}
/**********************************************************************************************/

struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{
    struct CRYPTO_dynlock_value *value;

    value = NULL;
    value = (struct CRYPTO_dynlock_value *) malloc(sizeof(struct CRYPTO_dynlock_value));
    if (!value)
        return(NULL);
#ifdef WINDOWS
    value->mutex = CreateMutex( NULL, FALSE, NULL );
#else
    pthread_mutex_init(&value->mutex, NULL);
#endif
    return value;
}
/**********************************************************************************************/

void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
#ifdef WINDOWS
        DWORD dwWaitResult;
        dwWaitResult = WaitForSingleObject(l->mutex,INFINITE);
#else
        pthread_mutex_lock(&l->mutex);
#endif
    }
    else
    {
#ifdef WINDOWS
        ReleaseMutex(l->mutex);
#else
        pthread_mutex_unlock(&l->mutex);
#endif
    }
}
/**********************************************************************************************/

void dyn_destroy_function(struct CRYPTO_dynlock_value *l,const char *file, int line)
{
#ifdef WINDOWS
    CloseHandle(l->mutex);
#else
    pthread_mutex_destroy(&l->mutex);
#endif
    free(l);
}
/**********************************************************************************************/
/*
    Open socket to the given IP Address and Port
    returns 0 for success
*/
int sock_connect(unsigned char *ip_address, unsigned  int port_number,SOCKET &sock)
{
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        cout << "Failed to create socket: " << sock << endl;
        return(0);
    }
    sockaddr.sin_family = AF_INET;                               /* Internet/IP */
    sockaddr.sin_addr.s_addr = inet_addr((const char *)ip_address);            /* IP address */
    sockaddr.sin_port = (u_short)htons((u_short)port_number);    /* server port */
    int i =  connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if ( i == SOCKET_ERROR)
    {
        cout << "Failed to connect with server: " << i << " " << ip_address << " at port: " << port_number << endl;
        return(0);
    }
	return(1);
}
/**********************************************************************************************/

void locking_function(int mode, int n, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
#ifdef WINDOWS
        DWORD dwWaitResult;
        dwWaitResult = WaitForSingleObject(mutex_buf[n],INFINITE);
#else
        pthread_mutex_lock(&mutex_buf[n]);
#endif
    }
    else
    {
#ifdef WINDOWS
        ReleaseMutex(mutex_buf[n]);
#else
        pthread_mutex_unlock(&mutex_buf[n]);
#endif
    }
}
/**********************************************************************************************/

int setnonblocking(int sock)
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

int setblocking(int sock)
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

#ifdef WINDOWS
int  sock_init(void)
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

int create_message(string plugin_name,string message,char *buff)
{
int size,tot_size;
unsigned int plugin_signature;

	plugin_signature = htonl(PLUGIN_SIGNATURE);
	memset(buff,0,MAX_SZ);		
	size = plugin_name.length();
	size = htonl(size);
	memcpy(buff,&plugin_signature,4);
	tot_size = plugin_name.length()+message.length()+4;		
	tot_size = htonl(tot_size);
	memcpy(buff,&plugin_signature,4); // Adding SIGNATURE
	memcpy(buff+4,&tot_size,4); 	  // Adding total size
	memcpy(buff+8,&size,4);			  // Adding plugin_name size
	memcpy(buff+12,plugin_name.data(),plugin_name.length());				// Addling plugin_name
	memcpy(buff+12+plugin_name.length(),message.data(),message.length()); 	// Adding message payload
	return(12+plugin_name.length()+message.length());
}
/*****************************************************************************************/

int create_message(string plugin_name,unsigned char req_type,string message,char *buff)
{
int size,tot_size;
unsigned int plugin_signature;

    plugin_signature = htonl(PLUGIN_SIGNATURE);
    memset(buff,0,MAX_SZ);
    size = plugin_name.length();
    size = htonl(size);
    memcpy(buff,&plugin_signature,4);
    tot_size = plugin_name.length()+message.length()+4;
    tot_size += 1;					  // adding 1 byte for req_type
    tot_size = htonl(tot_size);
    memcpy(buff,&plugin_signature,4); // Adding SIGNATURE
    memcpy(buff+4,&tot_size,4);       // Adding total size
    memcpy(buff+8,&size,4);           // Adding plugin_name size
    memcpy(buff+12,plugin_name.data(),plugin_name.length());               // Adding plugin_name
    memcpy(buff+12+plugin_name.length(),&req_type,1);       				// Adding req_type
    memcpy(buff+13+plugin_name.length(),message.data(),message.length());  // Adding message payload
    return(13+plugin_name.length()+message.length());
}
/*****************************************************************************************/

int ssl_init(SSL_CTX **ctx)
{
	int i;
#if (OPENSSL_VERSION_NUMBER >= 0x10000000L) // openssl returns a const SSL_METHOD
	const SSL_METHOD *method = NULL;
#else
	SSL_METHOD *method = NULL;
#endif
	SSL_load_error_strings();
   	SSL_library_init();
   	OpenSSL_add_all_algorithms();
#ifdef WINDOWS
	mutex_buf = (HANDLE *) malloc(CRYPTO_num_locks() * sizeof(HANDLE));
#else
	mutex_buf = (pthread_mutex_t *) malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
#endif
	if (mutex_buf == NULL)
		return (0);
	for (i = 0; i < CRYPTO_num_locks(); i++)
	{
#ifdef WINDOWS
		mutex_buf[i] = CreateMutex( NULL, FALSE, NULL );
#else
		pthread_mutex_init(&mutex_buf[i], NULL);
#endif
	}
	/* static locks callbacks */    	
	CRYPTO_set_locking_callback(locking_function);
    CRYPTO_set_id_callback(id_function);
    /* dynamic locks callbacks */
    CRYPTO_set_dynlock_create_callback(dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
#ifndef WINDOWS
	RAND_load_file("/dev/urandom", MAX_SZ);
#endif
	// Compatible with SSLv2, SSLv3 and TLSv1
	method = SSLv3_client_method();
	// Create new context from method.
	*ctx = SSL_CTX_new(method);
	if(*ctx == NULL)
	{
		cout << "CTX is NULL.." << endl;
		return(0);
	}
	SSL_CTX_set_mode(*ctx,SSL_MODE_AUTO_RETRY);		
	return(1);
}
/*****************************************************************************************/
