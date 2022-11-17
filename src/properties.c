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

	u_int8_t readBuffer[PROPERTY_FILE_READ_BUFFER];
	uint_fast64_t lineNumber = 0;
	// TODO: Add handling of running out of bnuffer space and having to copy not completly parsed buffer into the ne buffer before continuing to read from disk. (jan - 2022.05.16)
	for(;;){
		uint_fast64_t bytesRead = fread(readBuffer, 1, PROPERTY_FILE_READ_BUFFER, filePtr);

		uint_fast64_t readOffset = 0;
		for(;;){
			lineNumber += 1;

			const int_fast64_t lineSplitt = util_findFirst((char*) (readBuffer + readOffset), bytesRead - readOffset, 0x0a);

			char* line = (char*) (readBuffer + readOffset);

			if(lineSplitt != -1){
				line[lineSplitt] = '\0';

				line = util_trim(line, lineSplitt);
			}else{
				line = util_trim(line, bytesRead - readOffset);
			}		 

			if(util_stringStartsWith(line, '#')){
				goto label_continue;
			}else{
				if(util_stringStartsWith_s(line, "//", 2)){
					goto label_continue;
				}else if(strncmp(line, "Version:", 8) == 0){
					// TODO: Handle different versions. (jan - 2022.05.16)
					goto label_continue;
				}
			}

			const int_fast64_t lineLength = strlen(line);

			if(lineLength == 0){
				goto label_continue;
			}

			// Name.
			const int_fast64_t nameSplitt = util_findFirst(line, lineLength, '=');

			if(nameSplitt == -1){
				UTIL_LOG_CONSOLE_(LOG_INFO, "Invalid property defenition in '%s' at line: %" PRIiFAST64 ". \"%s\" missing assignment.", filePath, lineNumber, line);

				goto label_continue;
			}

			line[nameSplitt] = '\0';
			line = util_trim(line, nameSplitt);

			char* name = line;
			const int_fast64_t nameLength = strlen(name);

			line += nameSplitt + 1;
			line = util_trim(line, strlen(line));

			// Value.
			char* value = line;
			const int_fast64_t valueLength = strlen(value);

			ERROR_CODE error;
			Property* property;
			if((error = properties_initProperty(&property, name, nameLength, (int8_t*) value, valueLength)) != ERROR_NO_ERROR){
				return ERROR(error);
			}

			linkedList_add(&propertyFile->properties, property, PROPERTIES_SIZEOF_PROPERTY(property));

			free(property);

		label_continue:
			if(lineSplitt == -1){
				break;
			}

			readOffset += lineSplitt + 1;
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

void properties_free(PropertyFile* propertyFile){
	linkedList_free(&propertyFile->properties);

	free(propertyFile->filePath);
}

inline Property* properties_get(PropertyFile* propertyFile, const char* name, const int_fast64_t nameLength){
	LinkedListIterator it;
	linkedList_initIterator(&it, &propertyFile->properties);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Property* _property = (Property*) LINKED_LIST_ITERATOR_NEXT(&it);

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

	memcpy(&(*property)->data, name, sizeof(*name) * (nameLength + 1));
	memcpy(((*property)->data + nameLength + 1), data, dataLength);

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE properties_updateProperty(PropertyFile* propertyFile, const char* name, const uint_fast64_t nameLength, const int8_t data[]){
	


	return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
}

inline void properties_propertyFileTemplateSetVersion(PropertyFileTemplate* template, Version version){
	template->version.release = version.release;
	template->version.update = version.update;
	template->version.hotfix = version.hotfix;
}

inline void properties_propertyFileTemplateSetFilePath(PropertyFileTemplate* template, char* filePath, int_fast64_t filePathLength){
	template->filePathLength = filePathLength;
	template->filePath = malloc(sizeof(*template->filePath) * (filePathLength + 1));
	memcpy(template->filePath, filePath, filePathLength + 1);
}

inline void properties_propertyFileTemplateAddSection(PropertyFileTemplate* template, PropertyFileSection* section){
	linkedList_add(&template->sections, section, sizeof(*section));
}

inline void properties_propertyFileTemplateFileSectionAddProperty(PropertyFileSection* section, PropertyTemplate* property){
	linkedList_add(&section->properties, property, sizeof(*property));
}

void properties_propertyFileTemplateAddProperty(PropertyFileTemplate* template, PropertyTemplate* property){
	linkedList_add(&template->properties, property, sizeof(*property));
}

ERROR_CODE properties_createPropertyFileFromTemplate(PropertyFileTemplate* template){
	ERROR_CODE error = ERROR_NO_ERROR;

	if(util_fileExists(template->filePath)){
		return ERROR_(ERROR_ALREADY_EXIST, "The file:'%s' already exists.", template->filePath);
	}

	FILE* filePtr = fopen(template->filePath, "w");

	if(filePtr == NULL){
		return ERROR_(ERROR_FAILED_TO_CREATE_FILE, "Failed to create the file:'%s'.", template->filePath);
	}

	const char versionString[] = "Version: ";
	fwrite(versionString, sizeof(char), strlen(versionString), filePtr);

	char formatedNumber[UTIL_FORMATTED_NUMBER_LENGTH];

	uint_fast64_t stringLength = UTIL_FORMATTED_NUMBER_LENGTH;
	stringLength = snprintf(formatedNumber, stringLength, "%" PRIdFAST64, (long) template->version.release);
	fwrite(formatedNumber, sizeof(char), stringLength, filePtr);

	fwrite(".", sizeof(char), 1, filePtr);

	stringLength = UTIL_FORMATTED_NUMBER_LENGTH;
	stringLength = snprintf(formatedNumber, stringLength, "%" PRIdFAST64, (long) template->version.update);
	fwrite(formatedNumber, sizeof(char), stringLength, filePtr);
	fwrite(".", sizeof(char), 1, filePtr);

	stringLength = UTIL_FORMATTED_NUMBER_LENGTH;
	stringLength = snprintf(formatedNumber, stringLength, "%" PRIdFAST64, (long) template->version.hotfix);
	fwrite(formatedNumber, sizeof(char), stringLength, filePtr);

	fwrite("\n", sizeof(char), 1, filePtr);

	// Sections.
	LinkedListIterator sectionIterator;
	linkedList_initIterator(&sectionIterator, &template->sections);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&sectionIterator)){
		PropertyFileSection* section = LINKED_LIST_ITERATOR_NEXT(&sectionIterator);

		fwrite("\n# ", sizeof(char), 3, filePtr);
		fwrite(section->name, sizeof(char), section->nameLength, filePtr);
		fwrite("\n", sizeof(char), 1, filePtr);

		// Properties.
		LinkedListIterator propertyIterator;
		linkedList_initIterator(&propertyIterator, &section->properties);
		while(LINKED_LIST_ITERATOR_HAS_NEXT(&propertyIterator)){
			PropertyTemplate* property = LINKED_LIST_ITERATOR_NEXT(&propertyIterator);
			
			fwrite(property->name, sizeof(char), property->nameLength, filePtr);
			fwrite(" = ", sizeof(char), 3, filePtr);
			fwrite(property->value, sizeof(char), property->valueLength, filePtr);
			fwrite("\n", sizeof(char), 1, filePtr);
		}
	}

	// Properties.
	if(template->properties.length > 0){
		fwrite("\n", sizeof(char), 1, filePtr);
	}

	LinkedListIterator propertyIterator;
	linkedList_initIterator(&propertyIterator, &template->properties);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&propertyIterator)){
		PropertyTemplate* property = LINKED_LIST_ITERATOR_NEXT(&propertyIterator);

		fwrite(property->name, sizeof(char), property->nameLength, filePtr);
		fwrite(" = ", sizeof(char), 3, filePtr);
		fwrite(property->value, sizeof(char), property->valueLength, filePtr);
		fwrite("\n", sizeof(char), 1, filePtr);
	}

	fclose(filePtr);

	return ERROR(error);
}

void properties_freePropertyFileTemplate(PropertyFileTemplate* template){
	free(template->filePath);

	LinkedListIterator sectionIterator;
	linkedList_initIterator(&sectionIterator, &template->sections);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&sectionIterator)){
		PropertyFileSection* section = LINKED_LIST_ITERATOR_NEXT(&sectionIterator);

		LinkedListIterator propertyIterator;
		linkedList_initIterator(&propertyIterator, &section->properties);

		while(LINKED_LIST_ITERATOR_HAS_NEXT(&propertyIterator)){
			PropertyTemplate* property = LINKED_LIST_ITERATOR_NEXT(&propertyIterator);

			free(property->name);
			free(property->value);
		}

		free(section->name);
		
		linkedList_free(&section->properties);
	}

	linkedList_free(&template->sections);

	LinkedListIterator propertyIterator;
	linkedList_initIterator(&propertyIterator, &template->properties);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&propertyIterator)){
		PropertyTemplate* property = LINKED_LIST_ITERATOR_NEXT(&propertyIterator);

		free(property->name);
		free(property->value);
	}

	linkedList_free(&template->properties);
}

#endif