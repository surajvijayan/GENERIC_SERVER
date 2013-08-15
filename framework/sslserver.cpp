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
#include "sslserver.h"

/// Struct to store OS agnostic mutex. Required for Openssl locking functions.
struct CRYPTO_dynlock_value
{
#ifdef WINDOWS
    HANDLE mutex;
#else
    pthread_mutex_t mutex;
#endif
};
#ifdef WINDOWS
static HANDLE *mutex_buf;
#else
static pthread_mutex_t *mutex_buf;
#endif
static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,const char *file, int line);
static void dyn_destroy_function(struct CRYPTO_dynlock_value *l,const char *file, int line);
static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line);
static void locking_function(int mode, int n, const char *file, int line);
static unsigned long id_function(void);
SSLServer::SSLServer() 
{
	cert_file = "";
	priv_key_file = "";
	ctx = NULL;
}
/**********************************************************************************************/

SSLServer::SSLServer(char *cFile, char *kFile) 
{
	cert_file = cFile;
	priv_key_file = kFile;
	ctx = NULL;
	mutex_buf = NULL;
}
/**********************************************************************************************/

SSL_CTX *SSLServer::get_ctx(void)
{
	return(ctx);
}
/**********************************************************************************************/

int SSLServer::tls_init(void)
{
    int i;

	SSL_load_error_strings();
	SSL_library_init();
    // Load algorithms and error strings.
	OpenSSL_add_all_algorithms();
    /* static locks area */
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
    RAND_load_file("/dev/urandom", 1024);
//  OpenSSL uses several sources of entropy automatically on Windows 
#endif
    return (1);
}
/**********************************************************************************************/

int SSLServer::tls_cleanup(void)
{
    int i;

    if (mutex_buf == NULL)
        return (0);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);

    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
#ifdef WINDOWS
        CloseHandle(mutex_buf[i]);
#else
        pthread_mutex_destroy(&mutex_buf[i]);
#endif
    }
    free(mutex_buf);
    mutex_buf = NULL;
    return (1);
}
/**********************************************************************************************/

int SSLServer::set_priv_key(string priv_key)
{
	priv_key_file = priv_key;
	return(1);
}
/**********************************************************************************************/

int SSLServer::set_cert_file(string server_cert_file)
{
	cert_file = server_cert_file;
	return(1);
}
/**********************************************************************************************/

int SSLServer::set_verify_client(bool status_flag)
{
        verify_client = status_flag;
        return(1);
}
/**********************************************************************************************/

int SSLServer::set_ca_cert(string ca_cert)
{
	ca_certificate = ca_cert;
	return(1);
}
/**********************************************************************************************/

int SSLServer::CreateCTX(void) 
{
// The method describes which SSL protocol we will be using.
#if (OPENSSL_VERSION_NUMBER >= 0x10000000L) // openssl returns a const SSL_METHOD
	const SSL_METHOD *method = NULL;
#else
	SSL_METHOD *method = NULL;
#endif
	STACK_OF(X509_NAME) *cert_names;

	// Compatible with SSLv2, SSLv3 and TLSv1
	method = SSLv23_server_method();
	//method = TLSv1_server_method();

	// Create new context from method.
	ctx = SSL_CTX_new(method);
	if(ctx == NULL) 
		return(0);
	SSL_CTX_set_mode(ctx,SSL_MODE_AUTO_RETRY);
	cert_names = SSL_load_client_CA_file(ca_certificate.c_str());
	if (cert_names != NULL)
		SSL_CTX_set_client_CA_list(ctx, cert_names);
 	else
		return(0);
	if(!SSL_CTX_load_verify_locations(ctx,ca_certificate.c_str(),NULL))
	if(verify_client)
		SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,0);
	return(1);
}
/**********************************************************************************************/

/* Load the certification files, ie the public and private keys. */
int SSLServer::LoadCerts(void) 
{
	if ( SSL_CTX_use_certificate_chain_file(ctx, cert_file.c_str()) <= 0) 
		return(0);
	if ( SSL_CTX_use_PrivateKey_file(ctx, priv_key_file.c_str(), SSL_FILETYPE_PEM) <= 0) 
		return(0);
	// Verify that the two keys goto together.
	if ( !SSL_CTX_check_private_key(ctx) ) 
		return(0);
	return(1);
}
/**********************************************************************************************/
/**
 * OpenSSL locking function.
 *
 * @param    mode    lock mode
 * @param    n        lock number
 * @param    file    source file name
 * @param    line    source file line number
 * @return   none
 */
static void locking_function(int mode, int n, const char *file, int line)
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

/**
 * OpenSSL uniq id function.
 *
 * @return    thread id
 */
static unsigned long id_function(void)
{
#ifdef WINDOWS
	return(unsigned long) GetCurrentThreadId();
	//return(1);
#else
    return ((unsigned long) pthread_self());
#endif
}
/**********************************************************************************************/

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
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

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,const char *file, int line)
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

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l,const char *file, int line)
{
#ifdef WINDOWS
	CloseHandle(l->mutex);
#else
    pthread_mutex_destroy(&l->mutex);
#endif
    free(l);
}
/**********************************************************************************************/
