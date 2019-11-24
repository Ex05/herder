#ifndef PROPERTY_FILE_C
#define PROPERTY_FILE_C

#include "propertyFile.h"

#include "linkedList.c"

#define PAGE_ENTRY_SIZE (sizeof(uint64_t) * 4)

local const Version VERSION = {1, 0, 0};

local ERROR_CODE propertyFile_initProperty(Property*, PropertyFileEntry*, PropertyFile*);

local ERROR_CODE propertyFile_addPropertyPage(PropertyFile*, PropertyPage**);

local ERROR_CODE propertyFile_initPropertyPage(PropertyPage*, const uint8_t, const uint_fast64_t);

ERROR_CODE propertyFile_init(PropertyFile* propertyFile, const char* fileName){
    propertyFile->file = fopen(fileName, "r+");

    if(propertyFile->file == NULL){
        return ERROR(ERROR_FAILED_TO_OPEN_FILE);
    }

    ERROR_CODE error;
    if((error = linkedList_init(&propertyFile->pages) != ERROR_NO_ERROR)){
        return error;
    }

    FILE* file = propertyFile->file;

    if(fread(&propertyFile->version, 1, sizeof(propertyFile->version), file) != sizeof(propertyFile->version)){
        return ERROR_(ERROR_READ_ERROR, "Failed to read property file version. '%s'", fileName);
    }

    if(VERSION.release != propertyFile->version.release && VERSION.update == propertyFile->version.update && VERSION.hotfix == propertyFile->version.hotfix){

        return ERROR_(ERROR_VERSION_MISSMATCH, "Version miss match [%" PRIu8 ".%" PRIu8 ".%" PRIu8 "].", propertyFile->version.release, propertyFile->version.update, propertyFile->version.hotfix);
    }

    if(fread(&propertyFile->maxPageEntries, 1, sizeof(propertyFile->maxPageEntries), file) != sizeof(propertyFile->maxPageEntries)){

        return ERROR_(ERROR_READ_ERROR, "Failed to read Max_Page_Entries. '%s'.", fileName);
    }

    int8_t readBuffer[sizeof(uint64_t)];

    for(;;){
        PropertyPage* propertyPage = malloc(sizeof(*propertyPage));
        if(propertyPage == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        propertyFile_initPropertyPage(propertyPage, propertyFile->maxPageEntries, ftell(file));

        linkedList_add(&propertyFile->pages, propertyPage);

        int i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            entry->entryOffset = ftell(file);

            // Name_Size.
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                return ERROR_(ERROR_READ_ERROR, "Failed to read Name_Size. '%s'", fileName);
            }

            entry->nameLength = util_byteArrayTo_uint64(readBuffer);

            // Name_Offset.
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                return ERROR_(ERROR_READ_ERROR, "Failed to read Name_Offset. '%s'", fileName);
            }

            entry->nameOffset = util_byteArrayTo_uint64(readBuffer);

            // Data_Size.
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                return ERROR_(ERROR_READ_ERROR, "Failed to read Data_Size. '%s'", fileName);
            }

            entry->length = util_byteArrayTo_uint64(readBuffer);

            // Data_Offset.
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                return ERROR_(ERROR_READ_ERROR, "Failed to read Data_Offset. '%s'", fileName);
            }

            entry->dataOffset = util_byteArrayTo_uint64(readBuffer);

            if(entry->nameOffset != 0 && entry->nameLength != 0){
                const uint_fast64_t fileOffset = ftell(file);

                if(fseek(file, entry->nameOffset, SEEK_SET) != 0){
                    return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
                }

                entry->name = malloc(sizeof(*entry->name) * (entry->nameLength + 1));     
                if(entry->name == NULL){
                    return ERROR(ERROR_OUT_OF_MEMORY);
                }

                entry->name[entry->nameLength] = '\0';

                if(fread(entry->name, sizeof(*entry->name), entry->nameLength, file) != entry->nameLength){
                    return ERROR_(ERROR_READ_ERROR, "Failed to read entry name. '%s'", fileName);
                }

                if(fseek(file, fileOffset, SEEK_SET) != 0){
                    return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
                }
            }else{
                entry->name = NULL;
            }

            propertyPage->entries[i] = entry;
        }

        if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
            return ERROR_(ERROR_READ_ERROR, "Failed to read next page offset. '%s'", fileName);
        }

        const uint_fast64_t nextPageOffset = util_byteArrayTo_uint64(readBuffer);

        if(nextPageOffset == 0){
            break;
        }
        
        if(fseek(file, nextPageOffset, SEEK_SET) == -1){
            return ERROR_(ERROR_DISK_ERROR, "Failed to seek to the next page ffset. [%" PRIdFAST64 "] '%s'.", nextPageOffset, fileName);
        }
    }

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE propertyFile_create(const char* fileName, const uint8_t numPageEntries){
    FILE* file = fopen(fileName, "a");

    if(file == NULL){
        return ERROR_(ERROR_FAILED_TO_CREATE_FILE, " %s '%s'", strerror(errno), fileName);
    }

    // Current code/PropertyFile version.
    uint8_t buffer[PAGE_ENTRY_SIZE] = {0};
    buffer[0] = VERSION.release;    
    buffer[1] = VERSION.update;    
    buffer[2] = VERSION.hotfix;

    if(fwrite(buffer, 1, sizeof(Version), file) != sizeof(Version)){
        return ERROR_(ERROR_WRITE_ERROR, "%s.", strerror(errno));
    }

    memset(buffer, 0, sizeof(Version));

    // Max_Page_Entries.
    buffer[0] = numPageEntries;

    if(fwrite(buffer, 1, sizeof(numPageEntries), file) != sizeof(numPageEntries)){
        return ERROR_(ERROR_WRITE_ERROR, "%s.", strerror(errno));
    }

    buffer[0] = 0;

    // Empty page entries.
    uint8_t i;
    for(i = 0; i < numPageEntries; i++){
        if(fwrite(buffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
            return ERROR_(ERROR_WRITE_ERROR, "%s.", strerror(errno));
        }
    }

    // Next_Page.
    if(fwrite(buffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        return ERROR_(ERROR_WRITE_ERROR, "%s.", strerror(errno));
    }

    if(fflush(file) != 0){
        return ERROR(ERROR_DISK_ERROR);
    }

    if(fclose(file) != 0){
        return ERROR(ERROR_DISK_ERROR);
    }

    return ERROR(ERROR_NO_ERROR);    
}

inline ERROR_CODE propertyFile_getProperty(PropertyFile* propertyFile, Property** property, const char* name){
    LinkedListIterator it;
    linkedList_initIterator(&it, &propertyFile->pages);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = LINKED_LIST_ITERATOR_NEXT(&it);

        uint_fast8_t i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            if(entry->name != NULL && strncmp(entry->name, name, entry->nameLength) == 0){
                Property* _property = malloc(sizeof(*_property));
                if(_property == NULL){
                    return ERROR(ERROR_OUT_OF_MEMORY);
                }

                propertyFile_initProperty(_property, entry, propertyFile);

                *property = _property;

                return ERROR(ERROR_NO_ERROR);
            }
        }
    }

    *property = NULL;

    return ERROR_(ERROR_ENTRY_NOT_FOUND, "Entry: '%s'", name);
}

inline ERROR_CODE propertyFile_initProperty(Property* property, PropertyFileEntry* entry, PropertyFile* propertyFile){
    property->entry = entry;
    property->callBack = propertyFile;

    property->buffer = malloc(sizeof(*property->buffer) * entry->length);
    if(property->buffer == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    if(fseek(propertyFile->file, entry->dataOffset, SEEK_SET) != 0){
        return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
    }
    
    if(fread(property->buffer, 1, entry->length, propertyFile->file) != entry->length){
        return ERROR_(ERROR_DISK_ERROR, "Failed to read property data. '%s'.", strerror(errno));
    }

    return ERROR(ERROR_NO_ERROR);
}

inline void propertyFile_free(PropertyFile* propertyFile){
    fclose(propertyFile->file);

    LinkedListIterator it;
    linkedList_initIterator(&it, &propertyFile->pages);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = LINKED_LIST_ITERATOR_NEXT(&it);

        uint_fast8_t i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            free(entry->name);
            free(entry);
        }

        free(propertyPage->entries);
        free(propertyPage);
    }

    linkedList_free(&propertyFile->pages);
}

inline bool propertyFile_contains(PropertyFile* propertyFile, const char* name){
    LinkedListIterator it;
    linkedList_initIterator(&it, &propertyFile->pages);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = LINKED_LIST_ITERATOR_NEXT(&it);

        uint_fast8_t i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            if(entry->name != NULL && strncmp(entry->name, name, entry->nameLength) == 0){
                return true;
            }
        }
    }

    return false;
}

ERROR_CODE propertyFile_addProperty(PropertyFile* propertyFile, Property** property, const char* name, const uint_fast64_t size){
    *property = NULL;

    if(propertyFile_contains(propertyFile, name)){
        return ERROR(ERROR_DUPLICATE_ENTRY);
    }

    PropertyFileEntry* entry = NULL;

    LinkedListIterator it;
    linkedList_initIterator(&it, &propertyFile->pages);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = LINKED_LIST_ITERATOR_NEXT(&it);

        int i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* _entry = propertyPage->entries[i];

            if(_entry->name == NULL && _entry->entryOffset != 0){
                entry = _entry;

                goto label_entryFound;
            }
        }
    }

    if(entry == NULL){
        ERROR_CODE error;
        PropertyPage* propertyPage;        
        if((error = propertyFile_addPropertyPage(propertyFile, &propertyPage)) != ERROR_NO_ERROR){
            return ERROR(error);
        }

        entry = propertyPage->entries[0];
    }

label_entryFound:
    entry->length = size;
    entry->nameLength = strlen(name);
    entry->name = malloc(sizeof(*entry->name) * (entry->nameLength + 1));
    if(entry->name == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    memcpy(entry->name, name, sizeof(*entry->name) * (entry->nameLength + 1));

    if(fseek(propertyFile->file, 0, SEEK_END) != 0){
        return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
    }

    entry->nameOffset = ftell(propertyFile->file);

    if(fwrite(entry->name, sizeof(*entry->name), entry->nameLength, propertyFile->file) != entry->nameLength){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write entry name '%s' to propertyFile", name);
    }

    uint8_t* voidBuffer = alloca(entry->length);
    memset(voidBuffer, 0, entry->length);

    entry->dataOffset = ftell(propertyFile->file);

    if(fwrite(voidBuffer, 1, entry->length, propertyFile->file) != entry->length){
        UTIL_LOG_ERROR("Failed to write empty propertyData.");

        return ERROR(ERROR_WRITE_ERROR);
    }

    // Update entry on disk.
    if(fseek(propertyFile->file, entry->entryOffset, SEEK_SET) != 0){
        return ERROR_(ERROR_DISK_ERROR, "Failed to write propertyData of proeprty '%s' to disk.", name);
    }

    int8_t writeBuffer[sizeof(uint64_t)];

    // Name_Size.
    util_uint64ToByteArray(writeBuffer, entry->nameLength);

    if(fwrite(writeBuffer, 1, sizeof(entry->nameLength), propertyFile->file) != sizeof(entry->nameLength)){
        UTIL_LOG_ERROR("");

        return ERROR_(ERROR_WRITE_ERROR, "Failed to write entry name size of property '%s' to propertyFile.", name);
    }

    // Name_Offset.
    util_uint64ToByteArray(writeBuffer, entry->nameOffset);

    if(fwrite(writeBuffer, 1, sizeof(entry->nameOffset), propertyFile->file) != sizeof(entry->nameOffset)){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write entry name offset of '%s' to propertyFile.", name);
    }

    // Data_Size.
    util_uint64ToByteArray(writeBuffer, entry->length);

    if(fwrite(writeBuffer, 1, sizeof(entry->length), propertyFile->file) != sizeof(entry->length)){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write entry size of property '%s' to propertyFile.", name);
    }

    // Data_Offset.
    util_uint64ToByteArray(writeBuffer, entry->dataOffset);

    if(fwrite(writeBuffer, 1, sizeof(entry->dataOffset), propertyFile->file) != sizeof(entry->dataOffset)){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write entry data offset of property '%s' to propertyFile.", name);
    }

    *property = malloc(sizeof(**property));
    if(*property == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    propertyFile_initProperty(*property, entry, propertyFile);

    return ERROR(ERROR_NO_ERROR);
}

inline void propertyFile_freeProperty(Property* property){    
    free(property->buffer);
}

inline ERROR_CODE propertyFile_setBuffer(Property* property, int8_t* buffer){
    const uint_fast64_t bufferSize = property->entry->length;

    memcpy(property->buffer, buffer, bufferSize);

    FILE* file = property->callBack->file;

    if(fseek(file, property->entry->dataOffset, SEEK_SET) != 0){
        return ERROR(ERROR_DISK_ERROR);
    } 

    if(fwrite(property->buffer, 1, bufferSize, file) != bufferSize){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write property data to file. '%s'.", strerror(errno));
    }

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE propertyFile_addPropertyPage(PropertyFile* propertyFile, PropertyPage** propertyPage){
    FILE* file = propertyFile->file;

    LinkedListIterator it;
    linkedList_initIterator(&it, &propertyFile->pages);

    uint_fast64_t prevPageOffset;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Node* node = linkedList_iteratorNextNode(&it);

        if(node->next == NULL){
            prevPageOffset = ((PropertyPage*) node)->offset;
        }
    }

    *propertyPage = malloc(sizeof(**propertyPage));
    if(*propertyPage == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    propertyFile_initPropertyPage(*propertyPage, propertyFile->maxPageEntries, ftell(file));

    int8_t* writeBuffer = alloca(PAGE_ENTRY_SIZE);
    memset(writeBuffer, 0 , PAGE_ENTRY_SIZE);
    
    uint_fast8_t i;
    for(i = 0; i < propertyFile->maxPageEntries; i++){
        (*propertyPage)->entries[i]->entryOffset = ftell(file);

        if(fwrite(writeBuffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write empty property entry to file. '%s'", strerror(errno));
        }
    }

    if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to write propertyPage offset to file. '%s'.", strerror(errno));
    }

    ERROR_CODE error;
    if((error = linkedList_add(&propertyFile->pages, *propertyPage)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(fseek(file, prevPageOffset + (PAGE_ENTRY_SIZE * propertyFile->maxPageEntries), SEEK_SET) != 0){
        return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
    }

    util_uint64ToByteArray(writeBuffer, (*propertyPage)->offset);

    if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        return ERROR_(ERROR_WRITE_ERROR, "Failed to update previous propertyPage offset. '%s'.", strerror(errno));
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE propertyFile_initPropertyPage(PropertyPage* propertyPage, const uint8_t maxPageEntries, const uint_fast64_t pageOffset){
    propertyPage->entries = malloc(sizeof(*propertyPage->entries) * maxPageEntries);
    if(propertyPage->entries == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    propertyPage->offset = pageOffset;

    uint_fast8_t i;
    for(i = 0; i < maxPageEntries; i++){
        propertyPage->entries[i] = malloc(sizeof(PropertyFileEntry));
        if(propertyPage->entries[i] == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }
    }

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE propertyFile_removeProperty(Property* property){
    FILE* file = property->callBack->file;

    LinkedListIterator it;
    linkedList_initIterator(&it, &property->callBack->pages);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = LINKED_LIST_ITERATOR_NEXT(&it);

        int i;
        for(i = 0; i < property->callBack->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            if(entry->name != NULL && strncmp(entry->name, property->entry->name, property->entry->nameLength) == 0){
                int8_t* writeBuffer = alloca(property->entry->length);
                memset(writeBuffer, 0, property->entry->length);

                // Clear data on disk.
                if(fseek(file, property->entry->dataOffset, SEEK_SET) != 0){
                    return ERROR_(ERROR_WRITE_ERROR, "Failed to move file pointer to property data on disk. '%s'.", strerror(errno)); 
                }

                if(fwrite(writeBuffer, 1, property->entry->length, file) != property->entry->length){
                    return ERROR_(ERROR_WRITE_ERROR, "Failed to erase property data on disk. '%s'.", strerror(errno));     
                }

                // Clear entry name on disk.
                if(property->entry->nameLength > property->entry->length){
                    writeBuffer = alloca(property->entry->nameLength);
                    memset(writeBuffer, 0, property->entry->nameLength);
                }

                if(fseek(file, property->entry->nameOffset, SEEK_SET) != 0){
                    return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
                }

                if(fwrite(writeBuffer, 1, property->entry->nameLength, file) != property->entry->nameLength){
                    return ERROR_(ERROR_WRITE_ERROR, "Failed to erase property name on disk. '%s'.", strerror(errno));                     
                }

                // Clear entry on disk.
                if(PAGE_ENTRY_SIZE > property->entry->length && PAGE_ENTRY_SIZE > property->entry->nameLength){
                    writeBuffer = alloca(PAGE_ENTRY_SIZE);
                    memset(writeBuffer, 0, PAGE_ENTRY_SIZE);
                }

                if(fseek(file, property->entry->entryOffset, SEEK_SET) != 0){
                    return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
                }

                if(fwrite(writeBuffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
                    return ERROR_(ERROR_WRITE_ERROR, "Failed to clear property entry on disk. '%s'.", strerror(errno));         
                }         

                free(property->entry->name);

                // Clear entry that is referenced in the propertyPage.
                memset(property->entry, 0, sizeof(*property->entry));

                return ERROR(ERROR_NO_ERROR);
            }            
        }
    }

    return ERROR(ERROR_NO_ERROR);
}

#undef PAGE_ENTRY_SIZE

inline ERROR_CODE propertyFile_propertySet(Property* property, const char* propertyName){
    if(PROPERTY_IS_NOT_SET(property)){
        UTIL_LOG_CONSOLE_(LOG_ERR, "ERROR: Property \"%s\" not set.", propertyName);

        __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_PROPERTY_NOT_SET);
        return ERROR(ERROR_PROPERTY_NOT_SET);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE propertyFile_createAndSetDirectoryProperty(PropertyFile* properties, Property* property, const char* propertyName, const char* value, const uint_fast64_t valueLength){
    // Note: Make sure 'slashTerminated' is clamped to '0 - 1' so we can use it later to add/subtract depending on whether the string was slash termianted or not. (jan - 2018.10.20)
    const bool slashTerminated = (value[valueLength - 1] == '/') & 0x01;

    const uint_fast64_t propertyStringLength = valueLength + !slashTerminated;

    char* propertyString;
    if(slashTerminated){
        propertyString = alloca(sizeof(*propertyString) * (valueLength + 1));
        memmove(propertyString, value, valueLength + 1);

    }else{
        propertyString = alloca(sizeof(*propertyString) * (propertyStringLength + 1));
        memcpy(propertyString, value, valueLength);
        propertyString[propertyStringLength - 1] = '/';
        propertyString[propertyStringLength] = '\0';
    } 

    return ERROR(propertyFile_createAndSetStringProperty(properties, property, propertyName, propertyString, propertyStringLength));
}

inline ERROR_CODE propertyFile_createAndSetStringProperty(PropertyFile* properties, Property* property, const char* propertyName, const char* value, const uint_fast64_t valueLength){
    ERROR_CODE error;
    if((error = propertyFile_createProperty(properties, property, propertyName, sizeof(*value) * (valueLength + 1))) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(propertyFile_setBuffer(property, (int8_t*) value) != ERROR_NO_ERROR){
        return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", propertyName);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE propertyFile_createAndSetUINT16Property(PropertyFile* properties, Property* property, const char* propertyName, const uint16_t value){
    ERROR_CODE error;
    if((error = propertyFile_createProperty(properties, property, propertyName, sizeof(value))) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    int8_t* buffer = alloca(sizeof(value));
    util_uint16ToByteArray(buffer, value);

    if(propertyFile_setBuffer(property, buffer) != ERROR_NO_ERROR){
        return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", propertyName);
    }

   return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE propertyFile_createProperty(PropertyFile* properties, Property* property, const char* propertyName, const uint_fast64_t size){
   if(PROPERTY_IS_NOT_SET(property)){
        if(propertyFile_addProperty(properties, &property, propertyName, sizeof(uint16_t)) != ERROR_NO_ERROR){
            return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", propertyName);
        }
    }else{
        if(property->entry->length != sizeof(uint16_t)){
            if(propertyFile_removeProperty(property) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", propertyName);
            }

            if(propertyFile_addProperty(properties, &property, propertyName, sizeof(uint16_t)) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", propertyName);
            }
        }
    }
    
    return ERROR(ERROR_NO_ERROR);
}

#endif