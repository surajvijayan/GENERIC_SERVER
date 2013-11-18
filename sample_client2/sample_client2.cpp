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
 * \file sample_client2.cpp
 * \brief This is a sample client program that interfaces with GENERIC_SERVER framework.
 * 
 * \author Suraj Vijayan
 *
 * Contact: suraj@broadcom.com
 *
 */
#include "util.h"
#define SET_NAME  1
#define FILE_DATA 2
#define CLOSE_FILE 3
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
	int n;
	string plugin_name,message,fname;
	SSL *ssl_client;
	SSL_CTX *ctx;
	ssl_client = NULL;
	ctx = NULL;

#ifdef WINDOWS
	char win_fname[1024],win_ext[10];
	sock_init();
	memset(win_fname,0,1024);
	memset(win_ext,0,10);
#endif
	memset(buff,0,MAX_SZ);
    if (argc < 6) 
       error("Usage: sample_client2 <ip_address> <port> <plugin_name> <filename> <1=SSL|0=RAW>");
    portno = atoi(argv[2]);
	plugin_name = string(argv[3]);
	fname = string(argv[4]);
    sess_type = atoi(argv[5]);
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
	// send filename to server
#ifdef WINDOWS
	 _splitpath_s((char *)fname.c_str(), NULL,0, NULL,0, win_fname,1024, win_ext,10);
	 strcat_s(win_fname,1024,win_ext);
    size = create_message(plugin_name,SET_NAME,win_fname,buff);
#else
	size = create_message(plugin_name,SET_NAME,basename((char *)fname.c_str()),buff);
#endif
    if(sess_type == 1)
        SSL_write(ssl_client, buff,size);
    else
        send(sock,buff,size,0);

	// open file and read in data
	ifstream ifs(fname.c_str(), std::ios::binary);
	while(!ifs.eof()) 
	{
		ifs.read(buff, MAX_SZ-(2*MSG_HDR_SIZE));
		message = string(buff,(unsigned int)ifs.gcount());
		// send 1 block of data to server
		size = create_message(plugin_name,FILE_DATA,message,buff);
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

	}
	// close file and let server know
	ifs.close();
	message = string("TERMINATE");
	size = create_message(plugin_name,CLOSE_FILE,message,buff);
	if(sess_type == 1)
	{
       	SSL_write(ssl_client, buff,size);
		while((SSL_shutdown(ssl_client)) == 0);
	}
	else
        send(sock,buff,size,0);
#ifdef WINDOWS
	closesocket(sock);
#else
    close(sock);
#endif
    return 0;
}
/***********************************************************************************/

