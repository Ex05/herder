#include "util.c"
#include "linkedList.c"
#include "arrayList.c"
#include "util.h"

#include <netdb.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
 #include <openssl/crypto.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <unistd.h>

int main(const int argc, const char** argv){
	int status = EXIT_SUCCESS;

	openlog("herder_epollTest", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
	
	fork();
	fork();
	fork();

	// Create socket.
	struct addrinfo addressHints = {0};
	addressHints.ai_family = AF_INET6;
	addressHints.ai_socktype = SOCK_STREAM;
	
	char service[UTIL_FORMATTED_NUMBER_LENGTH];
	snprintf(service, UTIL_FORMATTED_NUMBER_LENGTH, "%d", 1869);

	struct addrinfo* serverInfo;
	int error;
	if((error = getaddrinfo("::1", service, &addressHints, &serverInfo)) != 0){
		UTIL_LOG_CONSOLE_(LOG_ERR, "GetaddrInfo: '%s'.", gai_strerror(error));

		freeaddrinfo(serverInfo);

		status = EXIT_FAILURE;

		goto label_cleanUp;
	}

	int serverSocketFD = 0;

	struct addrinfo* sockInfo;
	// Loop through all returned internet addresses.
	for(sockInfo = serverInfo; sockInfo != NULL; sockInfo = sockInfo->ai_next){
		// Try to create communication endpoint with internet address.
		if((serverSocketFD = socket(sockInfo->ai_family, sockInfo->ai_socktype, sockInfo->ai_protocol)) == -1){
			continue;
		}

		// Connect file discriptor with internet address.
		if(connect(serverSocketFD, sockInfo->ai_addr, sockInfo->ai_addrlen) == -1) {
			UTIL_LOG_CONSOLE_(LOG_ERR, "Connection failed: '%s'.", strerror(errno));

			close(serverSocketFD);

			serverSocketFD = -1;

			continue;
		}

		break;
	}

	if(sockInfo == NULL){
		UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to connect to '%s:%s'.", "::1", service);

		freeaddrinfo(serverInfo);

		status = EXIT_FAILURE;

		goto label_cleanUp;
	}

	freeaddrinfo(serverInfo);

	// Read incoming data.
	char buffer[8046] = {"This is ony a test. !~!"};

	sleep(1);

	int writtenBytes = write(serverSocketFD, buffer, strlen(buffer) + 1);

	 UTIL_LOG_CONSOLE_(LOG_DEBUG, "Wrote [%d] bytes..", writtenBytes);

	if(writtenBytes <= 0){
		UTIL_LOG_DEBUG("Write error.");
	}

	if(writtenBytes != (int) strlen(buffer) + 1){
		UTIL_LOG_DEBUG("Write error II.");
	}

	// fsync(serverSocketFD);

	int bytesRead = read(serverSocketFD, buffer, 8046);
	buffer[bytesRead] = '\0';

	UTIL_LOG_CONSOLE_(LOG_DEBUG, "Read: '%s'.", buffer);

	// Cleanup.
	close(serverSocketFD);

label_cleanUp:

	closelog();
	
	return status;
}