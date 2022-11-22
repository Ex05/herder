#ifndef PROPERTIES_C
#define PROPERTIES_C

#include "properties.h"
#include "linkedList.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h>

#define PROPERTY_FILE_READ_BUFFER 8192

ERROR_CODE properties_load(PropertyFile* propertyFile, const char* filePath, const uint_fast64_t filePathLength){
	propertyFile->filePathLength = filePathLength;
	propertyFile->filePath = malloc(sizeof(*propertyFile->filePath) * (filePathLength + 1));
	memcpy(propertyFile->filePath, filePath, filePathLength + 1);

	memset(&propertyFile->properties, 0, sizeof(propertyFile->properties));

	FILE* filePtr = fopen(propertyFile->filePath, "r");

	if(filePtr == NULL){
		return ERROR_(ERROR_FAILED_TO_OPEN_FILE, "File: '%s'.", filePath);
	}

	PropertyFileEntry* currentPropertyFileSection = NULL;

	u_int8_t readBuffer[PROPERTY_FILE_READ_BUFFER];
	uint_fast64_t lineNumber = 0;
	// TODO: Add handling of running out of buffer space and having to copy not completly parsed buffer into the ne buffer before continuing to read from disk. (jan - 2022.05.16)
	for(;;){
		uint_fast64_t bytesRead = fread(readBuffer, 1, PROPERTY_FILE_READ_BUFFER, filePtr);

		uint_fast64_t readOffset = 0;
		for(;;){
			lineNumber += 1;

			const int_fast64_t lineSplitt = util_findFirst((char*) (readBuffer + readOffset), bytesRead - readOffset, 0x0a/*Line feed character*/);

			// Current line.
			char* line = (char*) (readBuffer + readOffset);

			uint_fast64_t lineLength;
			// The last line in every file will not be terminated by a line feed character so lineSplitt will be '-1'.
			if(lineSplitt == -1){
				lineLength = bytesRead - readOffset;

				UTIL_LOG_CONSOLE_(LOG_DEBUG, "LineLength: '%" PRIuFAST64 "'", lineLength);
			}else{
				lineLength = lineSplitt;
			}		

			line = util_trim(line, &lineLength);

			ERROR_CODE error;
			if((error = properties_parseLine(propertyFile, &currentPropertyFileSection, line, lineLength) != ERROR_NO_ERROR)){
				// Termianted string to print an acceptable error message.
				// NOTE: This will remove the last character of the line if we are on the last line of the file. (jan - 2022.11.19)
				line[lineLength] = '\0';

				UTIL_LOG_CONSOLE_(LOG_DEBUG, "Failed to parse line: '%s' of '%s' (%s)", line, filePath, util_toErrorString(error));
			}

			// Advance read offset by line length plus the line feed character.
			readOffset += lineSplitt + 1;
			if(readOffset >= bytesRead){
				break;
			}
		}

		if(bytesRead < PROPERTY_FILE_READ_BUFFER){
			if(feof(filePtr) != 0){
				// End of file.
				break;
			}
		}else{
			return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
		}
	}

	fclose(filePtr);

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_initPropertyFileSection(PropertyFileEntry** section, char* name, uint_fast64_t nameLength){
	(*section) = malloc(sizeof((**section)));
	if((*section) == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	// Make sure properties is '0' initialised.
	memset((*section), 0 , sizeof(PropertyFileEntry));

	(*section)->nameLength = nameLength;

	(*section)->name = malloc(sizeof(*(*section)->name * (nameLength + 1)));
	if((*section)->name == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	memcpy((*section)->name, name, nameLength);
	
	*section = (*section);

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_addPropertyFileEntry(PropertyFile* properties, PropertyFileEntry* section, Property* property){
	linkedList_add((section != NULL ? (&section->properties) : (&properties->properties)), &property, sizeof(Property*));

	return ERROR(ERROR_NO_ERROR);
}

void properties_free(PropertyFile* propertyFile){
	linkedList_free(&propertyFile->properties);

	free(propertyFile->filePath);
}

inline ERROR_CODE properties_parseLine(PropertyFile* properties, PropertyFileEntry** section, char* line, uint_fast64_t lineLength){	
	ERROR_CODE error;

	Property* property;

	// Empty line
	if(lineLength == 0){
		*section = NULL;
		
		// Create a property with dataLength 0 to represent an empty line.
		if((error = properties_initProperty(&property, NULL, 0, NULL, 0)) != ERROR_NO_ERROR){
			return ERROR(error);
		}

		goto label_addProperty;
	}

	// Remove leading and trailing white space from line.
	line = util_trim(line, &lineLength);

	if(util_stringStartsWith(line, '#')){
		// Skip leading '#' symbol.
		line += 1;
		lineLength -= 1;

		line = util_trim(line, &lineLength);

		if((error = properties_initPropertyFileSection(&property, line, lineLength)) != ERROR_NO_ERROR){
			return ERROR(error);
		}

		properties_addPropertyFileEntry(properties, (*section != NULL ? (*section) : NULL), property);

		// Make _section the currently active section.
		*section = property;

		return ERROR(ERROR_NO_ERROR);
	}else if(util_stringStartsWith_s(line, "//", 2)){
			if((error = properties_initProperty(&property, line, lineLength, NULL, -1)) != ERROR_NO_ERROR){
				return ERROR(error);
			}

			goto label_addProperty;
		}else if(strncmp(line, "Version:", 8) == 0){
			// TODO: Handle different versions. (jan - 2022.05.16)

			return ERROR(ERROR_NO_ERROR);
		}

	// Name.
	const int_fast64_t nameSplitt = util_findFirst(line, lineLength, '=');

	if(nameSplitt == -1){
		return ERROR(ERROR_INVALID_VALUE);
	}

	line[nameSplitt] = '\0';
	lineLength = nameSplitt;
	line = util_trim(line, &lineLength);

	char* name = line;
	const int_fast64_t nameLength = lineLength;

	line += nameSplitt + 1;

	lineLength = strlen(line);
	line = util_trim(line, &lineLength);

	// Value.
	char* value = line;
	const int_fast64_t valueLength = lineLength;

	if((error = properties_initProperty(&property, name, nameLength, (int8_t*) value, valueLength)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

label_addProperty:
	return ERROR(properties_addPropertyFileEntry(properties, (*section != NULL ? (*section) : NULL), property));
}

inline Property* properties_get(PropertyFile* propertyFile, const char* name, const uint_fast64_t nameLength){
	LinkedListIterator it;
	linkedList_initIterator(&it, &propertyFile->properties);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* _property = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		if(PROPERTIES_IS_ENTRY_SECTION(_property)){
			LinkedListIterator _it;
			linkedList_initIterator(&_it, &_property->properties);

			while(LINKED_LIST_ITERATOR_HAS_NEXT(&_it)){
				Property* _property = LINKED_LIST_ITERATOR_NEXT_PTR(&_it, Property);

				if(strncmp(name, PROPERTIES_PROPERTY_NAME(_property), (nameLength > _property->nameLength ? nameLength : _property->nameLength) + 1) == 0){
					return _property;
				}
			}
		}

		if(strncmp(name, PROPERTIES_PROPERTY_NAME(_property), (nameLength > _property->nameLength ? nameLength : _property->nameLength) + 1) == 0){
			return _property;
		}
	}

	return NULL;
}

inline bool properties_propertyExists(PropertyFile* propertyFile, const char* name, const uint_fast64_t nameLength){
	return properties_get(propertyFile, name, nameLength) != NULL;
}

ERROR_CODE properties_initProperty(Property** property, char* name, const int_fast64_t nameLength, int8_t* data, const int_fast64_t dataLength){
	*property = malloc((2 * sizeof(int_fast64_t)) + (nameLength + 1 + dataLength));

	if(*property == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	(*property)->nameLength = nameLength;
	(*property)->dataLength = dataLength;

	if(name != NULL){
		memcpy(&(*property)->data, name, sizeof(*name) * (nameLength + 1));
	}

	if(data != NULL){
		memcpy(((*property)->data + nameLength + 1), data, dataLength);
	}

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_updateProperty(PropertyFile* propertyFile, const char* name, const uint_fast64_t nameLength, const int8_t data[]){
	// TODO: ...

	return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
}

#endif