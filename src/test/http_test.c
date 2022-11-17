#ifndef HTTP_TEST_C
#define HTTP_TEST_C

#include "../test.c"
#include <string.h>
#include <sys/syslog.h>

TEST_TEST_FUNCTION(http_initheaderField){
	char headerFieldName[] = "Host";
	const uint_fast64_t headerFieldNameLength = strlen(headerFieldName);
	char headerFieldValue[] = "localhost";
	const uint_fast64_t headerFieldValueLength = strlen(headerFieldName);

	HTTP_HeaderField headerField = {0};
	http_initheaderField(&headerField, headerFieldName, headerFieldNameLength, headerFieldValue ,headerFieldValueLength);

	if(strncmp(headerFieldName, headerField.name, headerFieldNameLength) != 0){
		return TEST_FAILURE("Header field name '%s' != '%s'.", headerFieldName, headerField.name);
	}

	if(strncmp(headerFieldValue, headerField.value, headerFieldValueLength) != 0){
		return TEST_FAILURE("Header field name '%s' != '%s'.", headerFieldValue, headerField.value);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(http_addHeaderField){
	HTTP_Request request;
	http_initRequest_(&request, NULL, 0, HTTP_REQUEST_TYPE_UNKNOWN);

	{
		char name[] = "Host";
		const uint_fast64_t headerFieldNameLength = strlen(name);
		char value[] = "localhost";
		const uint_fast64_t headerFieldValueLength = strlen(value);

		char* headerFieldName = malloc(sizeof(*headerFieldName) * (headerFieldNameLength + 1));
		strncpy(headerFieldName, name, headerFieldNameLength + 1);

		char* headerFieldValue = malloc(sizeof(*headerFieldValue) * (headerFieldValueLength + 1));
		strncpy(headerFieldValue, value, headerFieldValueLength + 1);

		if(http_addHeaderField(&request, headerFieldName, headerFieldNameLength, headerFieldValue, headerFieldValueLength) != ERROR_NO_ERROR){
			return TEST_FAILURE("%s", "Failed to add header field to http request.");
		}

		if(request.httpHeaderFields.length != 1){
			return TEST_FAILURE("%s", "Failed to add header field to http request.");
		}

		HTTP_HeaderField* headerField = http_getHeaderField(&request, name);

		if(headerField == NULL){
			return TEST_FAILURE("Failed to retrieve Header field '%s'.", name);
		}

		if(strncmp(name, headerField->name, headerFieldNameLength + 1) != 0){
			return TEST_FAILURE("HeaderField name '%s' != '%s'.", name, headerField->name);
		}

		if(strncmp(value, headerField->value, headerFieldValueLength + 1) != 0){
			return TEST_FAILURE("HeaderField name '%s' != '%s'.", value, headerField->name);
		}
	}

	{
		HTTP_ADD_HEADER_FIELD(&request, Content-Length, "128");

		#define HEADER_FIELD_NAME "Content-Length"
		HTTP_HeaderField* headerField = http_getHeaderField(&request, HEADER_FIELD_NAME);

		if(headerField == NULL){
			return TEST_FAILURE("Failed to retrieve Header field '%s'.", HEADER_FIELD_NAME);
		}

		if(strncmp(HEADER_FIELD_NAME, headerField->name, strlen(HEADER_FIELD_NAME) + 1) != 0){
			return TEST_FAILURE("HeaderField name '%s' != '%s'.", HEADER_FIELD_NAME, headerField->name);
		}

		if(strncmp("128", headerField->value, strlen("128") + 1) != 0){
			return TEST_FAILURE("HeaderField name '%s' != '%s'.", "128", headerField->name);
		}
		#undef HEADER_FIELD_NAME
	}

	http_freeHTTP_Request(&request);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(http_getHeaderField){
	char name[] = "Host";
	const uint_fast64_t headerFieldNameLength = strlen(name);
	
	char value[] = "localhost";
	const uint_fast64_t headerFieldValueLength = strlen(value);

	char* headerFieldName = malloc(sizeof(*headerFieldName) * (headerFieldNameLength + 1));
	strncpy(headerFieldName, name, headerFieldNameLength + 1);

	char* headerFieldValue = malloc(sizeof(*headerFieldValue) * (headerFieldValueLength + 1));
	strncpy(headerFieldValue, value, headerFieldValueLength + 1);

	HTTP_Request request;
	http_initRequest_(&request, NULL, 0, HTTP_REQUEST_TYPE_UNKNOWN);

	if(http_addHeaderField(&request, headerFieldName, headerFieldNameLength, headerFieldValue, headerFieldValueLength) != ERROR_NO_ERROR){
		return TEST_FAILURE("%s", "Failed to add header field to http request.");
	}

	HTTP_HeaderField* headerFieldHost = http_getHeaderField(&request, "Host");

	if(headerFieldHost == NULL){
		return TEST_FAILURE("%s", "Failed to retrieve header field from http request.");
	}

	if(strncmp(headerFieldHost->name, headerFieldName, headerFieldNameLength) !=0){
		return TEST_FAILURE("%s", "Failed to retrieve header field from http request.");
	}

	http_freeHTTP_Request(&request);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(HTTP_contentTypeToString){
	const char* contentType = http_contentTypeToString(HTTP_CONTENT_TYPE_TEXT_HTML);

	if(strncmp(contentType, "text/html", strlen(contentType) + 1) != 0){
		return TEST_FAILURE("'%s' != 'text/html'.", contentType);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(http_parseRequestType){
	if(http_parseRequestType("GET", 3) != HTTP_REQUEST_TYPE_GET){
		return TEST_FAILURE("%s", "Failed to parse 'GET' as 'HTTP_REQUEST_TYPE_GET'.");
	}

	if(http_parseRequestType("POST", 4) != HTTP_REQUEST_TYPE_POST){
		return TEST_FAILURE("%s", "Failed to parse 'POST' as 'HTTP_REQUEST_TYPE_POST'.");
	}

	if(http_parseRequestType("HEAD", 4) != HTTP_REQUEST_TYPE_HEAD){
		return TEST_FAILURE("%s", "Failed to parse 'HEAD' as 'HTTP_REQUEST_TYPE_HEAD'.");
	}
	
	if(http_parseRequestType("PUT", 3) != HTTP_REQUEST_TYPE_PUT){
		return TEST_FAILURE("%s", "Failed to parse 'PUT' as 'HTTP_REQUEST_TYPE_PUT'.");
	}
	
	if(http_parseRequestType("DELETE", 6) != HTTP_REQUEST_TYPE_DELETE){
		return TEST_FAILURE("%s", "Failed to parse 'DELETE' as 'HTTP_REQUEST_TYPE_DELETE'.");
	}
	
	if(http_parseRequestType("OPTIONS", 7) != HTTP_REQUEST_TYPE_OPTIONS){
		return TEST_FAILURE("%s", "Failed to parse 'OPTIONS' as 'HTTP_REQUEST_TYPE_OPTIONS'.");
	}
	
	if(http_parseRequestType("CONNECT", 7) != HTTP_REQUEST_TYPE_CONNECT){
		return TEST_FAILURE("%s", "Failed to parse 'CONNECT' as 'HTTP_REQUEST_TYPE_CONNECT'.");
	}
	
	if(http_parseRequestType("PATCH", 5) != HTTP_REQUEST_TYPE_PATCH){
		return TEST_FAILURE("%s", "Failed to parse 'PATCH' as 'HTTP_REQUEST_TYPE_PATCH'.");
	}
	
	if(http_parseRequestType("UNKNOWN", 7) != HTTP_REQUEST_TYPE_UNKNOWN){
		return TEST_FAILURE("%s", "Failed to parse 'UNKNOWN' as 'HTTP_REQUEST_TYPE_UNKNOWN'.");
	}
	
	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(http_parseHTTP_Request){
	char requestString[] = "GET /index.html HTTP/1.1\r\nHost: localhost:1869\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:99.0) Gecko/20100101 Firefox/99.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,/;q=0.8\r\nAccept-Language: en-US,en;q=0.5\r\nAccept-Encoding: gzip, deflate, br\r\nDNT: 1\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nSec-Fetch-Dest: document\r\nSec-Fetch-Mode: navigate\r\nSec-Fetch-Site: none\r\nSec-Fetch-User: ?1\r\nSec-GPC: 1\r\n";

	HTTP_Request request;
	http_initRequest_(&request, NULL, 0, 0);

	ERROR_CODE error;
	if((error = http_parseHTTP_Request(&request, requestString, strlen(requestString))) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to parse http request. '%s'.", util_toErrorString(error));
	}

	if(request.httpRequestType != HTTP_REQUEST_TYPE_GET){
		return TEST_FAILURE("Failed to parse http request type. '%s' != '%s'.", http_requestTypeToString(request.httpRequestType), http_requestTypeToString(HTTP_REQUEST_TYPE_GET));
	}

	if(strncmp("/index.html", request.requestURL, request.requestURLLength) != 0){
		return TEST_FAILURE("Failed to parse http request type. '%s' != '%s'.", request.requestURL, "/index.html");
	}

	if(request.httpVersion.release != 1 && request.httpVersion.update != 1){
		return TEST_FAILURE("Failed to parse http version. '%" PRIuFAST8 ".%" PRIuFAST8 "' != '1.1.'", request.httpVersion.release, request.httpVersion.update);
	}

	http_freeHTTP_Request(&request);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(http_parseHTTP_Version){
	Version version = http_parseHTTP_Version(CONSTANTS_HTTP_VERSION_1_0, strlen(CONSTANTS_HTTP_VERSION_1_0));
	if(version.release != 1 && version.update != 0){
		return TEST_FAILURE("%s", "Failed to parese version.");
	}

	version = http_parseHTTP_Version(CONSTANTS_HTTP_VERSION_1_1, strlen(CONSTANTS_HTTP_VERSION_1_1));
	if(version.release != 1 && version.update != 1){
		return TEST_FAILURE("%s", "Failed to parese version.");
	}

	version = http_parseHTTP_Version(CONSTANTS_HTTP_VERSION_2_0, strlen(CONSTANTS_HTTP_VERSION_2_0));
	if(version.release != 2 && version.update != 0){
		return TEST_FAILURE("%s", "Failed to parese version.");
	}

	return TEST_SUCCESS;
}

#endif