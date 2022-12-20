#ifndef SERVER_TEST_C
#define SERVER_TEST_C

#include "../test.c"

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(server, Server, server){
	*server = calloc(1, sizeof(**server));

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(server, Server, server){
	LinkedListIterator it;
	linkedList_initIterator(&it, &server->contexts);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Context* context = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Context);

		server_freeContext(context);

		free(context);
	}

	linkedList_free(&server->contexts);

	free(server);

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(server_addContext, Server, server){
	server_addContext(server, "/img", NULL);

	if(server->contexts.length != 1){
		return TEST_FAILURE("Failed to add context '%s' to server.", "/img");
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &server->contexts);

	Context* context = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Context);

	if(strncmp(context->symbolicFileLocation, "/img", 5) != 0){
		return TEST_FAILURE("Failed to add context '%s' to server.", "/img");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(server_getContextHandler, Server, server){
	server_addContext(server, "/img", NULL);

	ERROR_CODE error;

	HTTP_Request request;
	request.requestURL = "/img";
	request.requestURLLength = 4;

	ContextHandler* contextHandler;
	if((error = server_getContextHandler(server, &contextHandler, &request)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to get contexthandler for request url '%s'.", "/img");
	}

	request.requestURL = "/index.html";
	request.requestURLLength = 11;

	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
	if((error = server_getContextHandler(server, &contextHandler, &request)) == ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to get contexthandler for request url '%s'.", "/index.html");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(server_translateSymbolicFileLocation, Server, server){
	#define HTTP_ROOT_DIRECTORY "/home/ex05/herder/www"

	#define PROPERTY_NAME "httpRootDirectory"
	properties_initProperty(&server->httpRootDirectory, PROPERTY_FILE_ENTRY_TYPE_PROPERTY, PROPERTY_NAME, strlen(PROPERTY_NAME), (int8_t*) HTTP_ROOT_DIRECTORY, strlen(HTTP_ROOT_DIRECTORY));
	#undef PROPERTY_NAME

	char symbolicFileLocation[] = "/img/img_001.jpg";
	const uint_fast64_t symbolicFileLocationLength = strlen(symbolicFileLocation);

	SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION(fileLocation, server, symbolicFileLocation, symbolicFileLocationLength);

	if(strncmp(fileLocation, HTTP_ROOT_DIRECTORY "/img/img_001.jpg", symbolicFileLocationLength + 1) != 0){
		return TEST_FAILURE("Failed to translate symbolic file location '%s'.", symbolicFileLocation);
	}
	#undef HTTP_ROOT_DIRECTORY

	free(server->httpRootDirectory->name);
	free(server->httpRootDirectory->data);

	free(server->httpRootDirectory);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(server_translateSymbolicFileLocationErrorPage, Server, server){
	#define CUSTOM_ERROR_PAGES_DIRECTORY "/home/ex05/herder/error_pages"

	#define PROPERTY_NAME "customErrorPageDirectory"
	properties_initProperty(&server->customErrorPageDirectory, PROPERTY_FILE_ENTRY_TYPE_PROPERTY, PROPERTY_NAME, strlen(PROPERTY_NAME), (int8_t*) CUSTOM_ERROR_PAGES_DIRECTORY, strlen(CUSTOM_ERROR_PAGES_DIRECTORY));
	#undef PROPERTY_NAME

	const uint_fast64_t symbolicFileLocationLength = + 5/*".html"*/ + 1/*'/'*/ + 1/*_*/+ 3/*http status code*/;
	char* symbolicFileLocation = alloca(sizeof(*symbolicFileLocation) * (symbolicFileLocationLength + 1));
	snprintf(symbolicFileLocation, symbolicFileLocationLength + 1, "/%03ld.html", http_getNumericalStatusCode(_404_NOT_FOUND));

	SERVER_TRANSLATE_SYMBOLIC_FILE_LOCATION_ERROR_PAGE(fileLocation, server, symbolicFileLocation, symbolicFileLocationLength);

	if(strncmp(fileLocation, CUSTOM_ERROR_PAGES_DIRECTORY "/404.html", fileLocationLength + 1) != 0){
		return TEST_FAILURE("Failed to translate symbolic file location '%s'.", symbolicFileLocation);
	}

	#undef CUSTOM_ERROR_PAGES_DIRECTORY

	free(server->customErrorPageDirectory->name);
	free(server->customErrorPageDirectory->data);

	free(server->customErrorPageDirectory);

	return TEST_SUCCESS;
}

#endif
