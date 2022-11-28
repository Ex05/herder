#ifndef PROPERTIES_C
#define PROPERTIES_C

#include "properties.h"
#include "doublyLinkedList.h"
#include "util.h"

#include "doublyLinkedList.c"
#include <sys/syslog.h>

ERROR_CODE properties_loadFromDisk(PropertyFile* properties, const char* filePath){
	ERROR_CODE error;

	uint_fast64_t fileSize;
	if((error = util_getFileSize(filePath, &fileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	u_int8_t* readBuffer = malloc(sizeof(*readBuffer) * fileSize);
	if(readBuffer == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	FILE* filePtr = fopen(filePath, "r");
	if(filePtr == NULL){
		return ERROR_(ERROR_FAILED_TO_OPEN_FILE, "File: '%s'.", filePath);
	}

	if(fread(readBuffer, 1, fileSize, filePtr) != fileSize){
		fclose(filePtr);

		return ERROR_(ERROR_FAILED_TO_LOAD_FILE, "Failed to load file: '%s'.", filePath);
	}
	
	fclose(filePtr);

	return ERROR(properties_parse(properties, (char*) readBuffer, fileSize));
}

ERROR_CODE _properties_writeToDisk(DoublyLinkedList* properties, FILE* filePtr){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, properties);

	// Iterate over all property file entries.
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* property = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		switch (property->type){
			case PROPERTY_FILE_ENTRY_TYPE_PROPERTY:{
				if(fwrite(property->name, 1, property->nameLength, filePtr) != property->nameLength){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write property name.");
				}

				if(fwrite(" = ", 1, 3, filePtr) != 3){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write property name value seperator.");
				}

				if(fwrite(property->value, 1, property->valueLength, filePtr) != property->valueLength){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write property value.");
				}

				break;
			}

			case PROPERTY_FILE_ENTRY_TYPE_COMMENT:{
				if(fwrite(property->name, 1, property->nameLength, filePtr) != property->nameLength){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write property name.");
				}

				break;
			}

			case PROPERTY_FILE_ENTRY_TYPE_SECTION:{
				if(fwrite("# ", 1, 2, filePtr) != 2){
					return ERROR_(ERROR_WRITE_ERROR, "Failed to write section marker for section: '%s'.", property->name );
				}

				if(fwrite(property->name, 1, property->nameLength, filePtr) != property->nameLength){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write property name.");
				}

				if(fwrite("\n", 1, 1, filePtr) != 1){
					return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write line feed character.");
				}

				ERROR_CODE error;
				if((error = _properties_writeToDisk(&property->properties, filePtr)) != ERROR_NO_ERROR){
					return ERROR(error);
				}

				break;
			}

			default:{
				break;
			}
		}

		if(property->type != PROPERTY_FILE_ENTRY_TYPE_SECTION){
			if(fwrite("\n", 1, 1, filePtr) != 1){
				return ERROR_(ERROR_WRITE_ERROR, "%s", "Failed to write line feed character.");
			}
		}
	}

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_saveToDisk(PropertyFile* properties, const char* filePath){
	FILE* filePtr = fopen(filePath, "w");
	if(filePtr == NULL){
		return ERROR_(ERROR_FAILED_TO_OPEN_FILE, "File: '%s'.", filePath);
	}

	ERROR_CODE error;
	if((error = _properties_writeToDisk(&properties->properties, filePtr)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	fclose(filePtr);

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_addPropertyFileEntry(PropertyFile* properties, PropertyFileEntry* section, Property* property){
	// Adds the property either to the current property file section, or the property file directly if no section is passed.
	doublyLinkedList_add(&properties->properties, &property, sizeof(Property*));

	return ERROR(ERROR_NO_ERROR);
}

inline local void _properties_free(DoublyLinkedList* list){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	// Iterate over all property file entries.
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* property = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		free(property->name);

		switch (property->type){
			case PROPERTY_FILE_ENTRY_TYPE_PROPERTY:{
				free(property->data);

				break;
			}
		
			case PROPERTY_FILE_ENTRY_TYPE_SECTION:{
				_properties_free(&property->properties);
				break;
			}

			default:{
				break;
			}
		}

		free(property);
	}

	doublyLinkedList_free(list);
}

inline void properties_free(PropertyFile* properties){
	_properties_free(&properties->properties);
}

inline ERROR_CODE _properties_parseEmptyLine(PropertyFile* properties, char* line, uint_fast64_t lineLength){
	ERROR_CODE error;
	
	Property* property;
	if((error = properties_initProperty(&property, PROPERTY_FILE_ENTRY_TYPE_EMPTY_LINE, line, lineLength, NULL, 0)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = properties_addPropertyFileEntry(properties, NULL, property)) != ERROR_NO_ERROR){
			return ERROR(error);
	}

	return ERROR(error);
}

inline ERROR_CODE _properties_parseComment(PropertyFile* properties, char* line, uint_fast64_t lineLength){
	ERROR_CODE error;
	
	Property* property;
	if((error = properties_initProperty(&property, PROPERTY_FILE_ENTRY_TYPE_COMMENT, line, lineLength, NULL, 0)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = properties_addPropertyFileEntry(properties, NULL, property)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(error);
}

inline ERROR_CODE _properties_parseSection(PropertyFile* properties, char* line, uint_fast64_t lineLength){
	ERROR_CODE error;
	
	Property* property;
	if((error = properties_initProperty(&property, PROPERTY_FILE_ENTRY_TYPE_SECTION, line, lineLength, NULL, 0)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = properties_addPropertyFileEntry(properties, NULL, property)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(error);
}

inline ERROR_CODE _properties_parseProperty(PropertyFile* properties, char* line, uint_fast64_t lineLength){
	ERROR_CODE error = ERROR_NO_ERROR;

	const int_fast64_t posSplitt = util_findFirst(line, lineLength, '=');
	if(posSplitt == -1){
		return ERROR(ERROR_INVALID_VALUE);
	}
	
	// Name.
	char* name = line;
	uint_fast64_t nameLength = posSplitt;

	name = util_trim(name, &nameLength);

	// Value.
	char* value = line + posSplitt + 1;
	uint_fast64_t valueLength = lineLength - (posSplitt + 1);

	value = util_trim(value, &valueLength);

	Property* property;
	if((error = properties_initProperty(&property, PROPERTY_FILE_ENTRY_TYPE_PROPERTY, name, nameLength, (int8_t*) value, valueLength)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = properties_addPropertyFileEntry(properties, NULL, property)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(error);
}

ERROR_CODE properties_parseLine(PropertyFile* properties, PropertyFileEntry** section, char* line, uint_fast64_t lineLength){	
	// Remove leading and trailing white space from entire line.
	line = util_trim(line, &lineLength);

	// Empty line.
	if(lineLength == 0){		
		return ERROR(_properties_parseEmptyLine(properties, line, lineLength));
	}

	// Comment.
	if(lineLength >= 2 && util_stringStartsWith_s(line, "//", 2)){
		return ERROR(_properties_parseComment(properties, line, lineLength));
	}

	// Section.
	if(util_stringStartsWith(line, '#')){
		// Skip leading '#' symbol.
		line += 1;
		lineLength -= 1;

		// Trim the line again, now that we have removed the leading pound symbol.
		line = util_trim(line, &lineLength);

		return ERROR(_properties_parseSection(properties, line, lineLength));		
	}

	// Version.
	if(lineLength >= 8 && util_stringStartsWith_s(line, "Version", 7)){
		// TODO: Handle version line.

		return ERROR(_properties_parseComment(properties, line, lineLength));
	}

	// Property.
	return ERROR(_properties_parseProperty(properties, line, lineLength));
}

inline local Property* _properties_get(DoublyLinkedList* list, const char* name, const uint_fast64_t nameLength){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	// Iterate over all property file entries.
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* property = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Property);

		if(property->type == PROPERTY_FILE_ENTRY_TYPE_SECTION){
			 if((property = _properties_get(&property->properties, name, nameLength)) != NULL){
				return property;
			 }
		}else if(property->type == PROPERTY_FILE_ENTRY_TYPE_PROPERTY){
			if(strncmp(name, property->name, nameLength) == 0){
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
	// PropertyFileEntry* currentPropertyFileSection = NULL;

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

		ERROR_CODE error;
		if((error = properties_parseLine(properties, NULL, line, lineLength) != ERROR_NO_ERROR)){
			return ERROR(error);
		}

		// Advance read offset by line length plus the line feed character.
		readOffset += lineLength + 1;
	}

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_initProperty(Property** property, PropertyFileEntryType type, char* name, const int_fast64_t nameLength, int8_t* data, const int_fast64_t dataLength){
	*property = malloc(sizeof(**property));
	if(*property == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}
	
	// Zero initialise struct.
	memset(*property, 0, sizeof(**property));

	(*property)->type = type;

	switch (type) {
		case PROPERTY_FILE_ENTRY_TYPE_SECTION:
		case PROPERTY_FILE_ENTRY_TYPE_PROPERTY:
		case PROPERTY_FILE_ENTRY_TYPE_COMMENT:{
			(*property)->name = malloc(sizeof(*name) * (nameLength + 1));
			if((*property)->name == NULL){
				return ERROR(ERROR_OUT_OF_MEMORY);
			}

			memcpy((*property)->name, name, nameLength);
			(*property)->name[nameLength] = '\0';

			(*property)->nameLength = nameLength;

			if(type == PROPERTY_FILE_ENTRY_TYPE_COMMENT || type == PROPERTY_FILE_ENTRY_TYPE_SECTION){
				break;
			}

			(*property)->data = malloc(sizeof(*data) * (dataLength + 1));
			if((*property)->data == NULL){
				return ERROR(ERROR_OUT_OF_MEMORY);
			}

			memcpy((*property)->data, data, dataLength);
			(*property)->dataLength = dataLength;

			(*property)->data[dataLength] = '\0';
		}

		default:{
			break;
		}
	}

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_updateProperty(PropertyFile* properties, const char* name, const uint_fast64_t nameLength, const int8_t data[]){
	// TODO: ...

	return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
}

#endif