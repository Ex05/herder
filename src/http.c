#ifndef HTTP_C
#define HTTP_C

#include "http.h"

#include "linkedList.c"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>

inline ERROR_CODE http_initRequest(HTTP_Request* request, const char* url, const uint_fast64_t urlLength, void* buffer, const uint_fast64_t bufferSize, const char version[] , const HTTP_RequestType requestType){
    ERROR_CODE error;
    error = http_initRequest_(request, buffer, bufferSize, requestType);

    http_setHTTP_Version((HTTP_Response*)(request), version);

    request->urlLength = urlLength;
    request->requestURL = malloc(sizeof(request->requestURL) * (urlLength + 1));
    if(request->requestURL == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    memcpy(request->requestURL, url, urlLength);
    request->requestURL[urlLength] = '\0';

    return ERROR(error);
}

inline ERROR_CODE http_initRequest_(HTTP_Request* request, void* buffer, const uint_fast64_t bufferSize, const HTTP_RequestType requestType){
    memset(request, 0, sizeof(*request));
    
    request->data = buffer;    
    request->bufferSize = bufferSize;    

    ERROR_CODE error;
    error = linkedList_init(&request->headerFields);

    request->type = requestType;

    return ERROR(error);
}

inline ERROR_CODE http_initResponse(HTTP_Response* response, void* buffer, const uint_fast64_t bufferSize){    
    memset(response, 0, sizeof(*response));
    
    response->data = buffer;    
    response->bufferSize = bufferSize;    

    ERROR_CODE error;
    error = linkedList_init(&response->headerFields);
                
    response->statusCode = -1;

    return ERROR(error);
}

inline ERROR_CODE http_initheaderField(HTTP_HeaderField* headerField, char* name, const uint_fast64_t nameLength, char* value, const uint_fast64_t valueLength){
    headerField->name = name;    
    headerField->nameLength = nameLength;    
    headerField->value = value;
    headerField->valueLength = valueLength;

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE http_addHeaderField(HTTP_Request* request, char* name, const uint_fast64_t nameLength, char* value, const uint_fast64_t valueLength){
    #define HTTP_MAX_HEADER_FIELDS 32
    if(request->headerFields.length == HTTP_MAX_HEADER_FIELDS){
        return ERROR(ERROR_MAX_HEADER_FIELDS_REACHED);
    }
    
    HTTP_HeaderField* headerField = malloc(sizeof(*headerField));
    if(headerField == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    http_initheaderField(headerField, name, nameLength, value, valueLength);
    
    linkedList_add(&request->headerFields, headerField);
    
    #undef HTTP_MAX_HEADER_FIELDS
    
    return ERROR(ERROR_NO_ERROR);
}

inline HTTP_HeaderField* http_getHeaderField(HTTP_Request* request, char* name){
    HTTP_HeaderField* ret = NULL;
        
    LinkedListIterator it;
    linkedList_initIterator(&it, &request->headerFields);    
    
    const uint_fast64_t nameLength = strlen(name);
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT(&it);
        
        if(strncasecmp(name, headerField->name, nameLength) == 0){
            ret = headerField;
            
            break;
        }
    }       
        
    return ret;
}

inline ERROR_CODE http_closeConnection(int_fast32_t sockFD){
    if(close(sockFD) != 0){
        return ERROR(ERROR_FAILED_TO_CLOSE_SOCKET);
    }

    return ERROR(ERROR_NO_ERROR);
}

// TODO:(jan) Clean up error handling/clarify error console output.
ERROR_CODE http_openConnection(int_fast32_t* sockFD, const char* url, const uint_fast16_t port){
    struct addrinfo addressHints = {0};
    // AF_INET6 == force IPv6.
    // Aparently the linux subsystem for Windoof does not like AF_UNSPEC & returns '0.0.0.0' for every getaddrinfo call if we do not force IPv4 via 'AF_INET'.
    addressHints.ai_family = AF_INET; 
    addressHints.ai_socktype = SOCK_STREAM;

    // Convert port to service string.
    char service[UTIL_FORMATTED_NUMBER_LENGTH];
    // util_formatNumber(service, UTIL_FORMATTED_NUMBER_LENGTH, port);
    snprintf(service, UTIL_FORMATTED_NUMBER_LENGTH, "%" PRIdFAST16 "", port);

    struct addrinfo* serverInfo;
    int error;
    if((error = getaddrinfo(url, service, &addressHints, &serverInfo)) != 0){
        UTIL_LOG_ERROR_("GetaddrInfo:'%s'.", gai_strerror(error));

        freeaddrinfo(serverInfo);

        return ERROR(ERROR_FAILED_TO_RETRIEVE_ADDRESS_INFORMATION);
    }
    
    struct addrinfo* sockInfo;
    // Loop through all returned internet addresses.
    for(sockInfo = serverInfo; sockInfo != NULL; sockInfo = sockInfo->ai_next){
        // Create communication endpoint with internet address.
        if((*sockFD = socket(sockInfo->ai_family, sockInfo->ai_socktype, sockInfo->ai_protocol)) == -1){
            continue;
        }

        // Connect file discriptor with internet address.
        if(connect(*sockFD, sockInfo->ai_addr, sockInfo->ai_addrlen) == -1) {
            UTIL_LOG_ERROR_("Connect:'%s'.", strerror(errno));

            close(*sockFD);

            *sockFD = -1;
            
            continue;   
        }

        break;
    }

    if(sockInfo == NULL){
        UTIL_LOG_ERROR_("Failed to connect to '%s:%s'.", url, service);

        freeaddrinfo(serverInfo);

        return ERROR(ERROR_FAILED_TO_CONNECT);
    }

    freeaddrinfo(serverInfo);

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE http_sendRequest(HTTP_Request* request, HTTP_Response* response, const int_fast32_t connection){
    int_fast64_t bufferSize = request->bufferSize - request->dataLength;

    char* buffer = (char*) (request->data + request->dataLength);

    // Request line.
    uint_fast64_t writeOffset = snprintf(buffer, request->bufferSize, "%s %s %s\r\n",
        http_getRequestType(request->type),
        request->requestURL,
        request->httpVersion
    );

    // Content-Length:.
    char* contentLength = malloc(sizeof(*contentLength) * UTIL_FORMATTED_NUMBER_LENGTH);
    if(contentLength == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    const uint_fast64_t contentLengthLength = snprintf(contentLength, UTIL_FORMATTED_NUMBER_LENGTH, "%" PRIdFAST64 "", request->dataLength);

    char* _contentLength = malloc(sizeof(*_contentLength) * 16);
    if(contentLength == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    strncpy(_contentLength, "Content-Length", 16);

    http_addHeaderField(request, _contentLength, 16, contentLength, contentLengthLength);

    // HeaderFields.
    LinkedListIterator it;   
    linkedList_initIterator(&it, &request->headerFields);    

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT(&it);
                    
        writeOffset += snprintf(buffer + writeOffset, request->bufferSize - writeOffset, "%s: %s\r\n",
        headerField->name, 
        headerField->value
        );

        bufferSize -= writeOffset;
    }       
    
    writeOffset += snprintf(buffer + writeOffset, request->bufferSize - writeOffset, "\r\n");             

    buffer[writeOffset] = '\0';

    bufferSize -= writeOffset + 1;

    // Thats bad, we just have overwritten data in the request.
    if(bufferSize < 0){
        return ERROR(ERROR_HTTP_REQUEST_SIZE_EXCEEDED);
    }

    // Send request.
    if(write(connection, buffer, writeOffset) != (int_fast64_t) writeOffset){
        UTIL_LOG_ERROR("Failed to write buffer.");
        
        return ERROR(ERROR_WRITE_ERROR);
    }
    
    if(write(connection, request->data, request->dataLength) != (int_fast64_t) request->dataLength){
        UTIL_LOG_ERROR("Failed to write buffer.");
        
        return ERROR(ERROR_WRITE_ERROR);
    }    

    // Receive response.
    ERROR_CODE error;
    error = http_receiveResponse(response, connection);

    if(error != ERROR_NO_ERROR){
        return error;
    }

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE http_sendResponse(HTTP_Response* response, const int_fast32_t connection){
    int_fast64_t bufferSize = !response->staticContent ? (response->bufferSize - response->dataLength) : response->bufferSize;

    char* buffer = (char*) (response->data + (response->staticContent ? 0 : response->dataLength));

    const int_fast16_t statusCode = http_getStatusCode(response->statusCode);

    // Response line.
    uint_fast64_t writeOffset = snprintf(buffer, response->bufferSize, "%*.*s %" PRIdFAST16 " %s\r\n",
        8, 8, response->httpVersion,
        statusCode,
        http_getStatusMsg(response->statusCode)
    );

    // TODO:(jan) Clean this up.
    // Content-Type:.
    const char tType[] = "Content-Type";

    const uint_fast64_t tTypeLength = strlen(tType);

    char* contentType = malloc(sizeof(*contentType) *  (tTypeLength + 1));
    if(contentType == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    memcpy(contentType, tType, tTypeLength + 1);

    uint_fast64_t contentTypeLength;
    const char* contentTypeString = http_contentTypeToString(response->contentType, &contentTypeLength);

    char* _contentTypeString = malloc(sizeof(_contentTypeString) * (contentTypeLength + 1));
    if(_contentTypeString == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    memcpy(_contentTypeString, contentTypeString, contentTypeLength);
    _contentTypeString[contentTypeLength] = '\0';

    http_addHeaderField((HTTP_Request*) response, contentType, strlen(contentType), _contentTypeString, contentTypeLength);

    // Content-Length:.
    char* contentLength = malloc(sizeof(*contentLength) * UTIL_FORMATTED_NUMBER_LENGTH);
    if(contentLength == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    const uint_fast64_t contentLengthLength = snprintf(contentLength, UTIL_FORMATTED_NUMBER_LENGTH, "%" PRIdFAST64 "", response->dataLength);

    const char cType[] = "Content-Length";

    const uint_fast64_t _contentLengthLength =  strlen(cType);
    char* _contentLength = malloc(sizeof(*_contentLength) * (_contentLengthLength + 1));
    if(_contentLength == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    memcpy(_contentLength, cType, _contentLengthLength + 1);

    http_addHeaderField((HTTP_Request*) response, _contentLength, strlen(_contentLength), contentLength, contentLengthLength);

    const char* const serverMessage = "Herder Web-Server";
    HTTP_ADD_HEADER_FIELD((*response), Server, serverMessage);

    // HeaderFields.
    LinkedListIterator it;   
    linkedList_initIterator(&it, &response->headerFields);    

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT(&it);
                    
        writeOffset += snprintf(buffer + writeOffset, response->bufferSize - writeOffset, "%s: %s\r\n",
        headerField->name, 
        headerField->value
        );

        bufferSize -= writeOffset;
    }       
    
    writeOffset += snprintf(buffer + writeOffset, response->bufferSize - writeOffset, "\r\n");             

    buffer[writeOffset] = '\0';  
    
    bufferSize -= writeOffset + 1;

    // Thats bad, we just have overwritten data in the response.
    if(bufferSize < 0){
        return ERROR(ERROR_HTTP_RESPONSE_SIZE_EXCEEDED);
    }

    // Send response.
    if(write(connection, buffer, writeOffset) != (int_fast64_t) writeOffset){
        UTIL_LOG_ERROR("Failed to write buffer.");
        
        return ERROR(ERROR_WRITE_ERROR);
    }

    if(!response->staticContent){
        if(write(connection, response->data, response->dataLength) != (int_fast64_t) response->dataLength){
            UTIL_LOG_ERROR("Failed to write buffer.");
            
            return ERROR(ERROR_WRITE_ERROR);
        }
    }else{
        if(write(connection, response->cacheObject->data, response->dataLength) != (int_fast64_t) response->dataLength){
            UTIL_LOG_ERROR("Failed to write buffer.");
            
            return ERROR(ERROR_WRITE_ERROR);
        }
    }

    return ERROR(ERROR_NO_ERROR);
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

ERROR_CODE http_receiveResponse(HTTP_Response* response, const int_fast32_t connection){
    char* buffer = (char*) response->data;

    int_fast64_t bytesRead; 
    if((bytesRead = read(connection, buffer, response->bufferSize)) == -1){
        return ERROR(ERROR_READ_ERROR);     
    }

    if(bytesRead == 0){
        return ERROR(ERROR_EMPTY_RESPONSE);
    }else if(bytesRead ==(int_fast64_t) response->bufferSize){
        return ERROR(ERROR_MAX_MESSAGE_SIZE_EXCEEDED); 
    }else if(bytesRead < 3/*RequestType*/ + 3/*Spaces*/ + 2/*Line delimiter*/ + 1/*RequestURL*/ + 8/*Version*/){
        return ERROR(ERROR_INSUFICIENT_MESSAGE_LENGTH);
    }

    // RequestLine.
    int_fast64_t i;
    for(i = 1; i < bytesRead; i++){
        if(buffer[i] == '\n' && buffer[i - 1] == '\r'){
            int_fast64_t posSplitBegin = 0;
            int_fast64_t posSplitEnd;

            // Version.
            posSplitEnd = util_findFirst(buffer, bytesRead, ' ');

            const char* const version = buffer;
            const uint_fast64_t versionLength = posSplitEnd;

            if(strncmp(version, HTTP_VERSION_1_1, versionLength) != 0){
                return ERROR(ERROR_VERSION_MISSMATCH);
            }

            posSplitBegin = posSplitEnd + 1;

            // StatusCode.
            posSplitEnd = util_findFirst(buffer + posSplitBegin, bytesRead, ' ');

            const char* const statusCode = buffer + posSplitBegin;
                    
            errno = 0;

            char* endPtr;
            response->statusCode = strtol(statusCode, &endPtr, 10/*Decimal*/);

            if((response->statusCode == 0 && endPtr == statusCode) || (errno == ERANGE && (response->statusCode == LONG_MIN || response->statusCode == LONG_MAX))){
                return ERROR(ERROR_INVALID_STATUS_CODE);
            }

            response->statusCode = http_translateStatusCode(response->statusCode);

            posSplitBegin = posSplitEnd + 1;

            // StatusMessage.
            posSplitEnd = util_findFirst(buffer + posSplitBegin, bytesRead, ' ');

            response->statusMsg = buffer + posSplitBegin;
            response->statusMsgLength = posSplitEnd - posSplitBegin;

            break;
        }
    }

    i++;

    // HeaderFields.       
    int_fast64_t posSplitBegin = i;
    int_fast64_t posSplitEnd;
    for(;i < bytesRead; i++){
        if(buffer[i] == '\n' && buffer[i - 1] == '\r'){
            // No need to check for index underflow, because we need atleast two "\r\n" in the buffer to even get here.
            if(buffer[i - 2] == '\n' && buffer[i - 3] == '\r'){
                break;
            }

            posSplitEnd = i - 1;

            char* line = buffer + posSplitBegin;
            uint_fast64_t lineLength = posSplitEnd - posSplitBegin;

            const int_fast64_t lineSplit = util_findFirst(line, lineLength, ':');

            if(lineSplit == -1){
                // Copy current line onto stack to add limiting '\0' for printing.
                char* _line = alloca(lineLength + 1);
                memcpy(_line, line, lineLength);
                _line[lineLength] = '\0';

                UTIL_LOG_ERROR_("Failed to parse '%s'.", _line);

                return ERROR(ERROR_INVALID_HEADER_FIELD);
            }

            uint_fast64_t nameLength = lineSplit;

            char* name = malloc(sizeof(*name) * (nameLength + 1));
            if(name == NULL){
                return  ERROR(ERROR_OUT_OF_MEMORY);
            }

            memcpy(name, line, nameLength);
            name[nameLength] = '\0';

            uint64_t valueLength = lineLength - (lineSplit + 1);

            char* value = malloc(sizeof(*value) * (valueLength + 1));
            if(value == NULL){
                return  ERROR(ERROR_OUT_OF_MEMORY);
            }

            memcpy(value, line + lineSplit + 1, valueLength);
            value[valueLength] = '\0';

            util_trim(value, valueLength);

            posSplitBegin = i + 1;

            if(strncmp(name, "Content-Length:", nameLength) == 0){        
                char* endPtr;

                int_fast64_t contentLength;
                contentLength = strtol(value, &endPtr, 10/*Decimal*/);

                errno = 0;
                if((contentLength == 0 && endPtr == value) || (errno == ERANGE && (contentLength == LONG_MIN || contentLength == LONG_MAX))){
                    return ERROR(ERROR_INVALID_CONTENT_LENGTH);
                }

                response->dataLength = contentLength;
            }

             // TODO: (jan) Use a chunk of the processing buffer to store headerField structs.
            // Because the headerFields are at the beginning of both structs, we should be fine casting between the types.
            http_addHeaderField((HTTP_Request*) response, name, nameLength, value, valueLength);
        }
    }

    uint_fast64_t bufferSize = bytesRead;
    if(i + response->dataLength > (uint_fast64_t) bytesRead){
        bytesRead += read(connection, buffer + bytesRead, HTTP_PROCESSING_BUFFER_SIZE - bytesRead);

        bufferSize = bytesRead - bufferSize;

        if(bytesRead == -1){
            return ERROR(ERROR_READ_ERROR);
        }else{
            if(bytesRead >= HTTP_PROCESSING_BUFFER_SIZE){
                return ERROR(ERROR_MAX_MESSAGE_SIZE_EXCEEDED);
            }
        }
    }

    // Data.
    response->data = (int_fast8_t*) (buffer + i + 1);

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE http_receiveRequest(HTTP_Request* request, const int_fast32_t connection){
    char* buffer = (char*) request->data;

    int_fast64_t bytesRead = read(connection, buffer, HTTP_PROCESSING_BUFFER_SIZE);

    if(bytesRead == -1){
        return ERROR(ERROR_READ_ERROR);
    }else if(bytesRead >= HTTP_PROCESSING_BUFFER_SIZE){
        return ERROR(ERROR_MAX_MESSAGE_SIZE_EXCEEDED);
    }else if(bytesRead < 3/*RequestType*/ + 3/*Spaces*/ + 2/*Line delimiter*/ + 1/*RequestURL*/ + 8/*Version*/){
        return ERROR(ERROR_INSUFICIENT_MESSAGE_LENGTH);
    }

    // RequestLine.
    int_fast64_t i;
    for(i = 1; i < bytesRead; i++){
        if(buffer[i - 1] == '\n' && buffer[i - 2] == '\r'){
            int_fast64_t posSplitBegin = 0;
            int_fast64_t posSplitEnd;

            // RequestType.
            posSplitEnd = util_findFirst(buffer, bytesRead, ' ');

            if(posSplitEnd == -1){
                return ERROR(ERROR_INVALID_HEADER_FIELD);
            }

            const char* const requestType = buffer;
            const uint_fast64_t requestTypeLength = posSplitEnd;

            request->type = http_parseRequestType(requestType, requestTypeLength);

            posSplitBegin = posSplitEnd + 1;

            // RequestURL.
            posSplitEnd = util_findFirst(buffer + posSplitBegin, bytesRead, ' ');

            if(posSplitEnd == -1){
                return ERROR(ERROR_INVALID_HEADER_FIELD);
            }

            request->urlLength = posSplitEnd;

            if(request->urlLength == 0){
                return ERROR_(ERROR_INVALID_REQUEST_URL, "URL length can't be '%" PRIuFAST16 "'.", request->urlLength);
            }

            request->requestURL = malloc(sizeof(*request->requestURL) * request->urlLength + 1);
            if(request->requestURL == NULL){
               return  ERROR(ERROR_OUT_OF_MEMORY);
            }

            memcpy(request->requestURL, buffer + posSplitBegin, request->urlLength);
            request->requestURL[request->urlLength] = '\0';
        
            posSplitBegin += posSplitEnd + 1;

            // Version.
            posSplitEnd = util_findFirst(buffer, bytesRead, ' ');

            const char* const version = buffer + posSplitBegin;
            const uint_fast64_t versionLength = posSplitEnd;

            if(strncmp(version, HTTP_VERSION_1_1, versionLength) != 0){
                return ERROR(ERROR_VERSION_MISSMATCH);
            }

            break;
        }
    }

    // HeaderFields.       
    uint_fast64_t posLineBegin = i;
    for(;i < bytesRead; i++){
        if(buffer[i] == '\n' && buffer[i - 1] == '\r'){
            if(buffer[i - 2] == '\n' && buffer[i - 3] == '\r'){
                // Skip trailing '\n'.
                i++;

                break;
            }

            char* line = buffer + posLineBegin;
            uint_fast64_t lineLength = i - posLineBegin;

            const int_fast64_t nameLength = util_findFirst(line, lineLength,':');
            if(nameLength == -1){
                return ERROR(ERROR_INVALID_HEADER_FIELD);
            }

            char* name = malloc(sizeof(*name) * nameLength);
            if(name == NULL){
                return  ERROR(ERROR_OUT_OF_MEMORY);
            }

            memcpy(name, line, nameLength);

            // Move line beginning after ':'
            line += nameLength + 1;
            lineLength -= nameLength + 1;

            // Skip trailing whitespace
            if(isspace(*line) != 0){
                line++;
                lineLength--;
            }

            const int_fast64_t valueLength = lineLength;
            if(valueLength == -1){
                return ERROR(ERROR_INVALID_HEADER_FIELD);
            }

            char* value = malloc(sizeof(*value) * valueLength);
            if(value == NULL){
                return  ERROR(ERROR_OUT_OF_MEMORY);
            }

            memcpy(value, line, valueLength);

            if(strncmp(name, "Content-Length:", nameLength) == 0){
                errno = 0;

                char* endPtr;
                int_fast64_t contentLength;
                contentLength = strtol(value, &endPtr, 10/*Decimal*/);

                if((contentLength == 0 && endPtr == value) || (errno == ERANGE && (contentLength == LONG_MIN || contentLength == LONG_MAX))){
                    return ERROR(ERROR_INVALID_CONTENT_LENGTH);
                }

                request->dataLength = contentLength;
            }

            http_addHeaderField(request, name, nameLength, value, valueLength);

            posLineBegin = i + 1;
        }
    }

    // We have not read the whole request, because of the way we handle/send request header and data in 'http_sendRequest'.
    uint_fast64_t bufferSize = bytesRead;
    if(i + request->dataLength > (uint_fast64_t) bytesRead){
        bytesRead += read(connection, buffer + bytesRead, HTTP_PROCESSING_BUFFER_SIZE - bytesRead);

        bufferSize = bytesRead - bufferSize;

        if(bytesRead == -1){
            return ERROR(ERROR_READ_ERROR);
        }else{
            if(bytesRead >= HTTP_PROCESSING_BUFFER_SIZE){
                return ERROR(ERROR_MAX_MESSAGE_SIZE_EXCEEDED);
            }
        }
    }

    // Data.
    request->data = (int_fast8_t*) (buffer + i);

    return ERROR(ERROR_NO_ERROR);
}

void http_freeHTTP_Request(HTTP_Request* request){
    LinkedListIterator it;
    linkedList_initIterator(&it, &request->headerFields);
   
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT(&it);
        
        free(headerField->name);
        free(headerField->value);
        free(headerField);
    }       
            
    free(request->requestURL);
    
    linkedList_free(&request->headerFields);
}

void http_freeHTTP_Response(HTTP_Response* response){
    LinkedListIterator it;
    linkedList_initIterator(&it, &response->headerFields);
   
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        HTTP_HeaderField* headerField = LINKED_LIST_ITERATOR_NEXT(&it);
        
        free(headerField->name);
        free(headerField->value);
        free(headerField);
    }       
            
    linkedList_free(&response->headerFields);
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

    HTTP_RequestType ret = REQUEST_TYPE_UNKNOWN;
        
    switch(util_hashString(type, length)){
        case HASH_GET:{
            ret = REQUEST_TYPE_GET;
            
            break;
        }
        case HASH_POST:{
            ret = REQUEST_TYPE_POST;
            
            break;
        }
        case HASH_HEAD:{
            ret = REQUEST_TYPE_HEAD;
            
            break;
        }
        case HASH_PUT:{
            ret = REQUEST_TYPE_PUT;
            
            break;
        }
        case HASH_DELETE:{
            ret = REQUEST_TYPE_DELETE;
            
            break;
        }
        case HASH_TRACE:{
            ret = REQUEST_TYPE_TRACE;
            
            break;
        }
        case HASH_OPTIONS:{
            ret =REQUEST_TYPE_OPTIONS;
            
            break;
        }
        case HASH_CONNECT:{
            ret = REQUEST_TYPE_CONNECT;
            
            break;
        }
        case HASH_PATCH:{
            ret = REQUEST_TYPE_PATCH;
            
            break;
        }
        default:{
            UTIL_LOG_ERROR_("Unknown request type:'%s'.", type);
            
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

local const char* const HTTP_STATUS_MESSAGE_MAPPING_ARRAY[] = {
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

local const char* HTTP_REQUEST_TYPE_MAPPING_ARRAY[] = {
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

inline const char* http_getRequestType(HTTP_RequestType requestType){
    return HTTP_REQUEST_TYPE_MAPPING_ARRAY[requestType];
}

local const int_fast16_t HTTP_STATUS_CODE_MAPPING_ARRAY[] = {
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

inline int_fast16_t http_getStatusCode(HTTP_StatusCode statusCode){
    return HTTP_STATUS_CODE_MAPPING_ARRAY[statusCode];
}

inline void http_setHTTP_Version(HTTP_Response* response, const char version[]){
    memcpy(response->httpVersion, version, 9);
}

typedef struct{
    const char*  name;
    const uint_fast64_t length;
}ContentType;

local const ContentType HTTP_CONTENT_TYPE_MAPPING_ARRAY[] = {
    {"TEXT/HTML", 9},
    {"text/css", 8},
    {"application/javascript", 22},
    {"application/json", 16},
    {"application/zip", 15},
    {"iamge/jpeg", 10},
    {"image/png", 9},
    {"image/svg+xml", 13},
    {"image/x-icon", 12}
};

inline const char* http_contentTypeToString(const HTTP_ContentType contentType, uint_fast64_t* length){
    const ContentType _contentType = HTTP_CONTENT_TYPE_MAPPING_ARRAY[contentType];

    *length = _contentType.length;

    return _contentType.name;
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

    switch(util_hashString(fileExtension, fileExtensionLength)){
        case HASH_CSS :{
          return HTTP_CONTENT_TYPE_TEXT_CSS;
          
          break;
      }
        case HASH_JS :{
          return HTTP_CONTENT_TYPE_APPLICATION_JAVASCRIPT;
          
          break;
      }
        case HASH_PNG :{
          return HTTP_CONTENT_TYPE_IMAGE_PNG;
          
          break;
      }
        case HASH_JPG :{
          return HTTP_CONTENT_TYPE_IMAGE_JPG;
          
          break;
      }
        case HASH_SVG :{
          return HTTP_CONTENT_TYPE_IMAGE_SVG;
          
          break;
      }            
      case HASH_ICO:{
          return HTTP_CONTENT_TYPE_IMAGE_ICON;
          
          break;
      }
      case HASH_HTML:
      case HASH_TXT:
      default:{
          return HTTP_CONTENT_TYPE_TEXT_HTML;
          
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
}

#endif