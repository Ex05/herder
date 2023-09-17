#include "http.h"

inline void http_initheaderField(HTTP_HeaderField* headerField, char* name, const uint_fast64_t nameLength, char* value, const uint_fast64_t valueLength){
	headerField->name = name;
	headerField->nameLength = nameLength;
	headerField->value = value;
	headerField->valueLength = valueLength;
}

inline ERROR_CODE http_initRequest(HTTP_Request* request, const char* url, const uint_fast64_t urlLength, void* buffer, const uint_fast64_t bufferSize, const Version version , const HTTP_RequestType requestType){
	http_initRequest_(request, buffer, bufferSize, requestType);

	http_setHTTP_Version(request, version);

	request->requestURLLength = urlLength;
	request->requestURL = malloc(sizeof(request->requestURL) * (urlLength + 1));
	if(request->requestURL == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	memcpy(request->requestURL, url, urlLength);
	request->requestURL[urlLength] = '\0';

	return ERROR(ERROR_NO_ERROR);
}

inline void http_initRequest_(HTTP_Request* request, void* buffer, const uint_fast64_t bufferSize, const HTTP_RequestType requestType){
	memset(request, 0, sizeof(*request));

	request->dataSegment = buffer;
	request->requestDataSegmentLength = bufferSize;

	request->httpRequestType = requestType;
}

inline void http_initHttpResponse(HTTP_Response* response, void* buffer, const uint_fast64_t bufferSize){
	memset(response, 0, sizeof(*response));

	response->dataSegment = buffer;
	response->responseBufferSize = bufferSize;

	http_setHTTP_Version((HTTP_Request*) response, HTTP_HTTP_VERSION_1_1);

	HTTP_ADD_HEADER_FIELD(response, Server, CONSTANTS_HTTP_HEADER_FIELD_SERVER_VALUE);
}

inline ERROR_CODE http_addHeaderField(HTTP_Request* request, char* name, const uint_fast64_t nameLength, char* value, const uint_fast64_t valueLength){
	
	if(request->httpHeaderFields.length == CONSTANTS_HTTP_MAX_HEADER_FIELDS){
		return ERROR(ERROR_MAX_HEADER_FIELDS_REACHED);
	}

	HTTP_HeaderField* headerField = malloc(sizeof(*headerField));
	if(headerField == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	http_initheaderField(headerField, name, nameLength, value, valueLength);
	
	linkedList_add(&request->httpHeaderFields, &headerField, sizeof(HTTP_HeaderField*));
		
	return ERROR(ERROR_NO_ERROR);
}

inline HTTP_StatusCode http_translateErrorCode(const ERROR_CODE error){
	switch (error) {
		case ERROR_INVALID_REQUEST_URL:{
			return _400_BAD_REQUEST;
		}

		case ERROR_ENTRY_NOT_FOUND:{
			return _404_NOT_FOUND;
		}

	default:{
			return _200_OK;
		}
	}
}

inline HTTP_HeaderField* http_getHeaderField(HTTP_Request* request, char* name){
	HTTP_HeaderField* ret = NULL;
		
	LinkedListIterator it;
	linkedList_initIterator(&it, &request->httpHeaderFields);
	
	const uint_fast64_t nameLength = strlen(name);
	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT_PTR(&it, HTTP_HeaderField);

		if(strncasecmp(name, headerField->name, nameLength) == 0){
			ret = headerField;

			break;
		}
	}

	return ret;
}

void http_freeHTTP_Request(HTTP_Request* request){
	LinkedListIterator it;
	linkedList_initIterator(&it, &request->httpHeaderFields);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT_PTR(&it, HTTP_HeaderField);

		free(headerField->name);
		free(headerField->value);
		free(headerField);
	}

	free(request->requestURL);
	
	linkedList_free(&request->httpHeaderFields);
}

HTTP_StatusCode http_translateStatusCode(const int_fast16_t statusCode){
	switch(statusCode){
	case 100:{
		return _100_CONTINUE;
		}
	case 101:{
		return _101_SWITCHING_PROTOCOLS;
		}
	case 102:{
		return _102_PROCESSING;
		}
	case 200:{
		return _200_OK;
		}
	case 201:{
		return _201_CREATED;
		}
	case 202:{
		return _202_ACCEPTED;
		}
	case 203:{
		return _203_NON_AUTHORATIVE_INFORMATION;
		}
	case 204:{
		return _204_NO_CONTENT;
		}
	case 205:{
		return _205_RESET_CONTROL;
		}
	case 206:{
		return _206_PARTIAL_CONTENT;
		}
	case 207:{
		return _207_MULTI_STATUS;
		}
	case 208:{
		return _208_ALREADY_REPORTED;
		}
	case 226:{
		return _226_IM_USED;
		}
	case 300:{
		return _300_MULTIPLE_CHOICES;
		}
	case 301:{
		return _301_MOVED_PERMANENTLY;
		}
	case 302:{
		return _302_FOUND;
		}
	case 303:{
		return _303_SEE_OTHER;
		}
	case 304:{
		return _304_NOT_MODIFIED;
		}
	case 305:{
		return _305_USE_PROXY;
		}
	case 307:{
		return _307_TEMPORARY_REDIRECT;
		}
	case 308:{
		return _308_PERMANENT_REDIRECT;
		}
	case 400:{
		return _400_BAD_REQUEST;
		}
	case 401:{
		return _401_UNAUTHORIZED;
		}
	case 402:{
		return _402_PAYMENT_REQUIRED;
		}
	case 403:{
		return _403_FORBIDDEN;
		}
	case 404:{
		return _404_NOT_FOUND;
		}
	case 405:{
		return _405_METHOD_NOT_ALLOWED;
		}
	case 406:{
		return _406_NOT_ACCEPTABLE;
		}
	case 407:{
		return _407_PROXY_AUTHENTICATION_REQUIRED;
		}
	case 408:{
		return _408_REQUEST_TIMEOUT;
		}
	case 409:{
		return _409_CONFLICT;
		}
	case 410:{
		return _410_GONE;
		}
	case 411:{
		return _411_LENGTH_REQUIRED;
		}
	case 412:{
		return _412_PRECONDITION_FAILED;
		}
	case 413:{
		return _413_PAYLOAD_TOO_LARGE;
		}
	case 414:{
		return _414_REQUEST_URI_TOO_LARGE;
		}
	case 415:{
		return _415_UNSUPPORTED_MEDIA_TYPE;
		}
	case 416:{
		return _416_REQUEST_RANGE_NOT_SATISFIABLE;
		}
	case 417:{
		return _417_EXPECTATION_FAILED;
		}
	case 418:{
		return _418_IM_A_TEAPOT;
		}
	case 421:{
		return _421_MISDIRECTED_REQUEST;
		}
	case 422:{
		return _422_UNPROCESSABLE_ENTITY;
		}
	case 423:{
		return _423_LOCKED;
		}
	case 424:{
		return _424_FAILED_DEPENDENCY;
		}
	case 426:{
		return _426_UPGRADE_REQUIRED;
		}
	case 428:{
		return _428_PRECONDITION_REQUIRED;
		}
	case 429:{
		return _429_TOO_MANY_REQUESTS;
		}
	case 431:{
		return _431_REQUEST_HEADER_FIELD_TOO_LARGE;
		}
	case 444:{
		return _444_CONNECTION_CLOSED_WITHOUT_RESPONSE;
		}
	case 451:{
		return _451_UNAVAILABLE_FOR_LEAGAL_REASON;
		}
	case 499:{
		return _499_CLIENT_CLOSED_REQUEST;
		}
	case 500:{
		return _500_INTERNAL_SERVER_ERROR;
		}
	case 501:{
		return _501_NOT_IMPLEMENTED;
		}
	case 502:{
		return _502_BAD_GATEWAY;
		}
	case 503:{
		return _503_SERVICE_UYNAVAILABLE;
		}
	case 504:{
		return _504_GATEWAY_TIMEOUT;
		}
	case 505:{
		return _505_HTTP_VERSION_NOT_SUPPORTED;
		}
	case 506:{
		return _506_VARIANT_ALSO_NEGOTIATES;
		}
	case 507:{
		return _507_INSUFFIECENT_STORAGE;
		}
	case 508:{
		return _508_LOOP_DETECTED;
		}
	case 510:{
		return _510_NOT_EXTENDED;
		}
	case 511:{
		return _511_NETWORK_AUTHENTICATION_REQUIRED;
		}
	case 599:{
		return _599_NETWORK_CONNECTION_TIMEOUT_ERROR;
		}
	default:{
		UTIL_LOG_CRITICAL_("[%" PRIdFAST16"] Unsupported HTTP status code.", statusCode);

		return _500_INTERNAL_SERVER_ERROR;
		}
	}
}

/*
	printf("#define HASH_GET %d\n", util_hashString("GET"));
	printf("#define HASH_HEAD %d\n", util_hashString("HEAD"));
	printf("#define HASH_POST %d\n", util_hashString("POST"));
	printf("#define HASH_PUT %d\n", util_hashString("PUT"));
	printf("#define HASH_DELETE %d\n", util_hashString("DELETE"));
	printf("#define HASH_TRACE %d\n", util_hashString("TRACE"));
	printf("#define HASH_OPTIONS %d\n", util_hashString("OPTIONS"));
	printf("#define HASH_CONNECT %d\n", util_hashString("CONNECT"));
	printf("#define HASH_PATCH %d\n", util_hashString("PATCH"));
*/
HTTP_RequestType http_parseRequestType(const char* const type, const uint_fast64_t length){
	#define HASH_GET 591093270
	#define HASH_HEAD 942011328
	#define HASH_POST -1319429824
	#define HASH_PUT 666496399
	#define HASH_DELETE 617851755
	#define HASH_TRACE 1359153669
	#define HASH_OPTIONS -1168929634
	#define HASH_CONNECT -431736502
	#define HASH_PATCH 867751912

	HTTP_RequestType ret = HTTP_REQUEST_TYPE_UNKNOWN;

	switch(util_hashString(type, length)){
		case HASH_GET:{
			ret = HTTP_REQUEST_TYPE_GET;

			break;
		}
		case HASH_POST:{
			ret = HTTP_REQUEST_TYPE_POST;

			break;
		}
		case HASH_HEAD:{
			ret = HTTP_REQUEST_TYPE_HEAD;

			break;
		}
		case HASH_PUT:{
			ret = HTTP_REQUEST_TYPE_PUT;

			break;
		}
		case HASH_DELETE:{
			ret = HTTP_REQUEST_TYPE_DELETE;

			break;
		}
		case HASH_TRACE:{
			ret = HTTP_REQUEST_TYPE_TRACE;

			break;
		}
		case HASH_OPTIONS:{
			ret = HTTP_REQUEST_TYPE_OPTIONS;

			break;
		}
		case HASH_CONNECT:{
			ret = HTTP_REQUEST_TYPE_CONNECT;

			break;
		}
		case HASH_PATCH:{
			ret = HTTP_REQUEST_TYPE_PATCH;

			break;
		}
		default:{
			break;
		}
	}

	#undef HASH_GET
	#undef HASH_HEAD
	#undef HASH_POST
	#undef HASH_PUT
	#undef HASH_DELETE
	#undef HASH_TRACE
	#undef HASH_OPTIONS
	#undef HASH_CONNECT
	#undef HASH_PATCH

	return ret;
}

 scope_local const char* const HTTP_STATUS_MESSAGE_MAPPING_ARRAY[] = {
	"Continue",
	"Switching Protocols",
	"Processing",
	"OK",
	"Created",
	"Accepted",
	"Non-authoritative Information",
	"No Content",
	"Reset Content",
	"Partial Content",
	"Multi-Status",
	"Already Reported",
	"IM Used",
	"Multiple Choices",
	"Moved Permanently",
	"Found",
	"See Other",
	"Not Modified",
	"Use Proxy",
	"Temporary Redirect",
	"Permanent Redirect",
	"Bad Request",
	"Unauthorized",
	"Payment Required",
	"Forbidden",
	"Not Found",
	"Method Not Allowed",
	"Not Acceptable",
	"Proxy Authentication Required",
	"Request Timeout",
	"Conflict",
	"Gone",
	"Length Required",
	"Precondition Failed",
	"Payload Too Large",
	"Request-URI Too Long",
	"Unsupported Media Type",
	"Requested Range Not Satisfiable",
	"Expectation Failed",
	"I'm a teapot",
	"Misdirected Request",
	"Unprocessable Entity",
	"Locked",
	"Failed Dependency",
	"Upgrade Required",
	"Precondition Required",
	"Too Many Requests",
	"Request Header Fields Too Large",
	"Connection Closed Without Response",
	"Unavailable For Legal Reasons",
	"Client Closed Request",
	"Internal Server Error",
	"Not Implemented",
	"Bad Gateway",
	"Service Unavailable",
	"Gateway Timeout",
	"HTTP Version Not Supported",
	"Variant Also Negotiates",
	"Insufficient Storage",
	"Loop Detected",
	"Not Extended",
	"Network Authentication Required",
	"Network Connect Timeout Error"
};

inline const char* http_getStatusMsg(HTTP_StatusCode statusCode){
	return HTTP_STATUS_MESSAGE_MAPPING_ARRAY[statusCode];
}

 scope_local const char* HTTP_REQUEST_TYPE_MAPPING_ARRAY[] = {
	"UNKNOWN",
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"OPTIONS",
	"CONNECT",
	"PATCH"
};

inline const char* http_requestTypeToString(HTTP_RequestType requestType){
	return HTTP_REQUEST_TYPE_MAPPING_ARRAY[requestType];
}

 scope_local const int_fast16_t HTTP_STATUS_CODE_MAPPING_ARRAY[] = {
	100,
	101,
	102,
	200,
	201,
	202,
	203,
	204,
	205,
	206,
	207,
	208,
	226,
	300,
	301,
	302,
	303,
	304,
	305,
	307,
	308,
	400,
	401,
	402,
	403,
	404,
	405,
	406,
	407,
	408,
	409,
	410,
	411,
	412,
	413,
	414,
	415,
	416,
	417,
	418,
	421,
	422,
	423,
	424,
	426,
	428,
	429,
	431,
	444,
	451,
	499,
	500,
	501,
	502,
	503,
	504,
	505,
	506,
	507,
	508,
	510,
	511,
	599
};

inline int_fast16_t http_getNumericalStatusCode(HTTP_StatusCode statusCode){
	return HTTP_STATUS_CODE_MAPPING_ARRAY[statusCode];
}

 scope_local const char* HTTP_VERSION_STRING_MAPPING_ARRAY[] = {
	"HTTP/1.0",
	"HTTP/1.1",
	"HTTP/2.0",
	"HTTP/2.1"
};

inline const char* http_getVersionString(const Version version){
	if(version.release == 1 && version.update == 0){
		return HTTP_VERSION_STRING_MAPPING_ARRAY[0];
	}else if(version.release == 1 && version.update == 1){
		return HTTP_VERSION_STRING_MAPPING_ARRAY[1];
	}else if(version.release == 1 && version.update == 0){
		return HTTP_VERSION_STRING_MAPPING_ARRAY[0];
	}else if(version.release == 1 && version.update == 0){
		return HTTP_VERSION_STRING_MAPPING_ARRAY[0];
	}else{
		// Just hope for the best, version '1.1' should always be supported I guess.
		return HTTP_VERSION_STRING_MAPPING_ARRAY[1];
	}
}

inline void http_setHTTP_Version(HTTP_Request* request, const Version version){
	request->httpVersion.release = version.release;
	request->httpVersion.update = version.update;
}

typedef struct{
	const char* name;
	const uint_fast64_t length;
}ContentType;

 scope_local const char* HTTP_CONTENT_TYPE_MAPPING_ARRAY[] = {
	"text/plain",
	"image/gif",
	"image/jpeg",
	"image/png",
	"image/tiff",
	"image/vnd.microsoft.icon",
	"image/svg+xml",
	"text/css",
	"text/csv",
	"text/html",
	"text/javascript",
	"text/xml",
	"application/zip",
};

inline const char* http_contentTypeToString(const HTTP_ContentType contentType){
	return HTTP_CONTENT_TYPE_MAPPING_ARRAY[contentType];
}

// printf("#define HASH_TXT %d\n", util_hashString("txt", 3));
// printf("#define HASH_HTML %d\n", util_hashString("html", 4));
// printf("#define HASH_CSS %d\n", util_hashString("css", 3));
// printf("#define HASH_JS %d\n", util_hashString("js", 2));
// printf("#define HASH_JPG %d\n", util_hashString("jpg", 3));
// printf("#define HASH_PNG %d\n", util_hashString("png", 3));
// printf("#define HASH_SVG %d\n", util_hashString("svg", 3));
// printf("#define HASH_ICO %d\n", util_hashString("ico", 3));
// printf("#define HASH_ZIP %d\n", util_hashString("zip", 3));
// printf("#define HASH_GIF %d\n", util_hashString("gif", 3));
// printf("#define HASH_XML %d\n", util_hashString("xml", 3));
// printf("#define HASH_TIF %d\n", util_hashString("tif", 3));
// printf("#define HASH_TIFF %d\n", util_hashString("tiff", 4));
// printf("#define HASH_CSV %d\n", util_hashString("csv", 3));

inline HTTP_ContentType http_getContentType(const char* fileExtension, const uint_fast64_t fileExtensionLength){
	#define HASH_TXT 966206576
	#define HASH_HTML 542175051
	#define HASH_CSS 825432995
	#define HASH_JS 6953609
	#define HASH_JPG 883066721
	#define HASH_PNG 932504553
	#define HASH_SVG 957813860
	#define HASH_ICO 873952437
	#define HASH_ZIP 1014791617
	#define HASH_GIF 857823012
	#define HASH_XML 998530999
	#define HASH_TIF 965222577
	#define HASH_TIFF 1227951093
	#define HASH_CSV 825432998

	switch(util_hashString(fileExtension, fileExtensionLength)){

	case HASH_CSS:{
		return HTTP_CONTENT_TYPE_TEXT_CSS;
	}
	case HASH_JS:{
		return HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT;
	}
	case HASH_JPG:{
		return HTTP_CONTENT_TYPE_IMAGE_JPEG;
	}
	case HASH_PNG:{
		return HTTP_CONTENT_TYPE_IMAGE_PNG;		
	}
	case HASH_SVG:{
		return HTTP_CONTENT_TYPE_IMAGE_SVG_XML;
	}
	case HASH_ICO:{
		return HTTP_CONTENT_TYPE_VMD_MICROSOFT_ICON;
	}
	case HASH_ZIP:{
		return HTTP_CONTENT_TYPE_APPLICATION_ZIP;
	}
	case HASH_GIF:{
		return HTTP_CONTENT_TYPE_IMAGE_GIF;
	}
	case HASH_XML:{
		return HTTP_CONTENT_TYPE_TEXT_XML;
	}
	case HASH_TIF:
	case HASH_TIFF:{
		return HTTP_CONTENT_TYPE_IMAGE_TIFF;
	}
	case HASH_CSV:{
		return HTTP_CONTENT_TYPE_TEXT_CSV;
	}
	case HASH_HTML:
	case HASH_TXT:{
		return HTTP_CONTENT_TYPE_TEXT_HTML;
	}

	default:{
		return HTTP_CONTENT_TYPE_TEXT_PLAIN;

		break;
	}
 }

	#undef HASH_TXT
	#undef HASH_HTML
	#undef HASH_CSS
	#undef HASH_JS
	#undef HASH_JPG
	#undef HASH_PNG
	#undef HASH_SVG
	#undef HASH_ICO
	#undef HASH_ZIP
	#undef HASH_GIF
	#undef HASH_XML
	#undef HASH_TIF
	#undef HASH_TIFF
	#undef HASH_CUR
	#undef HASH_CSV
}

// printf("#define HASH_1_0 %d\n", util_hashString(CONSTANTS_HTTP_VERSION_1_0, strlen(CONSTANTS_HTTP_VERSION_1_0)));
// printf("#define HASH_1_1 %d\n", util_hashString(CONSTANTS_HTTP_VERSION_1_1, strlen(CONSTANTS_HTTP_VERSION_1_1)));
// printf("#define HASH_2_0 %d\n", util_hashString(CONSTANTS_HTTP_VERSION_2_0, strlen(CONSTANTS_HTTP_VERSION_2_0)));
Version http_parseHTTP_Version(const char* version, const uint_fast64_t versionStringLength){
	#define HASH_1_0 1183657804
	#define HASH_1_1 1183657805
	#define HASH_2_0 1191919309

	switch (util_hashString(version, versionStringLength)){
		case HASH_1_0:{
			return HTTP_HTTP_VERSION_1_0;
		}

		case HASH_1_1:{
			return HTTP_HTTP_VERSION_1_1;
		}

		case HASH_2_0:{
			return HTTP_HTTP_VERSION_2_0;
		}

		default:{
			Version ret = {0, 0, 0};

			return ret;
		}
	}

	#undef HASH_1_0
	#undef HASH_1_1
	#undef HASH_2_0
}

ERROR_CODE http_parseHTTP_Request(HTTP_Request* request, char* httpProcessingBuffer, const uint_fast64_t httpProcessingBufferSize){
	// TODO: Replace with settings/property file entry for http processing buffer size. (jan - 2022.09.05)
 	if(httpProcessingBufferSize >= 8096){
		return ERROR(ERROR_MAX_MESSAGE_SIZE_EXCEEDED);
	}else if(httpProcessingBufferSize < 3/*RequestType*/ + 3/*Spaces*/ + 2/*Line delimiter*/ + 1/*RequestURL*/ + 8/*Version*/){
		return ERROR(ERROR_INSUFICIENT_MESSAGE_LENGTH);
	}

	// RequestLine.
	uint_fast64_t i;
	for(i = 1; i < httpProcessingBufferSize; i++){
		// Find line break/end of line.
		if(httpProcessingBuffer[i - 1] == '\n' && httpProcessingBuffer[i - 2] == '\r'){
			int_fast64_t posSplitBegin = 0;
			int_fast64_t posSplitEnd;

			// RequestType.
			posSplitEnd = util_findFirst(httpProcessingBuffer, httpProcessingBufferSize, ' ');

			if(posSplitEnd == -1){
				return ERROR(ERROR_INVALID_HEADER_FIELD);
			}

			const char* const requestType = httpProcessingBuffer;
			const uint_fast64_t requestTypeLength = posSplitEnd;

			request->httpRequestType = http_parseRequestType(requestType, requestTypeLength);

			// Reset search location to after request type.
			posSplitBegin = posSplitEnd + 1;

			// RequestURL.
			posSplitEnd = util_findFirst(httpProcessingBuffer + posSplitBegin, httpProcessingBufferSize - posSplitBegin, ' ');

			if(posSplitEnd == -1){
				return ERROR(ERROR_INVALID_HEADER_FIELD);
			}

			request->requestURLLength = posSplitEnd;

			if(request->requestURLLength == 0){
				return ERROR_(ERROR_INVALID_REQUEST_URL, "URL length can't be of length '%" PRIuFAST64 "'.", request->requestURLLength);
			}

			request->requestURL = malloc(sizeof(*request->requestURL) * request->requestURLLength + 1);
			if(request->requestURL == NULL){
			 	return ERROR(ERROR_OUT_OF_MEMORY);
			}

			memcpy(request->requestURL, httpProcessingBuffer + posSplitBegin, request->requestURLLength);
			request->requestURL[request->requestURLLength] = '\0';

			// Reset search location to after request url.
			posSplitBegin += posSplitEnd + 1;

			const int_fast64_t getRequestParameterOffset = util_findFirst(request->requestURL, request->requestURLLength, '?');

			// If we have additional get parameters sent via the url.
			if(getRequestParameterOffset != -1){
				request->requestURL[getRequestParameterOffset] = '\0';

				request->getRquestParameterLength = request->requestURLLength - getRequestParameterOffset;
				request->requestURLLength = getRequestParameterOffset;

				request->getRequestParameter = request->requestURL + (getRequestParameterOffset + 1);
			}

			// Version.
			const char* const version = httpProcessingBuffer + posSplitBegin;
			const uint_fast64_t versionLength = i/*Line length*/ - posSplitBegin - 2/*'\r\n'*/;

			if(strncmp(version, CONSTANTS_HTTP_VERSION_1_1, versionLength) != 0){
				return ERROR(ERROR_VERSION_MISSMATCH);
			}else{
				http_setHTTP_Version(request, HTTP_HTTP_VERSION_1_1);
			}

			break;
		}
	}

	// Header fields.
	uint_fast64_t posLineBegin = i;
	// For each line in the http request.
	for(;i < httpProcessingBufferSize; i++){
		// Find line break/end of line.
		if(httpProcessingBuffer[i] == '\n' && httpProcessingBuffer[i - 1] == '\r'){
			if(httpProcessingBuffer[i - 2] == '\n' && httpProcessingBuffer[i - 3] == '\r'){
				// Skip trailing '\n'.
				i += 1;

				break;
			}

			char* line = httpProcessingBuffer + posLineBegin;
			uint_fast64_t lineLength = i - posLineBegin;

			// Header field name.
			const int_fast64_t nameLength = util_findFirst(line, lineLength,':');
			if(nameLength == -1){
				return ERROR(ERROR_INVALID_HEADER_FIELD);
			}

			char* name = malloc(sizeof(*name) * (nameLength + 1));
			if(name == NULL){
				return ERROR(ERROR_OUT_OF_MEMORY);
			}

			memcpy(name, line, nameLength);
			name[nameLength] = '\0';

			// Move line beginning after the ':' seperator.
			line += nameLength + 1;
			lineLength -= nameLength + 1;

			// Skip trailing whitespace
			if(isspace(*line) != 0){
				line += 1;
				lineLength -= 1;
			}

			// Header field value.
			const int_fast64_t valueLength = lineLength;
			if(valueLength == -1){
				return ERROR(ERROR_INVALID_HEADER_FIELD);
			}

			char* value = malloc(sizeof(*value) * (valueLength + 1));
			if(value == NULL){
				return ERROR(ERROR_OUT_OF_MEMORY);
			}

			memcpy(value, line, valueLength);
			value[valueLength] = '\0';

			http_addHeaderField(request, name, nameLength, value, valueLength);

			posLineBegin = i + 1;
		}
	}

	// Data.
	request->dataSegment = (int8_t*) (httpProcessingBuffer + i);

	return ERROR(ERROR_NO_ERROR);
}