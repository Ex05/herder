#ifndef HTTP_H
#define HTTP_H

#include "util.h"
#include "linkedList.h"
#include "cache.h"

#define HTTP_VERSION_1_0 "HTTP/1.0"
#define HTTP_VERSION_1_1 "HTTP/1.1"
#define HTTP_VERSION_2_0 "HTTP/2.0"

#define HTTP_ADD_HEADER_FIELD(request, _name, _value) do{ \
    HTTP_HeaderField* headerField ##  _name = malloc(sizeof(*headerField ##  _name)); \
    memset(headerField ##  _name, 0, sizeof(*headerField ##  _name)); \
    const uint_fast64_t headerFieldHostNameLength = strlen(# _name); \
    headerField ##  _name->name = malloc(sizeof(headerField ##  _name->name) * (headerFieldHostNameLength + 1)); \
    if(headerField ##  _name->name == NULL){ \
        UTIL_LOG_ERROR_("%s.", util_toErrorString(ERROR_OUT_OF_MEMORY)); \
    } \
    strncpy(headerField ##  _name->name, # _name, headerFieldHostNameLength + 1); \
    \
    const uint_fast64_t headerField ##  _nameValueLength = strlen(_value); \
    headerField ##  _name->value = malloc(sizeof(headerField ##  _name->value) * (headerField ##  _nameValueLength + 1)); \
    strncpy(headerField ##  _name->value, _value, headerField ##  _nameValueLength + 1); \
    \
    linkedList_add(&request.headerFields, headerField ##  _name);\
    }while(0)

#define HTTP_PROCESSING_BUFFER_SIZE 8192 //16384

#define http_parseRsponseType http_parseRequestType

typedef enum{
    REQUEST_TYPE_UNKNOWN = 0,
    REQUEST_TYPE_GET = 1,
    REQUEST_TYPE_HEAD,
    REQUEST_TYPE_POST,
    REQUEST_TYPE_PUT,
    REQUEST_TYPE_DELETE,
    REQUEST_TYPE_TRACE,
    REQUEST_TYPE_OPTIONS,
    REQUEST_TYPE_CONNECT,
    REQUEST_TYPE_PATCH  
}HTTP_RequestType;

typedef HTTP_RequestType HTTP_ResponseType;

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
    HTTP_CONTENT_TYPE_TEXT_HTML = 0,
    HTTP_CONTENT_TYPE_TEXT_CSS,
    HTTP_CONTENT_TYPE_APPLICATION_JAVASCRIPT,
    HTTP_CONTENT_TYPE_APPLICATION_JSON,
    HTTP_CONTENT_TYPE_APPLICATION_ZIP,
    HTTP_CONTENT_TYPE_IMAGE_JPG,
    HTTP_CONTENT_TYPE_IMAGE_PNG,
    HTTP_CONTENT_TYPE_IMAGE_SVG,
    HTTP_CONTENT_TYPE_IMAGE_ICON
}HTTP_ContentType;

typedef struct{
    LinkedList headerFields;
    char httpVersion[9];
    HTTP_RequestType type;
    char* requestURL;
    uint_fast16_t urlLength;
    int8_t* data;
    uint_fast64_t bufferSize;
    uint_fast64_t dataLength;
    uint_fast64_t getParameterLength;
    char* getParameter;
}HTTP_Request;

typedef struct{
    LinkedList headerFields;
    char httpVersion[9];
    HTTP_ContentType contentType;
    char* statusMsg;
    int_fast16_t statusCode;    
    uint_fast64_t statusMsgLength;
    int8_t* data;
    uint_fast64_t bufferSize;
    uint_fast64_t dataLength;
    bool staticContent;
    CacheObject* cacheObject;
}HTTP_Response;

typedef struct{
    char* name;
    uint_fast64_t nameLength;
    char* value;
    uint_fast64_t valueLength;
}HTTP_HeaderField;
    
ERROR_CODE http_initRequest(HTTP_Request*, const char*, const uint_fast64_t, void*, uint_fast64_t, const char[], const HTTP_RequestType);

ERROR_CODE http_initRequest_(HTTP_Request*, void*, uint_fast64_t, const HTTP_RequestType);

ERROR_CODE http_initResponse(HTTP_Response*, void*, uint_fast64_t);

ERROR_CODE http_initheaderField(HTTP_HeaderField*, char*, const uint_fast64_t, char*,  const uint_fast64_t);
    
ERROR_CODE http_addHeaderField(HTTP_Request*, char*, const uint_fast64_t, char*,  const uint_fast64_t);    
HTTP_HeaderField* http_getHeaderField(HTTP_Request*, char*);
    
void http_freeHTTP_Request(HTTP_Request*);

void http_freeHTTP_Response(HTTP_Response*);

HTTP_RequestType http_parseRequestType(const char* const, const uint_fast64_t);

HTTP_ContentType http_getContentType(const char*, const uint64_t);

const char*  http_getStatusMsg(HTTP_StatusCode);

const char*  http_getRequestType(HTTP_RequestType);

int_fast16_t http_getStatusCode(HTTP_StatusCode);

ERROR_CODE http_sendRequest(HTTP_Request*, HTTP_Response*, const int_fast32_t);

ERROR_CODE http_sendResponse(HTTP_Response*, const int_fast32_t);

ERROR_CODE http_receiveResponse(HTTP_Response*, const int_fast32_t);

ERROR_CODE http_receiveRequest(HTTP_Request*, const int_fast32_t);

HTTP_StatusCode http_translateStatusCode(const int_fast16_t);

ERROR_CODE http_openConnection(int_fast32_t*, const char*, const uint_fast16_t);

ERROR_CODE http_closeConnection(int_fast32_t);

void http_setHTTP_Version(HTTP_Response*, const char[__require 8]);

const char* http_contentTypeToString(const HTTP_ContentType, uint_fast64_t*);

#endif