#ifndef SERVER_C
#define SERVER_C

// POSIX Version ¢2008
#define _XOPEN_SOURCE 700

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include "util.c"
#include "server.h"
#include "linkedList.c"
#include "arrayList.c"
#include "threadPool.c"
#include "properties.c"
#include "http.c"
#include "cache.c"

THREAD_POOL_RUNNABLE_(epoll_run, Server, server);

local Server* server;

// main
#ifdef TEST_BUILD
	int server_totalyNotMain(const int argc, const char* argv[]){
#else
	int main(const int argc, const char* argv[]){
#endif
	ERROR_CODE error;

	server = malloc(sizeof(Server));
	if(server == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_RETRY_AGAIN);
	if((error = server_init(server, RESOURCES_PROPERTY_FILE_LOCATION, strlen(RESOURCES_PROPERTY_FILE_LOCATION))) != ERROR_NO_ERROR){
		goto label_free;
	}
	
	server_start(server);

label_free:
	server_free(server);

	return 0;
}

ERROR_CODE server_writeTemplatePropertyFileToDisk(char* propertyFileLocation, const int_fast64_t propertyFileLocationLength){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Writing template seetings file to disk...");

	// PropertyFile.		
	PROPERTIES_NEW_PROPERTY_FILE_TEMPLATE(Template, RESOURCES_PROPERTY_FILE_LOCATION);

	// Security
	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Security, CONSTANTS_PROPERTY_FILE_SECTION_SECURITY);	

		// SSL_Certificate
		PROPERTIES_NEW_PROPERTY_TEMPLATE(SSL_Certificate, CONSTANTS_SSL_CERTIFICATE_LOCATION_PROPERTY_NAME, CONSTANTS_SSL_CERTIFICATE_LOCATION_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Security, SSL_Certificate);

		// SSL_PrivateKeyFile
		PROPERTIES_NEW_PROPERTY_TEMPLATE(SSL_PrivateKeyFile, CONSTANTS_SSL_PRIVATE_KEY_FILE_PROPERTY_NAME, CONSTANTS_SSL_PRIVATE_KEY_FILE_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Security, SSL_PrivateKeyFile);

		PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(Template, Security);

	// Server
	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Server, CONSTANTS_PROPERTY_FILE_SECTION_SERVER);	

		// HTTP_ReadBufferSize
		PROPERTIES_NEW_PROPERTY_TEMPLATE(HTTP_ReadBufferSize, CONSTANTS_HTTP_READ_BUFFER_SIZE_PROPERTY_NAME, CONSTANTS_HTTP_READ_BUFFER_SIZE_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, HTTP_ReadBufferSize);

		// ErrorPageChacheSize
		PROPERTIES_NEW_PROPERTY_TEMPLATE(ErrorPageChacheSize, CONSTANTS_ERROR_PAGE_CACHE_SIZE_PROPERTY_NAME, CONSTANTS_ERROR_PAGE_CACHE_SIZE_POPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, ErrorPageChacheSize);

		// HTTP_CacheSize
		PROPERTIES_NEW_PROPERTY_TEMPLATE(HTTP_CacheSize, CONSTANTS_HTTP_CACHE_SIZE_PROPERTY_NAME, CONSTANTS_HTTP_CACHE_SIZE_POPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, HTTP_CacheSize);

		// EpollEventBufferSize
		PROPERTIES_NEW_PROPERTY_TEMPLATE(EpollEventBufferSize, CONSTANTS_EPOLL_EVENT_BUFFER_SIZE_PROPERTY_NAME, CONSTANTS_EPOLL_EVENT_BUFFER_SIZE_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, EpollEventBufferSize);

		// LogFileDirectory
		PROPERTIES_NEW_PROPERTY_TEMPLATE(LogFileDirectory, CONSTANTS_LOGFILE_DIRECTORY_PROPERTY_NAME, CONSTANTS_LOGFILE_DIRECTORY_POPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, LogFileDirectory);

		// CustomErrorPagesDirectory
		PROPERTIES_NEW_PROPERTY_TEMPLATE(CustomErrorPagesDirectory, CONSTANTS_CUSTOM_ERROR_PAGES_DIRECTORY_PROPERTY_NAME, CONSTANTS_CUSTOM_ERROR_PAGES_DIRECTORY_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, CustomErrorPagesDirectory);

		// HTTP_RootDirectory
		PROPERTIES_NEW_PROPERTY_TEMPLATE(HTTP_RootDirectory, CONSTANTS_HTTP_ROOT_DIRECTORY_PROPERTY_NAME, CONSTANTS_HTTP_ROOT_DIRECTORY_POPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, HTTP_RootDirectory);

		// NumWorkerThreads
		PROPERTIES_NEW_PROPERTY_TEMPLATE(NumWorkerThreads, CONSTANTS_NUM_WORKER_THREADS_PROPERTY_NAME, CONSTANTS_NUM_WORKER_THREADS_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, NumWorkerThreads);

		// Port
		PROPERTIES_NEW_PROPERTY_TEMPLATE(Port, CONSTANTS_PORT_PROPERTY_NAME, CONSTANTS_PORT_PROPERTY_DEFAULT_VALUE);
		PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, Port);

		PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(Template, Server);

	// Syslog_ID
	PROPERTIES_NEW_PROPERTY_TEMPLATE(SYSLOG_ID, CONSTANTS_SYSLOG_ID_PROPERTY_NAME, CONSTANTS_SYSLOG_ID_PROPERTY_DEFAULT_VALUE);
	PROPERTIES_PROPERTY_FILE_ADD_PROPERTY(Template, SYSLOG_ID);			

	// WorkDirectory
	PROPERTIES_NEW_PROPERTY_TEMPLATE(WorkDirectory, CONSTANTS_WORK_DIRECTORY_PROPERTY_NAME, CONSTANTS_WORK_DIRECTORY_PROPERTY_DEFAULT_VALUE);

	PROPERTIES_PROPERTY_FILE_ADD_PROPERTY(Template, WorkDirectory);			

	ERROR_CODE error;
	if((error = PROPERTIES_CREATE_PROPERTY_FILE(Template)) != ERROR_NO_ERROR){
		PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);

		return ERROR(error);
	}

	PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);	

	return ERROR(error);	
}

ERROR_CODE server_init(Server* server, char* propertyFileLocation, const int_fast64_t propertyFileLocationLength){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: Initialising server...");

	ERROR_CODE error;

	memset(server, 0, sizeof(Server));

	// Block default signal handler.
	sigset_t signalMask;
	sigemptyset(&signalMask);
	sigaddset(&signalMask, SIGINT);
	sigaddset(&signalMask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &signalMask, NULL);

	// Register cusotm signal handler.
	struct sigaction signalhandler = {0};
	signalhandler.sa_handler = &server_sigHandler;
	sigemptyset(&signalhandler.sa_mask);
	signalhandler.sa_flags = 0;

	// Register handle for these signals.
	sigaction(SIGUSR1, &signalhandler, NULL);
	sigaction(SIGINT, &signalhandler, NULL);
	sigaction(SIGPIPE, &signalhandler, NULL);

	// Check if settings file exists.
	if(!util_fileExists(propertyFileLocation)){
		if((error = server_createPropertyFile(propertyFileLocation, propertyFileLocationLength)) != ERROR_NO_ERROR){
			return ERROR(error);
		}
	}else{
		if((error = server_loadProperties(server, propertyFileLocation, propertyFileLocationLength))){
			return ERROR(error);
		}
	}

	// Syslog_ID.
	Property* syslogID_Property = PROPERTIES_GET(&server->properties, SYSLOG_ID);

	char* syslogID = alloca(sizeof(*syslogID) * (syslogID_Property->dataLength + 1));
	memcpy(syslogID, PROPERTIES_PROPERTY_DATA(syslogID_Property), syslogID_Property->dataLength);
	syslogID[syslogID_Property->dataLength] = '\0';

	openlog(syslogID, LOG_PID, LOG_USER);

	if(sem_init(&server->running, 0, 0) != 0){
		return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
	}

	// HTTP_RootDirectory.
	server->httpRootDirectory = PROPERTIES_GET(&server->properties, HTTP_ROOT_DIRECTORY);

	// WorkDirectory.
	server->workDirectory = PROPERTIES_GET(&server->properties, WORK_DIRECTORY);

	// CustomErrorPageDirectory.
	server->customErrorPageDirectory = PROPERTIES_GET(&server->properties, CUSTOM_ERROR_PAGES_DIRECTORY);

	// NumWorkerThreads.
	Property* numWorkerThreadsProperty = PROPERTIES_GET(&server->properties, NUM_WORKER_THREADS);

	char* numWorkerThreadsString = alloca(sizeof(*numWorkerThreadsString) * (numWorkerThreadsProperty->dataLength + 1));
	memcpy(numWorkerThreadsString, PROPERTIES_PROPERTY_DATA(numWorkerThreadsProperty), numWorkerThreadsProperty->dataLength);
	numWorkerThreadsString[numWorkerThreadsProperty->dataLength] = '\0';

	int_fast64_t numWorkerThreads;
	if((error = util_stringToInt(numWorkerThreadsString, &numWorkerThreads)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tInitialising worker thread poll...");
	if((error = threadPool_init(&server->epollWorkerThreads, numWorkerThreads)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = server_initSSL_Context(server)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tCreating server socket...");
	// Create server socket.
	struct sockaddr_in6 serverSocketAddress = {0};
	serverSocketAddress.sin6_flowinfo = 0;
	serverSocketAddress.sin6_family = AF_INET6;
	serverSocketAddress.sin6_port = htons(1869);
	serverSocketAddress.sin6_addr = in6addr_any;

	server->socketFileDescriptor = socket(serverSocketAddress.sin6_family, SOCK_STREAM, 0);
	if(server->socketFileDescriptor == 0){		
		return ERROR(ERROR_UNIX_DOMAIN_SOCKET_INITIALISATION_FAILED);
	}

 	if(bind(server->socketFileDescriptor, (struct sockaddr*) &serverSocketAddress, sizeof(serverSocketAddress)) != 0){
		return ERROR(ERROR_FAILED_TO_BIND_SERVER_SOCKET);
	}

	if(listen(server->socketFileDescriptor, 1) < 0){
		return ERROR(ERROR_FAILED_TO_LISTEN_ON_SERVER_SOCKET);
	}

	if((error = server_initEpoll(server)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	// TODO: Pull cache size from settings. (jan - 2022.10.01)
	// 'www' directory cache.
	if((error = cache_init(&server->cache, server->epollWorkerThreads.numWorkers, MB(64)))){
		return ERROR(error);
	}

	// Error page cache.
	if((error = cache_init(&server->errorPageCache, server->epollWorkerThreads.numWorkers, MB(2)))){
		return ERROR(error);
	}

	// HTML/Static pages
	server_addContext(server, "/", server_defaultContextHandler);
	server_addContext(server, "/img", server_defaultContextHandler);
	server_addContext(server, "/css", server_defaultContextHandler);
	
	return ERROR(ERROR_NO_ERROR);
}

THREAD_POOL_RUNNABLE_(epoll_run, Server, server){
	ERROR_CODE error;

	Property* httpReadBufferSizeProperty = PROPERTIES_GET(&server->properties, HTTP_READ_BUFFER_SIZE);

	char* httpReadBufferSizeString = alloca(sizeof(*httpReadBufferSizeString) * (httpReadBufferSizeProperty->dataLength + 1));
	memcpy(httpReadBufferSizeString, PROPERTIES_PROPERTY_DATA(httpReadBufferSizeProperty), httpReadBufferSizeProperty->dataLength);
	httpReadBufferSizeString[httpReadBufferSizeProperty->dataLength] = '\0';

	int_fast64_t httpReadBufferSize;
	if((error = util_stringToInt(httpReadBufferSizeString, &httpReadBufferSize)) != ERROR_NO_ERROR){
		THREAD_POOL_RUNNABLE_RETURN(error);
	}

	Property* epollReadBufferSizePoroperty = PROPERTIES_GET(&server->properties, HTTP_READ_BUFFER_SIZE);

	char* epollReadBufferSizeString = alloca(sizeof(*epollReadBufferSizeString) * (epollReadBufferSizePoroperty->dataLength + 1));
	memcpy(epollReadBufferSizeString, PROPERTIES_PROPERTY_DATA(epollReadBufferSizePoroperty), epollReadBufferSizePoroperty->dataLength);
	epollReadBufferSizeString[epollReadBufferSizePoroperty->dataLength] = '\0';

	int_fast64_t epollReadBufferSize;
	if((error = util_stringToInt(epollReadBufferSizeString, &epollReadBufferSize)) != ERROR_NO_ERROR){
		THREAD_POOL_RUNNABLE_RETURN_(int, ERROR_FAILED_TO_RETRIEV_FILE_INFO);
	}

	struct epoll_event* epollEventBuffer;
	epollEventBuffer = malloc(sizeof(struct epoll_event) * epollReadBufferSize);

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: Epoll worker entering event loop...");

	for(;;){
		sigset_t signalMask;
		sigemptyset(&signalMask);
		
		int numberEvents = epoll_pwait(server->epollClientHandlingFileDescriptor, epollEventBuffer, epollReadBufferSize, -1, &signalMask);

		if(numberEvents == -1){
			break;
		}

		char readBuffer[httpReadBufferSize];

		for(int i = 0; i < numberEvents; ++i){
			// Init SSL.
			SSL* sslInstance = SSL_new(server->sslContext);
			SSL_set_ciphersuites(sslInstance, "TLS_AES_256_GCM_SHA384");
			SSL_set_fd(sslInstance, epollEventBuffer[i].data.fd);

			for(;;){
				const int accept = SSL_accept(sslInstance);

				// Success.
				if(accept == 1){
					break;
				}else if(accept == -1){
					const int sslError = SSL_get_error(sslInstance, accept);

					if(sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE){
						// The action depends on the underlying BIO . When using a non-blocking socket, nothing is to be done, but select() can be used to check for the required condition.					
						struct timespec sleepRequest = {1, 0};
						struct timespec sleepRemaining;
						
						nanosleep(&sleepRequest, &sleepRemaining);		

						continue;				
					}
				}else{
					goto label_closeSSL_Connection;
				}
			}

			uint_fast64_t readBufferOffset = 0;
			for(;;){
				// Note: SSL_read...with a maximum record size of 16kB for SSLv3/TLSv1).
				int_fast32_t bytesRead = SSL_read(sslInstance, readBuffer + readBufferOffset, httpReadBufferSize / 4);

				UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tSSL_read (%" PRIdFAST32 ") bytes.", bytesRead);

				if(bytesRead > 0){
					readBufferOffset += bytesRead;

					continue;
				}else if(bytesRead == 0){
					const int sslError = SSL_get_error(sslInstance, bytesRead);

					UTIL_LOG_CONSOLE_(LOG_ERR, "Worker: \tbytesRead = 0 '%s'.", ERR_error_string(SSL_get_error(sslInstance, bytesRead), NULL));

					if(sslError ==SSL_ERROR_WANT_READ){
						// When using a non-blocking socket, nothing is to be done, but select() can be used to check for the required condition.
						UTIL_LOG_CONSOLE(LOG_DEBUG, "SSL_ERROR_WANT_READ.");

						sched_yield();
					}else if(sslError == SSL_ERROR_WANT_WRITE){
						UTIL_LOG_CONSOLE(LOG_DEBUG, "SSL_ERROR_WANT_WRITE.");

						sched_yield();
					}else if(sslError == SSL_RECEIVED_SHUTDOWN){
						UTIL_LOG_CONSOLE(LOG_DEBUG, "SSL_RECEIVED_SHUTDOWN.");

						break;
					}else if(sslError == SSL_ERROR_ZERO_RETURN){
						UTIL_LOG_CONSOLE(LOG_DEBUG, "SSL_ERROR_ZERO_RETURN.");

						break;
					}
				}else{
					const int sslError = SSL_get_error(sslInstance, bytesRead);

					if(sslError == SSL_ERROR_NONE){
						UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tSSL_ERROR_NONE.");
					}else if(sslError == SSL_RECEIVED_SHUTDOWN){
						UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tSSL_RECEIVED_SHUTDOWN.");

						sleep(1);
					}

					UTIL_LOG_CONSOLE_(LOG_ERR, "%s", ERR_error_string(sslError, NULL));

					break;
				}
			}

			uint_fast64_t bytesRead = readBufferOffset;
			if(bytesRead > 0){
				HTTP_Request request = {0};
				if((error = http_parseHTTP_Request(&request, readBuffer, bytesRead)) != ERROR_NO_ERROR){
					UTIL_LOG_CONSOLE_(LOG_DEBUG, "Failed to parse HTTP request. [%s]", util_toErrorString(error));
				} 

				UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tRequest URL:'%s'.", request.requestURL);

				HTTP_Response response;
				http_initHttpResponse(&response, readBuffer, httpReadBufferSize);

				UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tRetrieving context handler...");

				ContextHandler* contextHandler;
				if((error = server_getContextHandler(server, &contextHandler, &request)) != ERROR_NO_ERROR){
					UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to retrieve http context handler. [%s]", util_toErrorString(error));

					if((error = server_constructErrorPage(server, &request, &response, _401_UNAUTHORIZED)) != ERROR_NO_ERROR){
						UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to construct error page. (%s)." , util_toErrorString(error));
					}
				}

				// Only if we are not sending an error page call the apropriate context handler.
				if(contextHandler != NULL){
					if((error = contextHandler(server, &request, &response)) != ERROR_NO_ERROR){
						if(error == ERROR_FAILED_TO_RETRIEV_FILE_INFO){
								if((error = server_constructErrorPage(server, &request, &response, _401_UNAUTHORIZED)) != ERROR_NO_ERROR){
						UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to construct error page. (%s)." , util_toErrorString(error));
					}
						}
					}
				}

				server_sendResponse(sslInstance, &response);
			}

		label_closeSSL_Connection:
			UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tClosing client connection. [FD:%d].\n", epollEventBuffer[i].data.fd);

			epoll_ctl(server->epollClientHandlingFileDescriptor, EPOLL_CTL_DEL, epollEventBuffer[i].data.fd, NULL);
			close(epollEventBuffer[i].data.fd);

			SSL_shutdown(sslInstance);
			SSL_free(sslInstance);

			error = ERROR_INVALID_SIGNAL;

			break;
		}
	}
	
	THREAD_POOL_RUNNABLE_RETURN_(int, error);
}

ERROR_CODE server_sendResponse(SSL* sslInstance, HTTP_Response* response){
	ERROR_CODE error = ERROR_NO_ERROR;

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tSending response...");

	// Note: We use the response buffer which is just the request buffer to build the reposen. If the response content is static the whole buffer is available, if not the response header information gets attached to the end of the data segment. (jan 2022.10.14)
	int8_t* responseBuffer = response->dataSegment + response->responseDataSegmentLength;
	const int responseBufferSize = response->responseBufferSize - response->responseDataSegmentLength;

	// Response line.
	uint_fast64_t writeOffset = snprintf((char*) responseBuffer, responseBufferSize, "%s %" PRIdFAST16 " %s\r\n", http_getVersionString(response->httpVersion), http_getNumericalStatusCode(response->httpStatusCode), http_getStatusMsg(response->httpStatusCode));

	// Header fields.
	LinkedListIterator it;
	linkedList_initIterator(&it, &response->httpHeaderFields);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT_PTR(&it, HTTP_HeaderField);

		memcpy(responseBuffer + writeOffset, headerField->name, headerField->nameLength);
		writeOffset += headerField->nameLength;

		memcpy(responseBuffer + writeOffset, ": ", 2);
		writeOffset += 2;

		memcpy(responseBuffer + writeOffset, headerField->value, headerField->valueLength);
		writeOffset += headerField->valueLength;

		memcpy(responseBuffer + writeOffset, "\r\n", 2);
		writeOffset += 2;
	}

	// Trailing new line to signal begining of data segment.
	memcpy(responseBuffer + writeOffset, "\r\n", 2);
	writeOffset += 2;

	uint_fast64_t bytesWritten = SSL_write(sslInstance, response->dataSegment + response->responseDataSegmentLength, writeOffset);
	if(bytesWritten != writeOffset){
		UTIL_LOG_ERROR_("SSL_Write ERROR, failed to write %" PRIuFAST64 " out of %" PRIuFAST64 " bytes.", writeOffset - bytesWritten, writeOffset);

		error = ERROR_WRITE_ERROR;
	}

	if(response->staticContent){
		bytesWritten = SSL_write(sslInstance, response->cacheObject->data, response->cacheObject->size);
		if(bytesWritten != response->cacheObject->size){
			UTIL_LOG_ERROR_("SSL_Write ERROR, failed to write %" PRIuFAST64 " out of %" PRIuFAST64 " bytes.", bytesWritten, response->cacheObject->size);
		}
	}else{
		bytesWritten = SSL_write(sslInstance, response->dataSegment, response->responseDataSegmentLength);
		if(bytesWritten != response->responseDataSegmentLength){
			UTIL_LOG_ERROR_("SSL_Write ERROR, failed to write %" PRIuFAST64 " out of %" PRIuFAST64 " bytes.", bytesWritten, response->responseDataSegmentLength);

			error = ERROR_WRITE_ERROR;
		}
	}

	return ERROR(error);
}

ERROR_CODE server_constructErrorPage(Server* server, HTTP_Request* request, HTTP_Response* response, HTTP_StatusCode httpStatusCode){
	ERROR_CODE error;

	const int_fast16_t nummericalHTTP_StatusCode = http_getNumericalStatusCode(httpStatusCode);

	char httpStatusCodeString[4];
	snprintf(httpStatusCodeString, 4, "%03" PRIdFAST16 "", nummericalHTTP_StatusCode);

	const uint_fast64_t symbolicFileLocationLength = + 5/*".html"*/ + 1/*'/'*/ + 1/*_*/+ 3/*http status code*/;
	char* symbolicFileLocation = alloca(sizeof(*symbolicFileLocation) * (symbolicFileLocationLength + 1));
	snprintf(symbolicFileLocation, symbolicFileLocationLength + 1, "/%s.html", httpStatusCodeString);

	SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION_ERROR_PAGE(fileLocation, server, symbolicFileLocation, symbolicFileLocationLength);
	
	CacheObject* cacheObject = NULL;
	// TODO: Clean up this type cast and see if we cant make the "symbolicFileLocation" parameter in 'cache_get' a constant char pointer.(jan - 2022.09.07)
	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
	if((error = cache_get(&server->errorPageCache, &cacheObject, (char*) symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
		if(error != ERROR_ENTRY_NOT_FOUND){
			return ERROR(error);
		}
		else{
			SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION_ERROR_PAGE(fileLocation, server, symbolicFileLocation, symbolicFileLocationLength);

			__UTIL_ENABLE_ERROR_LOGGING__();
			__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_FAILED_TO_RETRIEV_FILE_INFO);
			if((error = cache_load(&server->errorPageCache, &cacheObject, fileLocation, fileLocationLength, (char*) symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
				if(error != ERROR_FAILED_TO_RETRIEV_FILE_INFO){
					return ERROR(error);
				}else{
					// Load default error page into cache.
					cache_add(&server->errorPageCache, &cacheObject,(uint8_t*) CONSTANTS_MINIMAL_HTTP_ERROR_PAGE, strlen(CONSTANTS_MINIMAL_HTTP_ERROR_PAGE) + 1, NULL, 0, symbolicFileLocation, symbolicFileLocationLength);
				}
			}
		}
		__UTIL_ENABLE_ERROR_LOGGING__();
	}

	memcpy(response->dataSegment, cacheObject->data, cacheObject->size);
	response->responseDataSegmentLength = strlen(CONSTANTS_MINIMAL_HTTP_ERROR_PAGE);

	// $errorCode
	util_replace((char*) response->dataSegment, response->responseBufferSize, &response->responseDataSegmentLength, CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_CODE, strlen(CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_CODE), httpStatusCodeString, 3);

	// $errrorMessage
	const char* errorMessage = http_getStatusMsg(httpStatusCode);
	const uint_fast64_t errorMessageLength = strlen(errorMessage);

	util_replace((char*) response->dataSegment, response->responseBufferSize, &response->responseDataSegmentLength, CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_MESSAGE, strlen(CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_MESSAGE), errorMessage, errorMessageLength);

	// $address
	const HTTP_HeaderField* headerFieldHost = http_getHeaderField(request, CONSTANTS_HTTP_HEADER_FIELD_HOST_NAME);

	if(headerFieldHost != NULL){
		util_replace((char*) response->dataSegment, response->responseBufferSize, &response->responseDataSegmentLength, CONSTANTS_ERROR_PAGE_SEARCH_STRING_ADDRESS, strlen(CONSTANTS_ERROR_PAGE_SEARCH_STRING_ADDRESS), headerFieldHost->value, headerFieldHost->valueLength);
	}

	// $port
	if(headerFieldHost != NULL){
		const int_fast64_t posSplit = util_findLast(headerFieldHost->value, headerFieldHost->valueLength, ':') + 1;

		if(posSplit != 0){
			char portString[5];

			int_fast16_t port;						
			if((error = util_stringToInt(headerFieldHost->value + posSplit, &port)) != ERROR_NO_ERROR){
				UTIL_LOG_ERROR(util_toErrorString(error));
			}
			const int_fast64_t portLength = snprintf(portString, 5, "%" PRIdFAST16 "", port);
		
			util_replace((char*) response->dataSegment, response->responseBufferSize, &response->responseDataSegmentLength, CONSTANTS_ERROR_PAGE_SEARCH_STRING_PORT, strlen(CONSTANTS_ERROR_PAGE_SEARCH_STRING_PORT), portString, portLength);
		}
	}

	response->httpStatusCode = httpStatusCode;

	response->httpContentType = HTTP_CONTENT_TYPE_TEXT_HTML;

	UTIL_INT_TO_STRING_HEAP_ALLOCATED(contentLengthString, response->responseDataSegmentLength);
	HTTP_ADD_HEADER_FIELD(response, Content-Length, contentLengthString);

	HTTP_ADD_HEADER_FIELD(response, Content-Type, http_contentTypeToString(response->httpContentType));

	HTTP_ADD_HEADER_FIELD(response, Connection, "Closed");
	
	return ERROR(ERROR_NO_ERROR);
}

SERVER_CONTEXT_HANDLER(server_defaultContextHandler){
	ERROR_CODE error;

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tEntering default context handler...");

	// Handle that 'index.html' can be reached without '/index.html' in the url.
	uint_fast64_t symbolicFileLocationLength;
	char* symbolicFileLocation;
	if(request->requestURLLength == 1 && request->requestURL[0] == '/'){
		symbolicFileLocationLength = 11/*"/index.html"*/;
		symbolicFileLocation = alloca(sizeof(*symbolicFileLocation) * (symbolicFileLocationLength + 1));

		memcpy(symbolicFileLocation, "/index.html", symbolicFileLocationLength);
		symbolicFileLocation[symbolicFileLocationLength] = '\0';
	}else{
		symbolicFileLocationLength = request->requestURLLength;
		 
		symbolicFileLocation = alloca(sizeof(*symbolicFileLocation) * (symbolicFileLocationLength + 1));
		memcpy(symbolicFileLocation, request->requestURL, symbolicFileLocationLength);
		symbolicFileLocation[symbolicFileLocationLength] = '\0';
	}

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tChecking cache for entries...");

	CacheObject* cacheObject = NULL;
	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
	if((error = cache_get(&server->errorPageCache, &cacheObject, (char*) symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tCache did not contaion an entry for: '%s'.", symbolicFileLocation);

		if(error != ERROR_ENTRY_NOT_FOUND){
			return ERROR(error);
		}
		else{
			SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION(fileLocation, server, symbolicFileLocation, symbolicFileLocationLength);

			__UTIL_ENABLE_ERROR_LOGGING__();
			__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_FAILED_TO_RETRIEV_FILE_INFO);

			UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tLoading cacheobject: '%s' from file: '%s'.", symbolicFileLocation, fileLocation);
			if((error = cache_load(&server->errorPageCache, &cacheObject, fileLocation, fileLocationLength, (char*) symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
				UTIL_LOG_CONSOLE(LOG_ERR, "Failed to load cacheObject.");

				return ERROR(error);
			}
		}
		__UTIL_ENABLE_ERROR_LOGGING__();
	}else{
		UTIL_LOG_CONSOLE(LOG_DEBUG, "Worker: \tCache entry found.");
	}

	response->cacheObject = cacheObject;
	response->staticContent = true;

	response->httpStatusCode = _200_OK;

	response->httpContentType = cacheObject->httpContentType;

	UTIL_INT_TO_STRING_HEAP_ALLOCATED(contentLengthString, response->cacheObject->size);
	HTTP_ADD_HEADER_FIELD(response, Content-Length, contentLengthString);

	HTTP_ADD_HEADER_FIELD(response, Content-Type, http_contentTypeToString(response->httpContentType));

	HTTP_ADD_HEADER_FIELD(response, Connection, "Closed");

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE server_loadProperties(Server* server, char* propertyFileLocation, const int_fast64_t propertyFileLocationLength){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tLoading settings from disk...");

	ERROR_CODE error;
		if((error = properties_load(&server->properties, propertyFileLocation, propertyFileLocationLength)) != ERROR_NO_ERROR){
			goto label_return;
		}

		// TODO: Check that values are valid, like port range, directory strings are properly terminated/formated. (jan - 2022.09.29)
		// #Server.
		PROPERTY_EXISTS(PORT);
		PROPERTY_EXISTS(NUM_WORKER_THREADS);
		PROPERTY_EXISTS(HTTP_ROOT_DIRECTORY);
		PROPERTY_EXISTS(CUSTOM_ERROR_PAGES_DIRECTORY);
		PROPERTY_EXISTS(LOGFILE_DIRECTORY);
		PROPERTY_EXISTS(EPOLL_EVENT_BUFFER_SIZE);
		PROPERTY_EXISTS(HTTP_CACHE_SIZE);
		PROPERTY_EXISTS(ERROR_PAGE_CACHE_SIZE);

		PROPERTY_EXISTS(HTTP_READ_BUFFER_SIZE);
		
		// #Security
		PROPERTY_EXISTS(SSL_CERTIFICATE_LOCATION);
		PROPERTY_EXISTS(SSL_PRIVATE_KEY_FILE);

		PROPERTY_EXISTS(SYSLOG_ID);
		PROPERTY_EXISTS(WORK_DIRECTORY);

label_return:
	return ERROR(error);
}

ERROR_CODE server_createPropertyFile(char* propertyFileLocation, const uint_fast64_t propertyFileLocationLength){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: Creating property file...");

	ERROR_CODE error;

	label_readUserInput:
		UTIL_LOG_CONSOLE_(LOG_INFO, "The file: '%s' does not exist do you want to create it? Yes/No.", propertyFileLocation);

		int_fast64_t userInputLength;
		char* userInput;

		if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
			goto label_return;
		}

		userInput = util_trim(userInput, userInputLength);
		util_toLowerChase(userInput);

		if(strncmp(userInput, "yes", 4) == 0){
			if((error = server_writeTemplatePropertyFileToDisk(propertyFileLocation, propertyFileLocationLength)) != ERROR_NO_ERROR){
				goto label_return;
			}else{
				UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully created file: '%s'.\n\nPlease adjust the default values to your needs and then run this executable again.", propertyFileLocation);

				error = ERROR_RETRY_AGAIN;
			}
		}else if(strncmp(userInput, "no", 3) != 0){
			free(userInput);

			goto label_readUserInput;
		}

label_return:
	return ERROR(error);
}

ERROR_CODE server_initSSL_Context(Server* server){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tInitialising SSL...");

	// Note: All properties are guaranteed to be available at this point in time, but there validity is not guaranteed. (jan - 2022.06.11)
	Property* sslCertificateLocationProperty = PROPERTIES_GET(&server->properties, SSL_CERTIFICATE_LOCATION);

	char* sslCertificateLocation = alloca(sizeof(*sslCertificateLocation) * (sslCertificateLocationProperty->dataLength + 1));
	memcpy(sslCertificateLocation, PROPERTIES_PROPERTY_DATA(sslCertificateLocationProperty), sslCertificateLocationProperty->dataLength);
	sslCertificateLocation[sslCertificateLocationProperty->dataLength] = '\0';

	Property* sslPrivateKeyProperty = PROPERTIES_GET(&server->properties, SSL_PRIVATE_KEY_FILE);

	char* sslPrivateKeyLocation = alloca(sizeof(*sslPrivateKeyLocation) * (sslPrivateKeyProperty->dataLength + 1));
	memcpy(sslPrivateKeyLocation, PROPERTIES_PROPERTY_DATA(sslPrivateKeyProperty), sslPrivateKeyProperty->dataLength);
	sslPrivateKeyLocation[sslPrivateKeyProperty->dataLength] = '\0';

	// Init SSL context.
	const SSL_METHOD* sslMethod = TLS_server_method();

	server->sslContext = SSL_CTX_new(sslMethod);
	if(server->sslContext == NULL){
		return ERROR(ERROR_SSL_INITIALISATION_ERROR);
	}

	SSL_CTX_set_min_proto_version(server->sslContext, TLS1_3_VERSION);
	
	// Generate certificate.
	// openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout testCertificate.pem -out testCertificate.pem

	if(!util_fileExists(sslCertificateLocation)){
		UTIL_LOG_CONSOLE_(LOG_INFO, "The SSL certificate file at '%s' does not exist.", sslCertificateLocation);

		return ERROR(ERROR_FILE_NOT_FOUND);
	}

	if(!util_fileExists(sslPrivateKeyLocation)){
		UTIL_LOG_CONSOLE_(LOG_INFO, "The SSL private file at '%s' does not exist.", sslCertificateLocation);

		return ERROR(ERROR_FILE_NOT_FOUND);
	}

	// Load certificate.
	if(SSL_CTX_use_certificate_file(server->sslContext, sslCertificateLocation, SSL_FILETYPE_PEM) != 1){
		SERVER_GET_SSL_ERROR_STRING(sslErrorString);

		UTIL_LOG_CONSOLE(LOG_ERR, "Error: Invalid SSL certificate.");

		return ERROR_(ERROR_SSL_INITIALISATION_ERROR, "SSL_ERROR: '%s'.", sslErrorString);
	}

	if(SSL_CTX_use_PrivateKey_file(server->sslContext, sslPrivateKeyLocation, SSL_FILETYPE_PEM) != 1){
		SERVER_GET_SSL_ERROR_STRING(sslErrorString);

		UTIL_LOG_CONSOLE(LOG_ERR, "Error: Invalid SSL private key file.");

		return ERROR_(ERROR_SSL_INITIALISATION_ERROR, "SSL_ERROR: '%s'.", sslErrorString);
	}

	if(SSL_CTX_check_private_key(server->sslContext) != 1){
		SERVER_GET_SSL_ERROR_STRING(sslErrorString);

		return ERROR_(ERROR_SSL_INITIALISATION_ERROR, "SSL_ERROR: '%s'.", sslErrorString);
	}

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE server_initEpoll(Server* server){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tInitialising epoll...");

	server->epollAcceptFileDescriptor = epoll_create1(0x0000);
	if(server->epollAcceptFileDescriptor == -1){
		return ERROR(ERROR_FAILED_TO_INITIALISE_EPOLL);
	}

	server->epollClientHandlingFileDescriptor = epoll_create1(0x0000);
	if(server->epollClientHandlingFileDescriptor == -1){
		return ERROR(ERROR_FAILED_TO_INITIALISE_EPOLL);
	}

	struct epoll_event event = {0};
	event.events = EPOLLIN;
	event.data.fd = server->socketFileDescriptor;
	epoll_ctl(server->epollAcceptFileDescriptor, EPOLL_CTL_ADD, server->socketFileDescriptor, &event);

	return ERROR(ERROR_NO_ERROR);
}

inline void server_free(Server* server){
	ERROR_CODE** returnValues = (ERROR_CODE**) threadPool_free(&server->epollWorkerThreads);

	if(*returnValues != NULL){
		uint_fast64_t i;
		for(i = 0; i < server->epollWorkerThreads.numWorkers; i++){
			ERROR_CODE* errorCode = (ERROR_CODE*) returnValues[i];

			if(*errorCode != ERROR_NO_ERROR){
				UTIL_LOG_ERROR_("ERROR: Worker thread[%" PRIuFAST64 "] returned with error code %d (%s).", i, *errorCode, util_toErrorString(*errorCode));
			}
		}
	}

	close(server->socketFileDescriptor);
	close(server->epollAcceptFileDescriptor);
	close(server->epollClientHandlingFileDescriptor);

	SSL_CTX_free(server->sslContext);

	sem_destroy(&server->running);

	closelog();
}

inline void server_start(Server* server){
	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: Starting server...");
	server_run(server);
}

void server_sigHandler(int signal){
	switch (signal){
		case SIGUSR1:{
			break;
		}

		case SIGINT:{
			server_stop(&server);

			break;
		}

		case SIGPIPE:{
			UTIL_LOG_CONSOLE(LOG_INFO, "Broken Pipe.");

			break;
		}
		
		default:{
			UTIL_LOG_CONSOLE_(LOG_DEBUG, "Signal not handled. %d", signal);

			break;
		}
	}
}

inline ERROR_CODE server_stop(Server* server){
	if(sem_post(&server->running) != 0){
		UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to change server running state: '%s'.", strerror(errno));
	}

	ERROR_CODE error;
	if((error = threadPool_signalAll(&server->epollWorkerThreads, SIGUSR1)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(ERROR_NO_ERROR);
}

void server_run(Server* server){
	ERROR_CODE error;

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tStarting worker threads...");

	uint_fast16_t i;
	for(i = 0; i < server->epollWorkerThreads.numWorkers; i++){
		threadPool_run(&server->epollWorkerThreads, (Runnable*) epoll_run, server);
	}

	Property* epollReadBufferSizePoroperty = PROPERTIES_GET(&server->properties, HTTP_READ_BUFFER_SIZE);

	char* epollReadBufferSizeString = alloca(sizeof(*epollReadBufferSizeString) * (epollReadBufferSizePoroperty->dataLength + 1));
	memcpy(epollReadBufferSizeString, PROPERTIES_PROPERTY_DATA(epollReadBufferSizePoroperty), epollReadBufferSizePoroperty->dataLength);
	epollReadBufferSizeString[epollReadBufferSizePoroperty->dataLength] = '\0';

	int_fast64_t epollReadBufferSize;
	if((error = util_stringToInt(epollReadBufferSizeString, &epollReadBufferSize)) != ERROR_NO_ERROR){
		// TODO: Return error code to calling thread pool.
	}

	struct epoll_event* epollEventBuffer;
	epollEventBuffer = malloc(sizeof(struct epoll_event) * epollReadBufferSize);

	sigset_t signalMask;
	sigemptyset(&signalMask);

	UTIL_LOG_CONSOLE(LOG_DEBUG, "Server: \tEntering server epoll event loop...");

	int running;
	do{
		if(sem_getvalue(&server->running, &running) != 0){
			UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to retrieve server running state: '%s'.", strerror(errno));

			break;
		}

		if(running != 0){
			break;
		}
	
		int numberEvents = epoll_pwait(server->epollAcceptFileDescriptor, epollEventBuffer, epollReadBufferSize, -1, &signalMask);

		if(numberEvents == -1){
			break;
		}

		int i;
		for(i = 0; i < numberEvents; ++i){
			if(epollEventBuffer[i].data.fd == server->socketFileDescriptor){
				struct sockaddr_in clientSocketAddress;
				socklen_t socketAddressLength = sizeof(clientSocketAddress);
				int clientSocketFD = accept(server->socketFileDescriptor, (struct sockaddr*) &clientSocketAddress, &socketAddressLength);

				const int flag = fcntl(clientSocketFD, F_GETFL, 0);
				fcntl(clientSocketFD, F_SETFL, flag | O_NONBLOCK);

				struct epoll_event event = {0};
				event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT;
				event.data.fd = clientSocketFD;
				epoll_ctl(server->epollClientHandlingFileDescriptor, EPOLL_CTL_ADD, clientSocketFD, &event);

				char clientIP_Address[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, &clientSocketAddress, clientIP_Address, INET6_ADDRSTRLEN);
				UTIL_LOG_CONSOLE_(LOG_DEBUG, "Worker: \tClient connected, '%s' [FD:%d].", clientIP_Address, clientSocketFD);
			}
		}
	}while(running == 0);
}

inline ERROR_CODE server_addContext(Server* server, const char* location, ContextHandler* contextHandler){
	Context* context = malloc(sizeof(*context));
	if(context == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	server_initContext(context, location, strlen(location), contextHandler);

	linkedList_add(&server->contexts, &context, sizeof(Context*));

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE server_initContext(Context* context, const char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength, ContextHandler* contextHandler){
	context->symbolicFileLocationLength = symbolicFileLocationLength;

	context->symbolicFileLocation = malloc(sizeof(*symbolicFileLocation) * (symbolicFileLocationLength + 1));
	if(context->symbolicFileLocation == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	strncpy(context->symbolicFileLocation, symbolicFileLocation, symbolicFileLocationLength + 1);

	context->contextHandler = contextHandler;

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE server_getContextHandler(Server* server, ContextHandler** contextHandler, HTTP_Request* request){
	char* baseDirectory;
	uint_fast64_t baseDirectoryLength;
	if(util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, request->requestURL, request->requestURLLength) != ERROR_NO_ERROR){
		*contextHandler = NULL;

		return ERROR_(ERROR_INVALID_REQUEST_URL, "Failed to find baseDirectory URL:'%s'.", request->requestURL);
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &server->contexts);
	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Context* context = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Context);

		if(strncmp(baseDirectory, context->symbolicFileLocation, baseDirectoryLength) == 0){
			if(context->symbolicFileLocation[baseDirectoryLength] == '\0'){
				*contextHandler = context->contextHandler;

				return ERROR(ERROR_NO_ERROR);
			}
		}
	}

	*contextHandler = NULL;

	return ERROR(ERROR_ENTRY_NOT_FOUND);
}

inline void server_freeContext(Context* context){
	free(context->symbolicFileLocation);
}

#endif
