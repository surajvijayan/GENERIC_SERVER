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
 * \file sample_client.cpp
 * \brief This is a sample client program that interfaces with GENERIC_SERVER framework.
 * 
 * \author Suraj Vijayan
 *
 * \date : 2011/12/23 12:16:20
 *
 * Contact: suraj@broadcom.com
 *
 */
#include "util.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
/***********************************************************************************/

int main(int argc, char *argv[])
{
    char buff[MAX_SZ],read_buff[MAX_SZ];
	SOCKET sock;
    unsigned int portno,size,sess_type;
	int n,i,no_req;
	string plugin_name,message;
	SSL *ssl_client;
	SSL_CTX *ctx;
	ssl_client = NULL;
	ctx = NULL;

#ifdef WINDOWS
	sock_init();
#endif
	memset(buff,0,MAX_SZ);
    if (argc < 7) 
       error("Usage: sample_client <ip_address> <port> <plugin_name> <message> <no_requests> <1=SSL|0=RAW>");
    portno = atoi(argv[2]);
	plugin_name = string(argv[3]);
	message = string(argv[4]);
    no_req = atoi(argv[5]);
    sess_type = atoi(argv[6]);
	if(sess_type == 1)
	{
		if(!ssl_init(&ctx))
			error("ERROR SSL init");
		if(!ssl_connect((unsigned char *) argv[1], portno,sock,&ssl_client,&ctx))
			error("ERROR SSL connecting");
	}
	else
	{
		if(!sock_connect((unsigned char *)argv[1],portno,sock))
       		error("ERROR connecting");
	}
	setblocking(sock);
	memset(buff,0,MAX_SZ);
	size = create_message(plugin_name,message,buff);
	for(i=0; i < no_req; i++)
    {
		if(sess_type == 1)
			n = SSL_write(ssl_client, buff,size);
		else
			n = send(sock,buff,size,0);
		if (n < 0) 
			error("write ERROR ..");
		if(sess_type == 1)
			n = ssl_read_socket((unsigned char *)read_buff,sock,&ssl_client);
		else
			n = read_socket((unsigned char *)read_buff,sock);
		if (n < 0) 
			error("read ERROR..");
		cout << "Ctr: " << i << " " << string(read_buff) << endl;
		if(!memcmp(read_buff,"00 TERMINATE",12))
			break;
    }
	if(sess_type == 1)
		while((SSL_shutdown(ssl_client)) == 0);
#ifdef WINDOWS
	closesocket(sock);
#else
    close(sock);
#endif
    return 0;
}
/***********************************************************************************/

