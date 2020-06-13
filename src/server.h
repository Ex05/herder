#ifndef SERVER_H
#define SERVER_H

#include "util.h"
#include "threadPool.h"
#include "http.h"
#include "linkedList.h"
#include "mediaLibrary.h"
#include "cache.h"

#include <sys/un.h>
#include <netinet/in.h>
#include <sys/inotify.h>

typedef struct {
    int sockFD;
    struct sockaddr_in sockAddr;
    uint_fast16_t port;
    ThreadPool threadPool;
    volatile bool alive;
    LinkedList contexts;
    MediaLibrary library;
    Cache cache;
    char* rootDirectory;
    uint_fast64_t rootDirectoryLength;
}HerderServer;

typedef struct{
    int sockFD;
    struct sockaddr_in socket;
    HerderServer* server;
}Client;

#define SERVER_CONTEXT_HANDLER(functionName) ERROR_CODE functionName(HerderServer* server, HTTP_Request* request, HTTP_Response* response)
typedef SERVER_CONTEXT_HANDLER(ContextHandler);

typedef struct{
    char* location;
    uint_fast64_t locationLength;
    ContextHandler* contextHandler;
}Context;

ERROR_CODE server_init(HerderServer*, const char*, const uint_fast64_t, uint_fast16_t);

ERROR_CODE server_start(HerderServer*);

ERROR_CODE server_stop(HerderServer*);

void server_free(HerderServer*);

ERROR_CODE server_addContext(HerderServer*, const char*, ContextHandler*);

ERROR_CODE server_getContext(HerderServer*, ContextHandler**, HTTP_Request*);

ERROR_CODE server_constructErrorPage(HerderServer*,HTTP_Request*, HTTP_Response*, HTTP_StatusCode);

ERROR_CODE server_defaultContextHandler(HerderServer*, HTTP_Request*, HTTP_Response*);

#endif