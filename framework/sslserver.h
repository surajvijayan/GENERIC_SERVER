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
 * \file sslserver.h
 * \brief Utility class for TLS.
 * Please refer excellent documentation at:
 * http://www.openssl.org/docs/ssl/ssl.html
 *
 * \author Suraj Vijayan
 *
 * Contact: suraj@broadcom.com
 *
 */

#ifndef WINDOWS
#include <pthread.h>
#
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <unistd.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#ifndef __SSL_SERVER_H__
#define __SSL_SERVER_H__

using namespace std;
/// Utility class for TLS. Please refer excellent documentation at: http://www.openssl.org/docs/ssl/ssl.html.
class SSLServer 
{
  // Private data
  private:
   SSL_CTX *ctx;
   string cert_file;
   string priv_key_file;
   string ca_certificate;
   bool verify_client;
   
  public:
   /// Constructors
   SSLServer();
   SSLServer(char *cFile, char *kFile);
   int set_priv_key(string priv_key);
   int set_ca_cert(string ca_cert);
   int set_cert_file(string server_cert_file);
   int set_verify_client(bool status_flag);
   /// Load algorithms and create context.
   int CreateCTX(void);
   /// Load certification files.
   int LoadCerts(void);
   SSL_CTX *get_ctx(void);
/**
*   Initialize TLS. Setup CRYPTO locking callbacks,RNG,mutuxes.
*/
   int tls_init(void);
   int tls_cleanup(void);
};

#endif
