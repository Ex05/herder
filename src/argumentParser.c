#ifndef ARGUMENT_PARSER_C
#define ARGUMENT_PARSER_C
    
#include "argumentParser.h"

#include "linkedList.c"

// Note:(jan) If the value of an argument points to this flag, then the Argument was present, but had no value assosiatet with it. e.g: ('--help', '-?') as opposed to ("--setImportDirectory '/herder/import'")
char ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG = {0};

local ERROR_CODE argumentParser_initArgument(Argument*, const uint_fast8_t);

local bool argumentParser_isArgument(const char*);

inline ERROR_CODE argumentParser_init(ArgumentParser* parser){
    int_fast32_t error;

    if((error = linkedList_init(&parser->arguments)) != 0){
        UTIL_LOG_ERROR_("Failed to initialise argument list [%" PRIdFAST32 "]", error);

        return ERROR(error);
    }

    return ERROR(error);
}

inline ERROR_CODE argumentParser_initArgument(Argument* argument, const uint_fast8_t numArguments){
    memset(argument, 0, sizeof(*argument));

    argument->numArguments = numArguments;
    
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

    linkedList_add(&parser->arguments, argument);

    va_end(args);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE argumentParser_parse(ArgumentParser* parser, const int numArguments, const char** arguments){
    if(numArguments <= 1){
        return ERROR(ERROR_NO_VALID_ARGUMENT);
    }

    LinkedListIterator it;
    register int i;
    for(i = 0; i < numArguments; i++){        
        linkedList_initIterator(&it, &parser->arguments);
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            Argument* argument = LINKED_LIST_ITERATOR_NEXT(&it);

            register uint_fast8_t j;
            for(j = 0; j < argument->numArguments; j++){
                if(strcmp(arguments[i], argument->arguments[j]) == 0){
                    if(argument->numValues != 0){
                        return ERROR(ERROR_DUPLICATE_ENTRY);
                    }

                    void* values = realloc(argument->values, sizeof(*argument->values) * (argument->numValues + 1));

                    if(values == NULL){
                        return ERROR(ERROR_OUT_OF_MEMORY);
                    }

                    argument->values = values;

                    void* valueLengths = realloc(argument->valueLengths, sizeof(*argument->valueLengths) * (argument->numValues + 1));

                    if(valueLengths == NULL){
                        return ERROR(ERROR_OUT_OF_MEMORY);
                    }

                    argument->valueLengths = valueLengths;

                    argument->numValues += 1;

                    // No more arguments.
                    if(i + 1 >= numArguments){
                        argument->values[argument->numValues - 1] = &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG;
                        argument->valueLengths[argument->numValues - 1] = 0;

                    }else{
                        // As long as we have arguments.
                        while(i + 1 < numArguments){
                            // If the next argument is actually not an argument.
                            if(!argumentParser_isArgument(arguments[i + 1])){
                                argument->values[argument->numValues - 1] = arguments[i + 1];
                                argument->valueLengths[argument->numValues - 1] = strlen(arguments[i + 1]);

                                i += 1;

                                // Continue if we have more than one value to an argument, e.g. '--rename "/home/29a" "/home/ex05"'
                                if(i + 1 < numArguments && !argumentParser_isArgument(arguments[i + 1])){
                                    void* values = realloc(argument->values, sizeof(*argument->values) * (argument->numValues + 1));

                                    if(values == NULL){
                                        return ERROR(ERROR_OUT_OF_MEMORY);
                                    }

                                    argument->values = values;

                                    void* valueLengths = realloc(argument->valueLengths, sizeof(*argument->valueLengths) * (argument->numValues + 1));

                                    if(valueLengths == NULL){
                                        return ERROR(ERROR_OUT_OF_MEMORY);
                                    }

                                    argument->valueLengths = valueLengths;

                                    argument->numValues += 1;
                                }else{
                                    break;
                                }
                            }else{
                                argument->values[argument->numValues - 1] = &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG;
                                argument->valueLengths[argument->numValues - 1] = 0;

                                break;
                            }
                        }
                    }
            
                }
            }
        }
    }

    return ERROR(ERROR_NO_ERROR);
}

inline bool argumentParser_contains(ArgumentParser* parser, Argument* argument){
    LinkedListIterator it;
    linkedList_initIterator(&it, &parser->arguments);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        const Argument* _argument = LINKED_LIST_ITERATOR_NEXT(&it);

    if(argument == _argument && (_argument->values != NULL || _argument->values[0] == &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG)){
            return true;
        }
    }

    return false;
}

inline bool argumentParser_isArgument(const char* s){
    return s[0] == '-' || (s[0] == '-' && s[1] == '-');
}

void argumentParser_free(ArgumentParser* parser){
    LinkedListIterator it;
    linkedList_initIterator(&it, &parser->arguments);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Argument* argument = LINKED_LIST_ITERATOR_NEXT(&it);

        register uint_fast8_t i;
        for(i = 0; i < argument->numArguments; i++){
            free(argument->arguments[i]);
        }

        free(argument->arguments);
        free(argument->values);
        free(argument->valueLengths);
    }

    linkedList_free(&parser->arguments);
}

#endif