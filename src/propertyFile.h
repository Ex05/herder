#ifndef PROPERTY_FILE_H
#define PROPERTY_FILE_H

#include "util.h"

#include "arrayList.h"

#define PROPERTY_IS_NOT_SET(property) (property == NULL)
#define PROPERTY_IS_SET(property) (property != NULL)

typedef struct{
	uint_fast8_t release;
    uint_fast8_t update;
    uint_fast8_t hotfix;
}Version;

typedef struct{
	FILE* file;
	Version version;
    uint_fast8_t maxPageEntries;
    ArrayList pages;
}PropertyFile;
    
typedef struct{
	char* name;
    uint_fast64_t nameOffset;
    uint_fast64_t nameLength;
    uint_fast64_t entryOffset;
    uint_fast64_t dataOffset;
    uint_fast64_t length;
}PropertyFileEntry;

typedef struct{
    PropertyFileEntry** entries;
    uint_fast64_t offset;
}PropertyPage;

typedef struct{
    int_fast8_t* buffer;
    PropertyFileEntry* entry;
    PropertyFile* callBack;
}Property;

ERROR_CODE propertyFile_init(PropertyFile*, const char*);

ERROR_CODE propertyFile_create(const char*, const uint_fast8_t);

ERROR_CODE propertyFile_getProperty(PropertyFile*, Property**, const char*);

void propertyFile_free(PropertyFile*);

void propertyFile_freeProperty(Property*);

ERROR_CODE propertyFile_addProperty(PropertyFile*, Property**,  const char*, const uint_fast64_t);

ERROR_CODE propertyFile_setBuffer(Property*, int_fast8_t*);

ERROR_CODE propertyFile_removeProperty(Property*);

bool propertyFile_contains(PropertyFile*, const char*);

ERROR_CODE propertyFile_propertySet(Property*, const char*);

#endif

/*
Version: 3 bytes
Max_Page_Entries: 1 byte  

########################
# Page_0:              # 
#                      #
# Entry_0:{            #
# Name_Size: 8 bytes   #
# Name_Offset: 8 bytes #
# Data_Size: 8 bytes   #
# Data_Offset: 8 bytes #  
# }                    #
# Next_Page: 8 bytes   #
########################

Show_Name,
Episode_Name,
Path,
Season,
Episode,
*/