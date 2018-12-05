#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "util.h"

#include "arrayList.h"

#if !defined __GNUC__ && !defined __GNUG__
    TODO:(jan); Add preprocessor macros for other compiler.
#endif

#define ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(name) (name.value != &ARGUMENT_PARSER_ARGUMENT_PRESENT_FLAG)

#define ARGUMENT_PARSER_ADD_ARGUMENT(name, numArguments, ...) Argument argument ## name; \
argumentParser_addArgument(&parser, &argument ## name, numArguments, __VA_ARGS__)

typedef struct{
    char** arguments;
    uint_fast8_t numArguments;   
    char* value;
    uint_fast64_t valueLength;
}Argument;

typedef struct{
    ArrayList arguments;
}ArgumentParser;

ERROR_CODE argumentParser_init(ArgumentParser*);

ERROR_CODE argumentParser_addArgument(ArgumentParser*, Argument*, const uint_fast8_t, ...);

ERROR_CODE argumentParser_parse(ArgumentParser*, const int, const char**);

bool argumentParser_contains(ArgumentParser*, Argument*);

void argumentParser_free(ArgumentParser*);

#endif