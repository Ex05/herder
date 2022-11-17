#ifndef PROPERTIES_TEST_C
#define PROPERTIES_TEST_C

#include "../test.c"
#include <stdint.h>
#include <string.h>
#include <sys/syslog.h>

TEST_TEST_FUNCTION(properties_newPropertyFileTemplate){
	#define PROPERTY_FILE_TEMPLATE_PATH "/home/ex05/herder"

	PROPERTIES_NEW_PROPERTY_FILE_TEMPLATE(Template, PROPERTY_FILE_TEMPLATE_PATH);

	if(strncmp(propertyFileTemplateTemplate.filePath, PROPERTY_FILE_TEMPLATE_PATH, strlen(PROPERTY_FILE_TEMPLATE_PATH) + 1) != 0){
		return TEST_FAILURE("Failed to set property file path '%s' != '%s'", propertyFileTemplateTemplate.filePath, PROPERTY_FILE_TEMPLATE_PATH);
	}

	#undef PROPERTY_FILE_TEMPLATE_PATH

	PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(properties_newPropertyTemplate){
	#define PROPERTY_TEMPLATE_NAME "port"
	#define PROPERTY_TEMPLATE_VALUE "1869"

	PROPERTIES_NEW_PROPERTY_TEMPLATE(Port, PROPERTY_TEMPLATE_NAME, PROPERTY_TEMPLATE_VALUE);

	if(strncmp(propertyTemplatePort.name, PROPERTY_TEMPLATE_NAME, strlen(PROPERTY_TEMPLATE_NAME) + 1) != 0){
		return TEST_FAILURE("Failed to set property file path '%s' != '%s'", propertyTemplatePort.name, PROPERTY_TEMPLATE_NAME);
	}

	if(strncmp(propertyTemplatePort.value, PROPERTY_TEMPLATE_VALUE, strlen(PROPERTY_TEMPLATE_VALUE)) != 0){
		return TEST_FAILURE("Failed to set property file path '%s' != '%s'", propertyTemplatePort.value, PROPERTY_TEMPLATE_VALUE);
	}

	free(propertyTemplatePort.name);
	free(propertyTemplatePort.value);

	#undef PROPERTY_TEMPLATE_NAME
	#undef PROPERTY_TEMPLATE_VALUE

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(properties_newPropertyFileSection){
	#define PROPERTY_FILE_SECTION_NAME "server"

	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Server, PROPERTY_FILE_SECTION_NAME);

	if(strncmp(propertyFileSectionServer.name, PROPERTY_FILE_SECTION_NAME, strlen(PROPERTY_FILE_SECTION_NAME) + 1) != 0){
		return TEST_FAILURE("Failed to set property file section name '%s' != '%s'.", propertyFileSectionServer.name, PROPERTY_FILE_SECTION_NAME);
	}

	free(propertyFileSectionServer.name);

	#undef PROPERTY_FILE_SECTION_NAME

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(properties_propertyFileSectionAddProperty){
	#define PROPERTY_FILE_SECTION_NAME "server"

	#define PROPERTY_TEMPLATE_NAME "port"
	#define PROPERTY_TEMPLATE_VALUE "1869"

	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Server, PROPERTY_FILE_SECTION_NAME);

	PROPERTIES_NEW_PROPERTY_TEMPLATE(Port, PROPERTY_TEMPLATE_NAME, PROPERTY_TEMPLATE_VALUE);

	PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Server, Port);

	if(propertyFileSectionServer.properties.length != 1){
		return TEST_FAILURE("Failed to add property template '%s' to property file section '%s'.", propertyTemplatePort.name, propertyFileSectionServer.name);
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &propertyFileSectionServer.properties);

	PropertyTemplate* property = LINKED_LIST_ITERATOR_NEXT(&it);

	if(strncmp(property->name, PROPERTY_TEMPLATE_NAME, strlen(PROPERTY_TEMPLATE_NAME) + 1) != 0){
		return TEST_FAILURE("Failed to set property tempalte name '%s' != '%s'", property->name, PROPERTY_TEMPLATE_NAME);
	}

	free(propertyTemplatePort.name);
	free(propertyTemplatePort.value);

	linkedList_free(&propertyFileSectionServer.properties);

	free(propertyFileSectionServer.name);

	#undef PROPERTY_FILE_SECTION_NAME

	#undef PROPERTY_TEMPLATE_NAME
	#undef PROPERTY_TEMPLATE_VALUE

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(properties_propertryFileTemplateAddSection){
	#define PROPERTY_FILE_TEMPLATE_PATH "/home/ex05/herder"
	#define PROPERTY_FILE_SECTION_NAME "server"

	PROPERTIES_NEW_PROPERTY_FILE_TEMPLATE(Template, PROPERTY_FILE_TEMPLATE_PATH);

	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Server, PROPERTY_FILE_SECTION_NAME);

	PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(Template, Server);

	if(propertyFileTemplateTemplate.sections.length != 1){
		return TEST_FAILURE("Failed to add property section '%s' to property file '%s'.", propertyFileSectionServer.name, propertyFileTemplateTemplate.filePath);
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &propertyFileTemplateTemplate.sections);

	PropertyFileSection* section = LINKED_LIST_ITERATOR_NEXT(&it);

	if(strncmp(section->name, PROPERTY_FILE_SECTION_NAME, strlen(PROPERTY_FILE_SECTION_NAME) + 1) != 0){
		return TEST_FAILURE("Failed to set property file path '%s' != '%s'", section->name, PROPERTY_FILE_SECTION_NAME);
	}

	PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);

	#undef PROPERTY_FILE_SECTION_NAME
	#undef PROPERTY_FILE_TEMPLATE_PATH

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(properties_createPropertyFileFromTemplate){
	#define PROPERTY_FILE_TEMPLATE_PATH "./tmp/properties_createPropertyFileFromTemplate"
	#define PROPERTY_FILE_SECTION_001_NAME "section_001"
	#define PERTY_FILE_PROPERTY_001_NAME "property_001"
	#define PERTY_FILE_PROPERTY_002_NAME "property_002"
	#define PERTY_FILE_PROPERTY_003_NAME "property_003"
	#define PERTY_FILE_PROPERTY_004_NAME "property_004"
	#define PROPERTY_FILE_SECTION_002_NAME "section_002"

	// PropertyFile.		
	PROPERTIES_NEW_PROPERTY_FILE_TEMPLATE(Template, PROPERTY_FILE_TEMPLATE_PATH);

	// Section_002
	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Section_002, PROPERTY_FILE_SECTION_002_NAME);	

	// Property_003
	PROPERTIES_NEW_PROPERTY_TEMPLATE(Property_003, PERTY_FILE_PROPERTY_003_NAME, "test123");
	PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Section_002, Property_003);

	PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(Template, Section_002);

	// Section_001
	PROPERTIES_NEW_PROPERTY_FILE_SECTION(Section_001, PROPERTY_FILE_SECTION_001_NAME);

	// Property_002
	PROPERTIES_NEW_PROPERTY_TEMPLATE(Property_002, PERTY_FILE_PROPERTY_002_NAME, "123");
	PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Section_001, Property_002);

	// Property_001
	PROPERTIES_NEW_PROPERTY_TEMPLATE(Property_001, PERTY_FILE_PROPERTY_001_NAME, "test");
	PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(Section_001, Property_001);

	PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(Template, Section_001);

	// Property_004
	PROPERTIES_NEW_PROPERTY_TEMPLATE(Property_004, PERTY_FILE_PROPERTY_004_NAME, "1234test");

	PROPERTIES_PROPERTY_FILE_ADD_PROPERTY(Template, Property_004);			

	ERROR_CODE error;
	if((error = PROPERTIES_CREATE_PROPERTY_FILE(Template)) != ERROR_NO_ERROR){
		PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);
	}

	PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(Template);

	// Read created file.
	FILE* filePtr = fopen(PROPERTY_FILE_TEMPLATE_PATH, "r");

	if(filePtr == NULL){
		return TEST_FAILURE("Failed to open file: '%s'.", PROPERTY_FILE_TEMPLATE_PATH);
	}

	int8_t readBuffer[PROPERTY_FILE_READ_BUFFER];
	uint_fast64_t bytesRead = fread(readBuffer, 1, PROPERTY_FILE_READ_BUFFER, filePtr);
	readBuffer[bytesRead] = '\0';

	char propertyFileString[] = "Version: 1.0.0\n\n\
# section_001\n\
property_001 = test\n\
property_002 = 123\n\
\n\
# section_002\n\
property_003 = test123\n\
\n\
property_004 = 1234test\n\
";

	if(strncmp((char*) readBuffer, propertyFileString, strlen(propertyFileString) + 1) != 0){
		return TEST_FAILURE("'%s'\n'%s'. does not match the expected output.", readBuffer, propertyFileString);
	}

	fclose(filePtr);

	// Check file content.

	#undef PROPERTY_FILE_TEMPLATE_PATH
	#undef PROPERTY_FILE_SECTION_001_NAME
	#undef PERTY_FILE_PROPERTY_001_NAME
	#undef PERTY_FILE_PROPERTY_002_NAME
	#undef PERTY_FILE_PROPERTY_003_NAME
	#undef PERTY_FILE_PROPERTY_004_NAME
	#undef PROPERTY_FILE_SECTION_002_NAME

	return TEST_SUCCESS;
}

#endif