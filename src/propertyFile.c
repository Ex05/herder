#ifndef PROPERTY_FILE_C
#define PROPERTY_FILE_C

#include "propertyFile.h"

#include "arrayList.c"

#define PAGE_ENTRY_SIZE (sizeof(uint64_t) * 4)

local const Version VERSION = {1, 0, 0};

local void propertyFile_initProperty(Property*, PropertyFileEntry*, PropertyFile*);

local PropertyPage* propertyFile_addPropertyPage(PropertyFile*);

local void propertyFile_initPropertyPage(PropertyPage*, const uint8_t, const uint_fast64_t);

local ARRAY_LIST_EXPAND_FUNCTION(propertyFile_expandPageList){
    return previousSize + 1;
}

ERROR_CODE propertyFile_init(PropertyFile* propertyFile, const char* fileName){
    propertyFile->file = fopen(fileName, "r+");

    if(propertyFile->file == NULL){
        return ERROR(ERROR_FAILED_TO_OPEN_FILE);
    }

    ERROR_CODE error;
    if((error = arrayList_init(&propertyFile->pages, 1, propertyFile_expandPageList) != ERROR_NO_ERROR)){
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
        propertyFile_initPropertyPage(propertyPage, propertyFile->maxPageEntries, ftell(file));

        arrayList_add(&propertyFile->pages, propertyPage);

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

                fseek(file, entry->nameOffset, SEEK_SET);

                entry->name = malloc(sizeof(*entry->name) * (entry->nameLength + 1));            
                entry->name[entry->nameLength] = '\0';

                if(fread(entry->name, sizeof(*entry->name), entry->nameLength, file) != entry->nameLength){
                    return ERROR_(ERROR_READ_ERROR, "Failed to read entry name. '%s'", fileName);
                }

                fseek(file, fileOffset, SEEK_SET);
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
        return ERROR(ERROR_WRITE_ERROR);
    }

    memset(buffer, 0, sizeof(Version));

    // Max_Page_Entries.
    buffer[0] = numPageEntries;

    if(fwrite(buffer, 1, sizeof(numPageEntries), file) != sizeof(numPageEntries)){
        return ERROR(ERROR_WRITE_ERROR);
    }

    buffer[0] = 0;

    // Empty page entries.
    uint8_t i;
    for(i = 0; i < numPageEntries; i++){
        if(fwrite(buffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
            return ERROR(ERROR_WRITE_ERROR);
        }
    }

    // Next_Page.
    if(fwrite(buffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        return ERROR(ERROR_WRITE_ERROR);
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
    ArrayListIterator it;
    arrayList_initIterator(&it, &propertyFile->pages);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = ARRAY_LIST_ITERATOR_NEXT(&it);

        uint_fast8_t i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            if(entry->name != NULL && strncmp(entry->name, name, entry->nameLength) == 0){
                Property* _property = malloc(sizeof(*_property));
                propertyFile_initProperty(_property, entry, propertyFile);

                *property = _property;

                return ERROR(ERROR_NO_ERROR);
            }
        }
    }

    *property = NULL;

    return ERROR_(ERROR_ENTRY_NOT_FOUND, "Entry: '%s'", name);
}

inline void propertyFile_initProperty(Property* property, PropertyFileEntry* entry, PropertyFile* propertyFile){
    property->entry = entry;
    property->callBack = propertyFile;

    property->buffer = malloc(sizeof(*property->buffer) * entry->length);

    fseek(propertyFile->file, entry->dataOffset, SEEK_SET);
    
    if(fread(property->buffer, 1, entry->length, propertyFile->file) != entry->length){
        UTIL_LOG_CONSOLE(LOG_ERR, "Failed to read property data.");
    }
}

inline void propertyFile_free(PropertyFile* propertyFile){
    fclose(propertyFile->file);

    ArrayListIterator it;
    arrayList_initIterator(&it, &propertyFile->pages);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = ARRAY_LIST_ITERATOR_NEXT(&it);

        uint_fast8_t i;
        for(i = 0; i < propertyFile->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            free(entry->name);
            free(entry);
        }

        free(propertyPage->entries);
        free(propertyPage);
    }

    arrayList_free(&propertyFile->pages);
}

inline bool propertyFile_contains(PropertyFile* propertyFile, const char* name){
    ArrayListIterator it;
    arrayList_initIterator(&it, &propertyFile->pages);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = ARRAY_LIST_ITERATOR_NEXT(&it);

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

    ArrayListIterator it;
    arrayList_initIterator(&it, &propertyFile->pages);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = ARRAY_LIST_ITERATOR_NEXT(&it);

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
        PropertyPage* propertyPage = propertyFile_addPropertyPage(propertyFile);

        entry = propertyPage->entries[0];
    }

label_entryFound:
    entry->length = size;
    entry->nameLength = strlen(name);
    entry->name = malloc(sizeof(*entry->name) * (entry->nameLength + 1));

    memcpy(entry->name, name, sizeof(*entry->name) * (entry->nameLength + 1));

    fseek(propertyFile->file, 0, SEEK_END);

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
    if(fseek(propertyFile->file, entry->entryOffset, SEEK_SET) == -1){
        UTIL_LOG_ERROR("");

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

    if(fseek(file, property->entry->dataOffset, SEEK_SET) == -1){
        return ERROR(ERROR_DISK_ERROR);
    } 

    if(fwrite(property->buffer, 1, bufferSize, file) != bufferSize){
        UTIL_LOG_ERROR("Failed to write property data to file.");

        return ERROR(ERROR_WRITE_ERROR);
    }

    return ERROR(ERROR_NO_ERROR);
}

PropertyPage* propertyFile_addPropertyPage(PropertyFile* propertyFile){
    FILE* file = propertyFile->file;

    const uint_fast64_t prevPageOffset = ((PropertyPage*) arrayList_get(&propertyFile->pages, propertyFile->pages.length - 1))->offset;

    PropertyPage* propertyPage = malloc(sizeof(*propertyPage));
    propertyFile_initPropertyPage(propertyPage, propertyFile->maxPageEntries, ftell(file));

    int8_t* writeBuffer = alloca(PAGE_ENTRY_SIZE);
    memset(writeBuffer, 0 , PAGE_ENTRY_SIZE);

    uint_fast8_t i;
    for(i = 0; i < propertyFile->maxPageEntries; i++){
        propertyPage->entries[i]->entryOffset = ftell(file);

        if(fwrite(writeBuffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
            UTIL_LOG_ERROR("Failed to write empty property entry to file.");
        }
    }

    if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        UTIL_LOG_ERROR("Failed to write propertyPage offset to file.");
    }

    arrayList_add(&propertyFile->pages, propertyPage);

    fseek(file, prevPageOffset + (PAGE_ENTRY_SIZE * propertyFile->maxPageEntries), SEEK_SET);

    util_uint64ToByteArray(writeBuffer, propertyPage->offset);

    if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
        UTIL_LOG_ERROR("Failed to update previous propertyPage offset.");
    }

    return propertyPage;
}

inline void propertyFile_initPropertyPage(PropertyPage* propertyPage, const uint8_t maxPageEntries, const uint_fast64_t pageOffset){
    propertyPage->entries = malloc(sizeof(*propertyPage->entries) * maxPageEntries);
    propertyPage->offset = pageOffset;

    uint_fast8_t i;
    for(i = 0; i < maxPageEntries; i++){
        propertyPage->entries[i] = malloc(sizeof(PropertyFileEntry));
    }
}

ERROR_CODE propertyFile_removeProperty(Property* property){
    FILE* file = property->callBack->file;

    ArrayListIterator it;
    arrayList_initIterator(&it, &property->callBack->pages);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        PropertyPage* propertyPage = ARRAY_LIST_ITERATOR_NEXT(&it);

        int i;
        for(i = 0; i < property->callBack->maxPageEntries; i++){
            PropertyFileEntry* entry = propertyPage->entries[i];

            if(entry->name != NULL && strncmp(entry->name, property->entry->name, property->entry->nameLength) == 0){
                int8_t* writeBuffer = alloca(property->entry->length);
                memset(writeBuffer, 0, property->entry->length);

                // Clear data on disk.
                if(fseek(file, property->entry->dataOffset, SEEK_SET) != 0){
                    UTIL_LOG_ERROR("Failed to move file pointer to property data on disk.");    

                    return ERROR(ERROR_DISK_ERROR); 
                }

                if(fwrite(writeBuffer, 1, property->entry->length, file) != property->entry->length){
                    UTIL_LOG_ERROR("Failed to erase property data on disk.");    

                    return ERROR(ERROR_WRITE_ERROR);          
                }

                // Clear entry name on disk.
                if(property->entry->nameLength > property->entry->length){
                    writeBuffer = alloca(property->entry->nameLength);
                    memset(writeBuffer, 0, property->entry->nameLength);
                }

                fseek(file, property->entry->nameOffset, SEEK_SET);

                if(fwrite(writeBuffer, 1, property->entry->nameLength, file) != property->entry->nameLength){
                    UTIL_LOG_ERROR("Failed to erase property name on disk.");  

                    return ERROR(ERROR_WRITE_ERROR);                       
                }

                // Clear entry on disk.
                if(PAGE_ENTRY_SIZE > property->entry->length && PAGE_ENTRY_SIZE > property->entry->nameLength){
                    writeBuffer = alloca(PAGE_ENTRY_SIZE);
                    memset(writeBuffer, 0, PAGE_ENTRY_SIZE);
                }

                fseek(file, property->entry->entryOffset, SEEK_SET);

                if(fwrite(writeBuffer, 1, PAGE_ENTRY_SIZE, file) != PAGE_ENTRY_SIZE){
                    UTIL_LOG_ERROR("Failed to clear property entry on disk.");      

                    return ERROR(ERROR_WRITE_ERROR);          
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

#endif