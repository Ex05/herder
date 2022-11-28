#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "util.h"

#include "doublyLinkedList.h"
#include <stdint.h>

// NOTE: A variable of type 'ERROR_CODE' named 'error' has to be in scope. (jan - 2022.06.08)
#define PROPERTY_EXISTS(name)do{ \
	if(!properties_propertyExists(&server->properties, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))){ \
		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Failed to retrieve property: '%s' from property file.", CONSTANTS_ ## name ## _PROPERTY_NAME); \
		error = ERRROR_MISSING_PROPERTY;\
	} \
}while(0)

#define PROPERTIES_GET(propertyFile, name) properties_get(propertyFile, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))

typedef struct version{
	uint_fast8_t release;
	uint_fast8_t update;
	uint_fast8_t hotfix;
}Version;

typedef enum{
	PROPERTY_FILE_ENTRY_TYPE_PROPERTY = 0,
	PROPERTY_FILE_ENTRY_TYPE_COMMENT,
	PROPERTY_FILE_ENTRY_TYPE_EMPTY_LINE,
	PROPERTY_FILE_ENTRY_TYPE_SECTION
}PropertyFileEntryType;

typedef struct{
	PropertyFileEntryType type;

	char* name;
	uint_fast64_t nameLength;

	union{
			DoublyLinkedList properties;

			struct{
				union{
					uint8_t* data;
					char* value;
				};

				union{
					uint_fast64_t dataLength;
					uint_fast64_t valueLength;
				};				
			};
	};
}PropertyFileEntry;

typedef struct{
	DoublyLinkedList properties;
}PropertyFile;

typedef PropertyFileEntry Property;

local const Version VERSION = {1, 0, 0};

ERROR_CODE properties_loadFromDisk(PropertyFile*, const char*, const uint_fast64_t);

ERROR_CODE properties_initProperty(Property**, PropertyFileEntryType, char*, const int_fast64_t, int8_t*, const int_fast64_t);

ERROR_CODE properties_addPropertyFileEntry(PropertyFile*, PropertyFileEntry*, Property*);

Property* properties_get(PropertyFile*, const char*, const uint_fast64_t);

ERROR_CODE properties_updateProperty(PropertyFile*, const char*, const uint_fast64_t, const int8_t data[]);

ERROR_CODE properties_parse(PropertyFile*, char*, uint_fast64_t);

ERROR_CODE properties_parseLine(PropertyFile*, PropertyFileEntry**, char*, uint_fast64_t);

bool properties_propertyExists(PropertyFile*, const char*, const uint_fast64_t);

void properties_free(PropertyFile*);

local ERROR_CODE _properties_parseEmptyLine(PropertyFile*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseComment(PropertyFile*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseSection(PropertyFile*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseProperty(PropertyFile*, char*, uint_fast64_t);

#endif