#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "util.h"

#include "linkedList.h"
#include <stdint.h>

#define PROPERTIES_SIZEOF_PROPERTY(property) ((2 * sizeof(int_fast64_t)) + sizeof(int8_t) + (sizeof(int8_t) * (property->nameLength + property->dataLength)))

#define PROPERTIES_PROPERTY_NAME(property) ((char*) property->data)
#define PROPERTIES_PROPERTY_DATA(property) (property->data + (property->nameLength + 1))

// NOTE: A variable of type 'ERROR_CODE' named 'error' has to be in scope. (jan - 2000.06.08)
#define PROPERTY_EXISTS(name)do{ \
	if(!properties_propertyExists(&server->properties, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))){ \
		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Failed to retrieve property: '%s' from property file: '%s'.", CONSTANTS_ ## name ## _PROPERTY_NAME, server->properties.filePath); \
		\
		error = ERRROR_MISSING_PROPERTY;\
	} \
}while(0)

#define PROPERTIES_GET(propertyFile, name) properties_get(propertyFile, CONSTANTS_ ## name ## _PROPERTY_NAME, strlen(CONSTANTS_ ## name ## _PROPERTY_NAME))

#define PROPERTIES_IS_ENTRY_PROPERTY(entry) (!PROPERTIES_IS_ENTRY_SECTION(entry) &&!PROPERTIES_IS_ENTRY_COMMENT(entry) && !PROPERTIES_IS_ENTRY_EMPTY_LINE(entry))
#define PROPERTIES_IS_ENTRY_COMMENT(entry) (entry->dataLength == -1)
#define PROPERTIES_IS_ENTRY_SECTION(entry) (entry->dataLength == 0 && !PROPERTIES_IS_ENTRY_EMPTY_LINE(entry))
#define PROPERTIES_IS_ENTRY_EMPTY_LINE(entry) (entry->nameLength == 0)

typedef struct version{
	uint_fast8_t release;
	uint_fast8_t update;
	uint_fast8_t hotfix;
}Version;

typedef struct{
	// Note: If the nameLength == 0, then an empty line was inserted in the settings file. (jan - 2022.11.21)
	uint_fast64_t nameLength;
	// Note: If dataLength == -1 then this property represents a comment in the settings file. And if dataLength == 0 and nameLength != 0 then this property represents a new Section in the settings file. (jan - 2022.11.21)
	int_fast64_t dataLength;
	union{
			struct{
				char* name;
				LinkedList properties;
			};
		// NOTE: Due to missing complier/language support for variable length members in unions we have to set data to a static size here. (jan - 2022.11.22)
		int8_t data[1];
	};
}PropertyFileEntry;

typedef struct{
	char* filePath;
	uint_fast64_t filePathLength;
	LinkedList sections;
	LinkedList properties;
}PropertyFile;

typedef PropertyFileEntry Property;

local const Version VERSION = {1, 0, 0};

ERROR_CODE properties_load(PropertyFile*, const char*, const uint_fast64_t);

ERROR_CODE properties_initProperty(Property**, char*, const int_fast64_t, int8_t*, const int_fast64_t);

ERROR_CODE properties_initPropertyFileSection(PropertyFileEntry**, char*, const uint_fast64_t);

ERROR_CODE properties_addPropertyFileEntry(PropertyFile*, PropertyFileEntry*, Property*);

Property* properties_get(PropertyFile*, const char*, const uint_fast64_t);

ERROR_CODE properties_updateProperty(PropertyFile*, const char*, const uint_fast64_t, const int8_t data[]);

ERROR_CODE properties_parse(PropertyFile*, char*, uint_fast64_t);

ERROR_CODE properties_parseLine(PropertyFile*, PropertyFileEntry**, char*, uint_fast64_t);

bool properties_propertyExists(PropertyFile*, const char*, const uint_fast64_t);

void properties_free(PropertyFile*);

#endif