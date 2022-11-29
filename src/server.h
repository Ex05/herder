#ifndef SERVER_H
#define SERVER_H

#include "http.h"
#include "linkedList.h"
#include "threadPool.h"
#include "util.h"
#include "properties.h"
#include "resources.h"
#include "constants.h"

#include <netdb.h>
#include <openssl/crypto.h>
#include <openssl/ossl_typ.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
#include <signal.h>

#define SERVER_SSL_ERROR_STRING_BUFFER_LENGTH 2048

#define SERVER_GET_SSL_ERROR_STRING(name) char name[SERVER_SSL_ERROR_STRING_BUFFER_LENGTH]; \
ERR_error_string_n(ERR_get_error(), name, SERVER_SSL_ERROR_STRING_BUFFER_LENGTH);

typedef struct {
	LinkedList contexts;
	PropertyFile properties;
	SSL_CTX* sslContext;
	int socketFileDescriptor;
	int epollAcceptFileDescriptor;
	int epollClientHandlingFileDescriptor;
	ThreadPool epollWorkerThreads;
	sem_t running;
	Cache errorPageCache;
	Cache cache;
	Property* workDirectory;
	Property* httpRootDirectory;
	Property* customErrorPageDirectory;
}Server;

#define SERVER_CONTEXT_HANDLER(functionName) ERROR_CODE functionName(Server* server, HTTP_Request* request, HTTP_Response* response)
typedef SERVER_CONTEXT_HANDLER(ContextHandler);

typedef struct{
	char* symbolicFileLocation;
	uint_fast64_t symbolicFileLocationLength;
	ContextHandler* contextHandler;
}Context;

#define SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION(varName, server, symbolicFileLocation, symbolicFileLocationLength) char* varName; \
	uint_fast64_t varName ## Length; \
	do{ \
		varName ## Length = server->httpRootDirectory->dataLength + symbolicFileLocationLength; \
		varName = alloca(sizeof(*fileLocation) * (fileLocationLength + 1/*'\0'*/ )); \
		\
		strncpy(fileLocation,(char*) PROPERTIES_PROPERTY_DATA(server->httpRootDirectory), server->httpRootDirectory->dataLength); \
		strncpy(fileLocation + server->httpRootDirectory->dataLength , symbolicFileLocation, symbolicFileLocationLength + 1); \
	}while(0)

#define SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION_ERROR_PAGE(varName, server, symbolicFileLocation, symbolicFileLocationLength) char* varName; \
	uint_fast64_t varName ## Length; \
	varName ## Length = server->customErrorPageDirectory->dataLength + symbolicFileLocationLength; \
	do{ \
	varName = alloca(sizeof(*fileLocation) * (varName ## Length + 1/*'\0'*/ )); \
	\
	strncpy(varName, (char*) PROPERTIES_PROPERTY_DATA(server->customErrorPageDirectory), server->customErrorPageDirectory->dataLength); \
	strncpy(varName + server->customErrorPageDirectory->dataLength , symbolicFileLocation, symbolicFileLocationLength + 1); \
	}while(0);

void server_sigHandler(int);

ERROR_CODE server_init(Server*, char*, const int_fast64_t);

ERROR_CODE server_initSSL_Context(Server*);

ERROR_CODE server_writeTemplatePropertyFileToDisk(char*, const int_fast64_t);

ERROR_CODE server_loadProperties(Server*, char*, const int_fast64_t);

ERROR_CODE server_createPropertyFile(char*, const uint_fast64_t);

ERROR_CODE server_initEpoll(Server*);

void server_start(Server*);

void server_run(Server*);

ERROR_CODE server_stop();

void server_free(Server*);

ERROR_CODE server_initContext(Context*, const char*, const uint_fast64_t, ContextHandler*);

void server_freeContext(Context*);

ERROR_CODE server_addContext(Server*, const char*, ContextHandler*);

ERROR_CODE server_getContextHandler(Server*, ContextHandler**, HTTP_Request*);

ERROR_CODE server_defaultContextHandler(Server*, HTTP_Request*, HTTP_Response*);

ERROR_CODE server_constructErrorPage(Server*, HTTP_Request*, HTTP_Response*, HTTP_StatusCode);

ERROR_CODE server_sendResponse(SSL*, HTTP_Response*);

void server_daemonize(void);

void server_printHelp(void);

ERROR_CODE server_showSettings(void);

#endif

/*
GET / HTTP/1.1
Host: localhost:1869
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:99.0) Gecko/20100101 Firefox/99.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,/;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate, br
DNT: 1
Connection: keep-alive
Upgrade-Insecure-Requests: 1
Sec-Fetch-Dest: document
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: none
Sec-Fetch-User: ?1
Sec-GPC: 1

HTTP/1.1 200 OK
Date: Mon, 27 Jul 2009 12:28:53 GMT
Server: Apache/2.2.14 (Win32)
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
Content-Length: 88
Content-Type: text/html
Connection: Closed
*/