#ifndef ARGUMENT_PARSER_C
#define ARGUMENT_PARSER_C
    
#include "argumentParser.h"

#include "arrayList.c"

// Note:(Jan) If the value of an argument points to this flag, then the Argument was present, but had no value assosiatet with it. e.g: ('--help', '-?') as opposed to ("--setImportDirectory '/herder/import'")
char ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG = {0};

local ERROR_CODE argumentParser_initArgument(Argument*, const uint_fast8_t);

local ARRAY_LIST_EXPAND_FUNCTION(argumentParser_expandArgumentList){
    return 2 * previousSize;
}

inline ERROR_CODE argumentParser_init(ArgumentParser* parser){
    int_fast32_t error;

    if((error = arrayList_init(&parser->arguments, 2, argumentParser_expandArgumentList)) != 0){
        UTIL_LOG_ERROR_("Failed to initialise argument list [%" PRIdFAST32 "]", error);

        return ERROR(error);
    }

    return ERROR(error);
}

inline ERROR_CODE argumentParser_initArgument(Argument* argument, const uint_fast8_t numArguments){
    argument->numArguments = numArguments;
    argument->value = NULL;
    argument->valueLength = 0;

    argument->arguments = malloc(sizeof(*argument->arguments) * numArguments);

    if(argument->arguments == NULL){
       return ERROR(ERROR_OUT_OF_MEMORY);
    }

    return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE argumentParser_addArgument(ArgumentParser* parser, Argument* argument, const uint_fast8_t numArguments, ...){
    argumentParser_initArgument(argument, numArguments);

    va_list args;
    va_start(args, numArguments);

    register uint_fast8_t i;
    for(i = 0; i < numArguments; i++){
        const char* s = va_arg(args, char*);
        const uint_fast64_t strLength = strlen(s);

        argument->arguments[i] = malloc(sizeof(*argument->arguments[i]) * (strLength + 1));
                
        if(argument->arguments[i] == NULL){
            va_end(args);

            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        strncpy(argument->arguments[i], s, strLength + 1);
    }

    arrayList_add(&parser->arguments, argument);

    va_end(args);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE argumentParser_parse(ArgumentParser* parser, const int numArguments, const char** arguments){
    ArrayListIterator it;
    register int i;
    for(i = 0; i < numArguments; i++){
        arrayList_initIterator(&it, &parser->arguments);

        while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
            Argument* argument = ARRAY_LIST_ITERATOR_NEXT(&it);

            register uint_fast8_t j;
            for(j = 0; j < argument->numArguments; j++){
                if(strcmp(arguments[i], argument->arguments[j]) == 0){
                    if(i + 1 < numArguments){
                        const char* const value = arguments[i + 1];

                        argument->valueLength = strlen(value);

                        argument->value = malloc(sizeof(*argument->value) * (argument->valueLength + 1));
                        if(argument->value == NULL){
                            return ERROR(ERROR_OUT_OF_MEMORY);
                        }

                        memcpy(argument->value, value, argument->valueLength + 1);
                    }else{
                        argument->value = &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG;
                    }
                }                
            }              
        }
    }

    return ERROR(ERROR_NO_ERROR);
}

inline bool argumentParser_contains(ArgumentParser* parser, Argument* argument){
    ArrayListIterator it;
    arrayList_initIterator(&it, &parser->arguments);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        const Argument* _argument = ARRAY_LIST_ITERATOR_NEXT(&it);

        if(argument == _argument && (_argument->value == &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG || _argument->value != NULL)){
            return true;
        }
    }

    return false;
}

void argumentParser_free(ArgumentParser* parser){
    ArrayListIterator it;
    arrayList_initIterator(&it, &parser->arguments);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        Argument* argument = ARRAY_LIST_ITERATOR_NEXT(&it);

        register uint_fast8_t i;
        for(i = 0; i < argument->numArguments; i++){
            free(argument->arguments[i]);
        }

        if(argument->value != &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG){
            free(argument->value);
        }

        free(argument->arguments);
    }

    arrayList_free(&parser->arguments);
}

#endif