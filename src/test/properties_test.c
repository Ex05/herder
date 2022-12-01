#ifndef PROPERTIES_TEST_C
#define PROPERTIES_TEST_C

#include "../test.c"

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(properties, properties){
	*properties = calloc(1, sizeof(DoublyLinkedList));
		
	if(*properties == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(properties, PropertyFile, properties){
	properties_free(properties);

	free(properties);

	return ERROR(ERROR_NO_ERROR);
}

#define EXAMPLE_PROPERTY_FILE "Version: 1.0.0\n\n# Server\nport = 1869\ndaemonize = true\nnum_worker_threads = 1\nhttp_root_directory = /home/ex05/herder/www\ncustom_error_pages_directoory = /home/ex05/herder/error_pages\nlogfile_directory = /home/ex05/herder/log\nepoll_event_buffer_size = 32\nhttp_cache_size = 256\nerror_page_cache_size = 4\n// Max architecture independant guaranteed size is 2pow(16) or 65_535 Bytes.\nhttp_read_buffer_size = 8096\n\n# Security\nssl_privateKeyFile = ./res/testCertificate.pem\nssl_certificate = ./res/testCertificate.pem\n\nwork_directory = /home/exo5/herder\nsystem_log_id = herder_server"

TEST_TEST_FUNCTION_(properties_parse, PropertyFile, properties){
	char settingsString[] = EXAMPLE_PROPERTY_FILE;

	if(properties_parse(properties, settingsString, strlen(settingsString)) != ERROR_NO_ERROR){
		return TEST_FAILURE("%s", "Failed to parse settings string.");
	}

	#define PROPERTY_PORT_NAME "port"
	#define PROPERTY_PORT_VALUE "1869"

	Property* portProperty;
	if(properties_get(properties, &portProperty, PROPERTY_PORT_NAME, strlen(PROPERTY_PORT_NAME)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to retrieve propertry: '%s'.", PROPERTY_PORT_NAME);
	}

	if(portProperty == NULL){
		return TEST_FAILURE("Failed to parse property: '%s'.", PROPERTY_PORT_NAME);
	}

	if(strncmp(portProperty->value, PROPERTY_PORT_VALUE, strlen(PROPERTY_PORT_VALUE)) != 0){
		return TEST_FAILURE("Failed to parse property: '%s' '%s'!='%s'.", PROPERTY_PORT_NAME, portProperty->value, PROPERTY_PORT_VALUE);
	}

	#undef PROPERTY_PORT_VALUE
	#undef PROPERTY_PORT_NAME

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(properties_get, PropertyFile, properties){
	#define PROPERTY_PORT_NAME "port"
	#define PROPERTY_PORT_VALUE "1869"

	Property* property;
	if(properties_initProperty(&property, PROPERTY_FILE_ENTRY_TYPE_PROPERTY, PROPERTY_PORT_NAME, strlen(PROPERTY_PORT_NAME), (int8_t*) PROPERTY_PORT_VALUE, strlen(PROPERTY_PORT_VALUE)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to initialise propertry: '%s'.", PROPERTY_PORT_NAME);
	}

	if(properties_addPropertyFileEntry(properties, NULL, property) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to add propertry: '%s' to property file.", PROPERTY_PORT_NAME);
	}

	Property* portProperty;
	if(properties_get(properties, &portProperty, PROPERTY_PORT_NAME, strlen(PROPERTY_PORT_NAME)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to retrieve propertry: '%s'.", PROPERTY_PORT_NAME);
	}

	if(portProperty == NULL){
		return TEST_FAILURE("Failed to retrieve propertry: '%s'.", PROPERTY_PORT_NAME);
	}

	if(strncmp(portProperty->value, PROPERTY_PORT_VALUE, strlen(PROPERTY_PORT_VALUE)) != 0){
		return TEST_FAILURE("Failed to parse property: '%s' '%s' != '%s'.", PROPERTY_PORT_NAME, portProperty->value, PROPERTY_PORT_VALUE);
	}

	if(properties_get(properties, &portProperty, "abbab", strlen("abbab")) == ERROR_NO_ERROR){
		return TEST_FAILURE("False posetive for property: '%s'.", "abbab");
	}

	#undef PROPERTY_PORT_VALUE
	#undef PROPERTY_PORT_NAME

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(properties_propertyExists, PropertyFile, properties){
	char settingsString[] = EXAMPLE_PROPERTY_FILE;

	if(properties_parse(properties, settingsString, strlen(settingsString)) != ERROR_NO_ERROR){
		return TEST_FAILURE("%s", "Failed to parse settings string.");
	}

	#define PROPERTY_PORT_NAME "port"

	if(!properties_propertyExists(properties, PROPERTY_PORT_NAME, strlen(PROPERTY_PORT_NAME))){
		return TEST_FAILURE("Property '%s' does not exist.", PROPERTY_PORT_NAME);
	}

	if(properties_propertyExists(properties, "abbab", strlen("abbab"))){
		return TEST_FAILURE("False posetive for property: '%s'.", "abbab");
	}

	#undef PROPERTY_PORT_NAME

	return TEST_SUCCESS;
}

#undef EXAMPLE_PROPERTY_FILE

#endif