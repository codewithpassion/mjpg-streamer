#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define MUTEX_TYPE            pthread_mutex_t
#define MUTEX_SETUP(x)        pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x)      pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)         pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)       pthread_mutex_unlock(&(x))
#define THREAD_ID             pthread_self()

#define SSL_CIPHERS "ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+3DES:!aNULL:!MD5:!DSS"
//#define SSL_CIPHERS "DHE-DSS-AES128-SHA256"

SSL_CTX* create_SSL_context(const char* cert_file, const char* prvkey_file);
SSL* bind_socket_to_SSL(SSL_CTX* ctx, int sock);
int secure_read(int fd, void* buffer, int count, SSL* sock);
int secure_write(int fd, void* buffer, int count, SSL* sock);
void ssl_context_cleanup(SSL_CTX* ctx);
