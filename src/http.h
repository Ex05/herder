#ifndef HTTP_H
#define HTTP_H

#include "util.h"
#include "properties.h"

static Version HTTP_HTTP_VERSION_1_0 = {1, 0, 0};
static Version HTTP_HTTP_VERSION_1_1 = {1, 1, 0};
static Version HTTP_HTTP_VERSION_2_0 = {2, 0, 0};

#define HTTP_ADD_HEADER_FIELD(response, name, value)do{ \
	uint_fast64_t nameLength = strlen(# name); \
	char* _name = malloc(sizeof(*_name) * (nameLength + 1)); \
	strncpy(_name, # name, nameLength + 1); \
	\
	uint_fast64_t valueLength = strlen(value); \
	char* _value = malloc(sizeof(*_value) * (valueLength + 1)); \
	strncpy(_value, value, valueLength + 1); \
	\
	http_addHeaderField((HTTP_Request*) response, _name, nameLength, _value, valueLength); \
}while(0)

typedef enum{
	HTTP_REQUEST_TYPE_UNKNOWN = 0,
	HTTP_REQUEST_TYPE_GET = 1,
	HTTP_REQUEST_TYPE_HEAD,
	HTTP_REQUEST_TYPE_POST,
	HTTP_REQUEST_TYPE_PUT,
	HTTP_REQUEST_TYPE_DELETE,
	HTTP_REQUEST_TYPE_TRACE,
	HTTP_REQUEST_TYPE_OPTIONS,
	HTTP_REQUEST_TYPE_CONNECT,
	HTTP_REQUEST_TYPE_PATCH 
}HTTP_RequestType;

typedef enum{
	// NOTE: Update HTTP_STATUSCODE_MAPPING_ARRAY & HTTP_STATUS_MESSAGE_MAPPING_ARRAY in 'http.c' if you change values in here.
	_100_CONTINUE = 0,
	_101_SWITCHING_PROTOCOLS,
	_102_PROCESSING,
	_200_OK,
	_201_CREATED,
	_202_ACCEPTED,
	_203_NON_AUTHORATIVE_INFORMATION,
	_204_NO_CONTENT,
	_205_RESET_CONTROL,
	_206_PARTIAL_CONTENT,
	_207_MULTI_STATUS,
	_208_ALREADY_REPORTED,
	_226_IM_USED,
	_300_MULTIPLE_CHOICES,
	_301_MOVED_PERMANENTLY,
	_302_FOUND,
	_303_SEE_OTHER,
	_304_NOT_MODIFIED,
	_305_USE_PROXY,
	_307_TEMPORARY_REDIRECT,
	_308_PERMANENT_REDIRECT,
	_400_BAD_REQUEST,
	_401_UNAUTHORIZED,
	_402_PAYMENT_REQUIRED,
	_403_FORBIDDEN,
	_404_NOT_FOUND,
	_405_METHOD_NOT_ALLOWED,
	_406_NOT_ACCEPTABLE,
	_407_PROXY_AUTHENTICATION_REQUIRED,
	_408_REQUEST_TIMEOUT,
	_409_CONFLICT,
	_410_GONE,
	_411_LENGTH_REQUIRED,
	_412_PRECONDITION_FAILED,
	_413_PAYLOAD_TOO_LARGE,
	_414_REQUEST_URI_TOO_LARGE,
	_415_UNSUPPORTED_MEDIA_TYPE,
	_416_REQUEST_RANGE_NOT_SATISFIABLE,
	_417_EXPECTATION_FAILED,
	_418_IM_A_TEAPOT,
	_421_MISDIRECTED_REQUEST,
	_422_UNPROCESSABLE_ENTITY,
	_423_LOCKED,
	_424_FAILED_DEPENDENCY,
	_426_UPGRADE_REQUIRED,
	_428_PRECONDITION_REQUIRED,
	_429_TOO_MANY_REQUESTS,
	_431_REQUEST_HEADER_FIELD_TOO_LARGE,
	_444_CONNECTION_CLOSED_WITHOUT_RESPONSE,
	_451_UNAVAILABLE_FOR_LEAGAL_REASON,
	_499_CLIENT_CLOSED_REQUEST,
	_500_INTERNAL_SERVER_ERROR,
	_501_NOT_IMPLEMENTED,
	_502_BAD_GATEWAY,
	_503_SERVICE_UYNAVAILABLE,
	_504_GATEWAY_TIMEOUT,
	_505_HTTP_VERSION_NOT_SUPPORTED,
	_506_VARIANT_ALSO_NEGOTIATES,
	_507_INSUFFIECENT_STORAGE,
	_508_LOOP_DETECTED,
	_510_NOT_EXTENDED,
	_511_NETWORK_AUTHENTICATION_REQUIRED,
	_599_NETWORK_CONNECTION_TIMEOUT_ERROR
}HTTP_StatusCode;

typedef enum{
	HTTP_CONTENT_TYPE_TEXT_PLAIN = 0,
	HTTP_CONTENT_TYPE_IMAGE_GIF,
	HTTP_CONTENT_TYPE_IMAGE_JPEG,
	HTTP_CONTENT_TYPE_IMAGE_PNG,
	HTTP_CONTENT_TYPE_IMAGE_TIFF,
	HTTP_CONTENT_TYPE_VMD_MICROSOFT_ICON,
	HTTP_CONTENT_TYPE_IMAGE_SVG_XML,
	HTTP_CONTENT_TYPE_TEXT_CSS,
	HTTP_CONTENT_TYPE_TEXT_CSV,
	HTTP_CONTENT_TYPE_TEXT_HTML,
	HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT,
	HTTP_CONTENT_TYPE_TEXT_XML,
	HTTP_CONTENT_TYPE_APPLICATION_ZIP
}HTTP_ContentType;

typedef struct{
	char* name;
	char* value;
	uint_fast64_t nameLength;
	uint_fast64_t valueLength;
}HTTP_HeaderField;

typedef struct{
	LinkedList httpHeaderFields;
	Version httpVersion;
	HTTP_RequestType httpRequestType;
	uint_fast16_t requestURLLength;
	uint_fast16_t requestBufferSize;
	uint_fast16_t requestDataSegmentLength;
	uint_fast16_t getRquestParameterLength;
	char* requestURL;
	char* getRequestParameter;
	int8_t* dataSegment;
}HTTP_Request;

#include "cache.h"

typedef struct{
	LinkedList httpHeaderFields;
	Version httpVersion;
	// Note: (jan - 2022.10.12)
	bool staticContent;
	HTTP_ContentType httpContentType;
	HTTP_StatusCode httpStatusCode;
	uint_fast64_t httpStatusMessageLength;
	uint_fast64_t responseDataSegmentLength;
	uint_fast64_t responseBufferSize;
	int8_t* dataSegment;
	CacheObject* cacheObject;
}HTTP_Response;

ERROR_CODE http_receiveRequest(HTTP_Request*, char[]);

HTTP_StatusCode http_translateStatusCode(const int_fast16_t);

HTTP_RequestType http_parseRequestType(const char* const, const uint_fast64_t);

HTTP_ContentType http_getContentType(const char*, const uint64_t);

const char* http_getStatusMsg(HTTP_StatusCode);

const char* http_requestTypeToString(HTTP_RequestType);

int_fast16_t http_getNumericalStatusCode(HTTP_StatusCode);

void http_initheaderField(HTTP_HeaderField*, char*, const uint_fast64_t, char*, const uint_fast64_t);
 
ERROR_CODE http_addHeaderField(HTTP_Request*, char*, const uint_fast64_t, char*, const uint_fast64_t);

HTTP_HeaderField* http_getHeaderField(HTTP_Request*, char*);

void http_setHTTP_Version(HTTP_Request*, const Version);

const char* http_contentTypeToString(const HTTP_ContentType);

ERROR_CODE http_initRequest(HTTP_Request*, const char*, const uint_fast64_t, void*, uint_fast64_t, const Version, const HTTP_RequestType);

void http_initRequest_(HTTP_Request*, void*, uint_fast64_t, const HTTP_RequestType);

void http_freeHTTP_Request(HTTP_Request*);

ERROR_CODE http_parseHTTP_Request(HTTP_Request*, char*, const uint_fast64_t);

Version http_parseHTTP_Version(const char*, const uint_fast64_t);

const char* http_getVersionString(const Version);

HTTP_StatusCode http_translateErrorCode(const ERROR_CODE);

void http_initHttpResponse(HTTP_Response*, void*, const uint_fast64_t);

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