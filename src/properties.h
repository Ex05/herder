#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "util.h"

#include "linkedList.h"

#define PROPERTIES_SIZEOF_PROPERTY(property) ((2 * sizeof(int_fast64_t)) + sizeof(int8_t) + (sizeof(int8_t) * (property->nameLength + property->dataLength)))

#define PROPERTIES_PROPERTY_NAME(property) ((char*) property->data)
#define PROPERTIES_PROPERTY_DATA(property) (property->data + (property->nameLength + 1))

#define PROPERTIES_NEW_PROPERTY_FILE_SECTION(_name, _value) PropertyFileSection propertyFileSection ## _name; \
	do{ \
	propertyFileSection ## _name.nameLength = strlen(_value); \
	propertyFileSection ## _name.name = malloc(sizeof(*propertyFileSection ## _name.name) + (propertyFileSection ## _name.nameLength + 1)); \
	memcpy(propertyFileSection ## _name.name, _value, propertyFileSection ## _name.nameLength + 1); \
	memset(&propertyFileSection ## _name.properties, 0, sizeof(propertyFileSection ## _name.properties)); \
	}while(0)

#define PROPERTIES_NEW_PROPERTY_TEMPLATE(_name, _propertyName, _value) PropertyTemplate propertyTemplate ## _name; \
	do{ \
	propertyTemplate ## _name.nameLength = strlen(_propertyName); \
	propertyTemplate ## _name.name = malloc(sizeof(*propertyTemplate ## _name.name) * (propertyTemplate ## _name.nameLength + 1)); \
	memcpy(propertyTemplate ## _name.name, _propertyName, propertyTemplate ## _name.nameLength + 1); \
	\
	propertyTemplate ## _name.valueLength = strlen(_value); \
	propertyTemplate ## _name.value = malloc(sizeof(*propertyTemplate ## _name.value) * propertyTemplate ## _name.valueLength); \
	memcpy(propertyTemplate ## _name.value, _value, propertyTemplate ## _name.valueLength); \
	}while(0)

#define PROPERTIES_PROPERTRY_FILE_TEMPLATE_ADD_SECTION(_template, _section) properties_propertyFileTemplateAddSection(&propertyFileTemplate ## _template, &propertyFileSection ## _section)

#define PROPERTIES_PROPERTY_FILE_SECTION_ADD_PROPERTY(_section, _property) properties_propertyFileTemplateFileSectionAddProperty(&propertyFileSection ## _section, &propertyTemplate ## _property);

#define PROPERTIES_NEW_PROPERTY_FILE_TEMPLATE(_name, _location) PropertyFileTemplate propertyFileTemplate ## _name = {0}; \
	do{ \
	properties_propertyFileTemplateSetFilePath(&propertyFileTemplate ## _name, _location, strlen(_location)); \
	properties_propertyFileTemplateSetVersion(&propertyFileTemplate ## _name, (Version) {1, 0, 0}); \
	}while(0)

#define PROPERTIES_CREATE_PROPERTY_FILE(_template) properties_createPropertyFileFromTemplate(&propertyFileTemplate ## _template)

#define PROPERTIES_FREE_PROPERTY_FILE_TEMPLATE(_template) properties_freePropertyFileTemplate(&propertyFileTemplate ## _template)

#define PROPERTIES_PROPERTY_FILE_ADD_PROPERTY(_template, _property) properties_propertyFileTemplateAddProperty(&propertyFileTemplate ## _template, &propertyTemplate ## _property);

// NOTE: A variable of type 'ERROR_CODE' named 'error' has to be in scope. (jan - 2000.06.08)
#define PROPERTY_EXISTS(name)do{ \
	if(!properties_propertyExists(&server->properties, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))){ \
		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Failed to retrieve property: '%s' from property file: '%s'.", CONSTANTS_ ## name ## _PROPERTY_NAME, server->properties.filePath); \
		\
		error = ERRROR_MISSING_PROPERTY;\
	} \
}while(0)

#define PROPERTIES_GET(propertyFile, name) properties_get(propertyFile, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))

typedef struct version{
	uint_fast8_t release;
	uint_fast8_t update;
	uint_fast8_t hotfix;
}Version;

typedef struct{
	char* filePath;
	uint_fast64_t filePathLength;
	LinkedList properties;
}PropertyFile;

typedef struct{
	int_fast64_t nameLength;
	int_fast64_t dataLength;
	int8_t data[];
}Property;

typedef struct{
	char* name;
	int_fast64_t nameLength;
	LinkedList properties;
}PropertyFileSection;

typedef struct{
	char* name;
	int_fast64_t nameLength;
	char* value;
	int_fast64_t valueLength;
}PropertyTemplate;

typedef struct{
	Version version;
	char* filePath;
	int_fast64_t filePathLength;
	LinkedList sections;
	LinkedList properties;
}PropertyFileTemplate;

local const Version VERSION = {1, 0, 0};

ERROR_CODE properties_load(PropertyFile*, const char*, const uint_fast64_t);

ERROR_CODE properties_initProperty(Property**, char*, const int_fast64_t, int8_t*, const int_fast64_t);

ERROR_CODE properties_addProperty(PropertyFile*, const char*, const uint_fast64_t, const int8_t data[]);

Property* properties_get(PropertyFile*, const char*, const int_fast64_t);

ERROR_CODE properties_updateProperty(PropertyFile*, const char*, const uint_fast64_t, const int8_t data[]);

bool properties_propertyExists(PropertyFile*, const char*, const uint_fast64_t);

void properties_propertyFileTemplateSetVersion(PropertyFileTemplate*, Version);

void properties_propertyFileTemplateSetFilePath(PropertyFileTemplate*, char*, int_fast64_t);

void properties_propertyFileTemplateAddSection(PropertyFileTemplate*, PropertyFileSection*);

void properties_propertyFileTemplateFileSectionAddProperty(PropertyFileSection*, PropertyTemplate*);

void properties_propertyFileTemplateFileTemplateAddProperty(PropertyFileTemplate*, PropertyTemplate*);

ERROR_CODE properties_createPropertyFileFromTemplate(PropertyFileTemplate*);

void properties_freePropertyFileTemplate(PropertyFileTemplate*);

void properties_free(PropertyFile*);

#endif