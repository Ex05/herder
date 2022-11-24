#ifndef PROPERTIES_C
#define PROPERTIES_C

#include "properties.h"
#include "doublyLinkedList.h"
#include "linkedList.h"
#include "util.h"

#include "doublyLinkedList.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h>


#define PROPERTY_FILE_READ_BUFFER 8192

ERROR_CODE properties_loadFromDisk(PropertyFile* properties, const char* filePath, const uint_fast64_t filePathLength){
	ERROR_CODE error;

	properties->filePathLength = filePathLength;
	properties->filePath = malloc(sizeof(*properties->filePath) * (filePathLength + 1));
	memcpy(properties->filePath, filePath, filePathLength + 1);

	memset(&properties->properties, 0, sizeof(properties->properties));

	uint_fast64_t fileSize;
	if((error = util_getFileSize(filePath, &fileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	u_int8_t* readBuffer = malloc(sizeof(*readBuffer) * fileSize);
	if(readBuffer == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	FILE* filePtr = fopen(properties->filePath, "r");
	if(filePtr == NULL){
		return ERROR_(ERROR_FAILED_TO_OPEN_FILE, "File: '%s'.", filePath);
	}

	if(fread(readBuffer, 1, fileSize, filePtr) != fileSize){
		fclose(filePtr);

		return ERROR_(ERROR_FAILED_TO_LOAD_FILE, "Failed to load file: '%s'.", filePath);
	}
	
	fclose(filePtr);

	error = properties_parse(properties, (char*) readBuffer, fileSize);

	return ERROR(error);
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
	// Adds the property either to the current property file section, or the property file directly if no section is passed.
	doublyLinkedList_add(((section != NULL) ? (&section->properties) : (&properties->properties)), &property, sizeof(Property*));

	return ERROR(ERROR_NO_ERROR);
}

inline local void _properties_free(DoublyLinkedList* list){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* property = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		if(PROPERTIES_IS_PROPERTY_FILE_ENTRY_PROPERTY_FILE_SECTION(property)){
			_properties_free(list);

			doublyLinkedList_free(list);

			free(property->name);
		}

		free(property);
	}
}

inline void properties_free(PropertyFile* properties){
	_properties_free(&properties->properties);

	free(properties->filePath);
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

inline local Property* _properties_get(DoublyLinkedList* list, const char* name, const uint_fast64_t nameLength){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	// Iterate over all property file entries.
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* property = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		// If an entry is a section, search in the section.
		if(PROPERTIES_IS_PROPERTY_FILE_ENTRY_PROPERTY_FILE_SECTION(property)){
			Property* _property = _properties_get(&property->properties, name, nameLength);
			if(_property != NULL){
				return _property;
			}
		}else if(PROPERTIES_IS_PROPERTY_FILE_ENTRY_PROPERTY(property)){
			if(strncmp(name, PROPERTIES_PROPERTY_NAME(property), nameLength + 1) == 0){
				return property;
			}
		}
	}

	return NULL;
}

inline Property* properties_get(PropertyFile* properties, const char* name, const uint_fast64_t nameLength){
	return _properties_get(&properties->properties, name, nameLength);
}

inline bool properties_propertyExists(PropertyFile* propertyFile, const char* name, const uint_fast64_t nameLength){
	return properties_get(propertyFile, name, nameLength) != NULL;
}

ERROR_CODE properties_parse(PropertyFile* properties, char* buffer, uint_fast64_t bufferSize){
	PropertyFileEntry* currentPropertyFileSection = NULL;

	uint_fast64_t readOffset;
	uint_fast64_t lineNumber;
	for(lineNumber = 0, readOffset = 0; readOffset < bufferSize ;lineNumber++){
		const int_fast64_t lineSplitt = util_findFirst((char*) (buffer + readOffset), bufferSize - readOffset, CONSTANTS_CHARACTER_LINE_FEED);

		// Current line.
		char* line = (char*) (buffer + readOffset);

		uint_fast64_t lineLength;
		// The last line in every file will not be terminated by a line feed character so lineSplitt will be '-1'.
		if(lineSplitt == -1){
			lineLength = bufferSize - readOffset;
		}else{
			lineLength = lineSplitt;
		}		

		line = util_trim(line, &lineLength);

		ERROR_CODE error;
		if((error = properties_parseLine(properties, &currentPropertyFileSection, line, lineLength) != ERROR_NO_ERROR)){
			// Because line is not '\0' terminated an we would overwrite data if a line is not line feed terminated, we create a '\0' termianted temp copy of line for our error message.
			char* _line = alloca(sizeof(*_line) * lineLength + 1);
			memcpy(_line, line, lineLength);
			_line[lineLength] = '\0';

			return ERROR_(error, "Failed to parse line: (%" PRIuFAST64 ") '%s'.", lineNumber, _line);
		}

		// Advance read offset by line length plus the line feed character.
		readOffset += lineLength + 1;
	}

	return ERROR(ERROR_NO_ERROR);
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

ERROR_CODE properties_updateProperty(PropertyFile* properties, const char* name, const uint_fast64_t nameLength, const int8_t data[]){
	// TODO: ...

	return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
}

#endif