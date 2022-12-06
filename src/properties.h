#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "util.h"

#include "doublyLinkedList.h"
#include <stdint.h>

// NOTE: A variable of type 'ERROR_CODE' named 'error' has to be in scope. (jan - 2022.06.08)
#define PROPERTY_EXISTS(name) do{ \
	Property* property; \
	if((error = properties_get(&server->properties, &property, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))) != ERROR_NO_ERROR){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' could not be loaded from property file.", CONSTANTS_ ## name ## _PROPERTY_NAME); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(error); \
	} \
 \
	if(property->valueLength == 0){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' has not been set.", CONSTANTS_ ## name ## _PROPERTY_NAME); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERROR_NO_VALID_ARGUMENT); \
	} \
}while(0)

#define PROPERTIES_GET(propertyFile, property, name) properties_get(propertyFile, &property, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))

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

ERROR_CODE properties_loadFromDisk(PropertyFile*, const char*);

ERROR_CODE properties_saveToDisk(PropertyFile*, const char*);

ERROR_CODE properties_initProperty(Property**, PropertyFileEntryType, char*, const int_fast64_t, int8_t*, const int_fast64_t);

ERROR_CODE properties_addPropertyFileEntry(PropertyFile*, PropertyFileEntry*, Property*);

ERROR_CODE properties_get(PropertyFile*, Property**, const char*, const uint_fast64_t);

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