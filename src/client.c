#include "util.c"
#include "linkedList.c"
#include "arrayList.c"
#include "stringBuilder.c"
#include "util.h"

#include <netdb.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
 #include <openssl/crypto.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(const int argc, const char** argv){
	int status = EXIT_SUCCESS;

	openlog("herder_test", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
	
	// Init context.
	const SSL_METHOD* sslMethod = TLS_client_method();

	SSL_CTX* sslContext = SSL_CTX_new(sslMethod);
	if(sslContext == NULL){
		UTIL_LOG_CONSOLE(LOG_ERR, "Failed to initialise ssl context.");

		status = EXIT_FAILURE;

		goto label_cleanUp;
	}
	
	SSL_CTX_set_min_proto_version(sslContext, TLS1_3_VERSION);

	SSL* sslInstance = SSL_new(sslContext);

	// Create socket.
	struct addrinfo addressHints = {0};
	addressHints.ai_family = AF_INET6;
	addressHints.ai_socktype = SOCK_STREAM;
	
	char service[UTIL_FORMATTED_NUMBER_LENGTH];
	// util_formatNumber(service, UTIL_FORMATTED_NUMBER_LENGTH, port);
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

	// Enable ssl.
	SSL_set_fd(sslInstance, serverSocketFD);
	
	if(SSL_connect(sslInstance) == -1){
		ERR_print_errors_fp(stderr);

		status = EXIT_FAILURE;

		goto label_cleanUp;
	}

	// Read incoming data.
	char buffer[8046] = {0};

	int numBytesRead = SSL_read(sslInstance, buffer, sizeof(buffer));
	if(numBytesRead <= 0){
		UTIL_LOG_CONSOLE(LOG_ERR, "Read error.");
		
		status = EXIT_FAILURE;

		goto label_shutDown;
	}

	printf("'%s'\n", buffer);

label_shutDown:
	SSL_shutdown(sslInstance);

	// Cleanup.
	close(serverSocketFD);

label_cleanUp:
	SSL_free(sslInstance);

	SSL_CTX_free(sslContext);

	closelog();
	
	return status;
}