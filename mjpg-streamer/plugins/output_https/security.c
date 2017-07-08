#include "security.h"
#include "../../mjpg_streamer.h"

/* Array of thread lock needed by OpenSSL library to handle SSL connections */
static MUTEX_TYPE *mutex_buf = NULL;

/* Callback function that handles the lock procedure. Required (and called) by OpenSSL library */
static void locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK){
        MUTEX_LOCK(mutex_buf[n]);
    }
    else{
        MUTEX_UNLOCK(mutex_buf[n]);
    }
}

/* Callback function that returns the thread's ID */
static unsigned long id_function(void)
{
    return ((unsigned long)THREAD_ID);
}

/* Callback function that possibly asks the user to prompt the private key file's password, so as to decrypt and load it into the context */
int pem_passwd_cb(char *buf, int size, int rwflag, void *userdata){
 	int ret;
 	char format_string[10];				//10 chars are enough to store the format string
 	printf("Enter your password for private key (max length = %i letters): ", size-1);
 	//We want to read at most <size-1> letters from the password, in order to prevent the scanf function from causing buffer overflow
 	sprintf(format_string ,"%%%is", size-1);			//Here we are building the format string: %<size-1>s
	ret = scanf(format_string, buf);
	if(ret != 1){
		fprintf(stderr, "scanf() failed in reading the password\n");
	}	
	return(strlen(buf));
 }
 

/******************************************************************************
Description.: Create the SSL context, from which will be possible to 
	      instantiate SSL sockets.
Input Value.: * cert_file..: path of the public certificate file
              * prvkey_file: path of the private key file
Return Value: pointer to data structure representing the SSL context
******************************************************************************/
SSL_CTX* create_SSL_context(const char* cert_file, const char* prvkey_file){
	SSL_CTX* ctx;
	int error;
	int i;
	
	SSL_library_init();
	
	SSL_load_error_strings();
	
	ctx = SSL_CTX_new(SSLv23_method());

    	if (ctx == NULL) {
        	fprintf(stderr, "Can't initialize SSL_CTX\n");
        	return NULL;
    	}
    	
    	SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS); 	
    	
    	SSL_CTX_set_cipher_list(ctx, SSL_CIPHERS);
    	
    	error = SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM);
    	if(error != 1){
    		fprintf(stderr, "Can't load public certificate: %s\n", ERR_error_string(ERR_get_error(), NULL));
        	return NULL;
    	}
    	
    	DBG("OPENSSL: Server certificate file loaded\n");
    	
    	SSL_CTX_set_default_passwd_cb(ctx,pem_passwd_cb);
    	
    	error = SSL_CTX_use_PrivateKey_file(ctx, prvkey_file, SSL_FILETYPE_PEM);
    	if(error != 1){
    		fprintf(stderr, "Can't load private key, wrong password maybe?\n");
        	return NULL;
    	}
    	
    	DBG("OPENSSL: Server private key file loaded\n");
    	printf("OPENSSL: private key and public certificate loaded\n");
    	error = SSL_CTX_check_private_key(ctx);
    	if(error != 1){
    		fprintf(stderr, "Private key does not match the public certificate.\n");
        	return NULL;
    	}
    	//non è richiesta l'autenticazione del client (visto che agiremo sul web)
    	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL); 
    	
    	//alloco l'array di locks
    	mutex_buf = malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
    	
    	//inizializzo i vari lock
    	for (i = 0;  i < CRYPTO_num_locks();  i++)
        	MUTEX_SETUP(mutex_buf[i]);
    	
    	//setto le due funzioni di callback per la gestione dei lock alla libreria openssl
    	CRYPTO_set_id_callback(id_function);
        CRYPTO_set_locking_callback(locking_function);
    	
    	return ctx;
}


/******************************************************************************
Description.: Bind a traditional TCP socket to the SSL context. As a result 
	      this function creates the so-called SSL socket.
Input Value.: * ctx.: pointer to the SSL context 
              * sock: system socket (file descriptor)
Return Value: pointer to data structure representing the SSL socket
******************************************************************************/
SSL* bind_socket_to_SSL(SSL_CTX* ctx, int sock){
	SSL* ssl_sock;
	int error;
	if(ctx == NULL || sock < 0){
		fprintf(stderr, "Error during the SSL socket bind: invalid arguments\n");
        	return NULL;
	}
	
	ssl_sock = SSL_new(ctx);
	if(ssl_sock == NULL){
		fprintf(stderr, "Error during the SSL socket creation\n");
        	return NULL;
	}
	
	error = SSL_set_fd(ssl_sock, sock);
	if(error != 1){
		fprintf(stderr, "Error during the SSL socket creation\n");
        	return NULL;	
	}
	error = SSL_accept(ssl_sock);
	if(error != 1){
		fprintf(stderr, "Error during the SSL handshake: %s\n", ERR_error_string(ERR_get_error(), NULL));
		SSL_free(ssl_sock);
        	return NULL;
	}
	return ssl_sock;
}

/******************************************************************************
Description.: Read at most <count> bytes from the SSL socket or, if it 
	      is NULL, from the TCP socket.  
Input Value.: * fd....: TCP socket
	      * buffer: buffer where to store the data
              * count.: number of bytes to read
              * sock..: SSL socket
Return Value: number of bytes read.
******************************************************************************/
int secure_read(int fd, void* buffer, int count, SSL* sock){
	int ret;
	//printf("secure_read\n");
	if(sock == NULL)
		return read(fd, buffer, count);
	else{
		ret = SSL_read(sock, buffer, count);
		//printf("ret = %i content =\n%s", ret, buffer);
		return ret;
	}
}

/******************************************************************************
Description.: Write at most <count> bytes on the SSL socket or, if it 
	      is NULL, on the TCP socket.  
Input Value.: * fd....: TCP socket
	      * buffer: buffer containing data to write
              * count.: number of bytes to write
              * sock..: SSL socket
Return Value: number of bytes written.
******************************************************************************/
int secure_write(int fd, void* buffer, int count, SSL* sock){
	//printf("secure_write\n");
	if(sock == NULL)
		return write(fd, buffer, count);
	else
		return SSL_write(sock, buffer, count);
}

/******************************************************************************
Description.: Clean and free all data structures allocated and used by the 
	      SSL context. 
Input Value.: * ctx: SSL context to cleanup
Return Value: -
******************************************************************************/
void ssl_context_cleanup(SSL_CTX* ctx){
	int i;
	if(ctx == NULL)
		return;
	SSL_CTX_free(ctx);
	CRYPTO_set_id_callback(NULL);
    	CRYPTO_set_locking_callback(NULL);
    	for (i = 0;  i < CRYPTO_num_locks();  i++)
       		MUTEX_CLEANUP(mutex_buf[i]);
    	free(mutex_buf);
    	return;
}

