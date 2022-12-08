#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "util.h"

#include "doublyLinkedList.h"

#define STRING_PROPERTY_EXISTS PROPERTY_EXISTS

#define PROPERTY_EXISTS(server, name) Property* property ## name; do{ \
	ERROR_CODE error; \
	if((error = properties_get(&server->properties, &property ## name, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))) != ERROR_NO_ERROR){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' could not be loaded from property file.", CONSTANTS_ ## name ## _PROPERTY_NAME); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(error); \
	} \
 \
	if(property ## name->valueLength == 0){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' has not been set.", CONSTANTS_ ## name ## _PROPERTY_NAME); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERROR_NO_VALID_ARGUMENT); \
	} \
}while(0)

#define INTEGER_PROPERTY_EXISTS(server, name) PROPERTY_EXISTS(server, name); \
	int64_t value ## name; do{ \
	if((error = util_stringToInt(property ## name->value, &value ## name)) != ERROR_NO_ERROR){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' value '%s' has to be of type 'INTEGER'", CONSTANTS_ ## name ## _PROPERTY_NAME, property ## name->value); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERRROR_MISSING_PROPERTY); \
	} \
}while(0)

#define INTEGER_PROPERTY_OF_RANGE_EXISTS(server, name, min, max) INTEGER_PROPERTY_EXISTS(server, name); do{ \
	if(value ## name < min || value ## name > max){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tInvalid range for property '%s' value '%s' has to be in range of %d-%d.", CONSTANTS_ ## name ## _PROPERTY_NAME, property ## name->value, min, max); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
		\
		return ERROR(ERROR_INVALID_VALUE); \
	} \
 \
}while(0)

#define BOOLEAN_PROPERTY_EXISTS(server, name) PROPERTY_EXISTS(server, name); do{ \
	if(strncmp(property ## name->value, "true", 5) != 0 && strncmp(property ## name->value, "false", 6) != 0){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tProperty '%s' value '%s' has to be of type 'BOOLEAN'", CONSTANTS_ ## name ## _PROPERTY_NAME, property ## name->value); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERROR_INVALID_VALUE); \
	} \
}while(0)

#define DIRECTORY_PROPERTY_EXISTS(server, name) PROPERTY_EXISTS(server, name); do{ \
	if(!util_directoryExists(property ## name->value)){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tThe directory reffered to by the property '%s' '%s' does not exist.", CONSTANTS_ ## name ## _PROPERTY_NAME, property ## name->value); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERROR_INVALID_VALUE); \
	} \
}while(0)

#define FILE_PROPERTY_EXISTS(server, name) PROPERTY_EXISTS(server, name); do{ \
	if(!util_fileExists(property ## name->value)){ \
		UTIL_LOG_CONSOLE_(LOG_INFO, "Server:\t\tThe file reffered to by the property '%s' '%s' does not exist.", CONSTANTS_ ## name ## _PROPERTY_NAME, property ## name->value); \
		UTIL_LOG_CONSOLE(LOG_INFO, "Server:\t\tYou can use '--showSettings' to list all settings entries."); \
 \
		return ERROR(ERROR_INVALID_VALUE); \
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

ERROR_CODE properties_parseLine(PropertyFile*, PropertyFileEntry**, DoublyLinkedList*, char*, uint_fast64_t);

bool properties_propertyExists(PropertyFile*, const char*, const uint_fast64_t);

void properties_free(PropertyFile*);

local ERROR_CODE _properties_parseEmptyLine(PropertyFile*, DoublyLinkedList*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseComment(PropertyFile*, DoublyLinkedList*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseSection(PropertyFile*, DoublyLinkedList*, char*, uint_fast64_t);

local ERROR_CODE _properties_parseProperty(PropertyFile*, DoublyLinkedList*, char*, uint_fast64_t);

#endif