#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "util.h"
#include "linkedList.h"

#if !defined __GNUC__ && !defined __GNUG__
	TODO:(jan); Add preprocessor macros for other compiler.
#endif

#define ARGUMENT_PARSER_ADD_ARGUMENT(name, numArguments, ...) Argument argument ## name; \
argumentParser_addArgument(&parser, &argument ## name, numArguments, __VA_ARGS__)

typedef struct{
	char** arguments;
	uint_fast8_t numArguments;
	const char** values;
	uint_fast64_t* valueLengths;
	uint_fast64_t numValues;
}Argument;

typedef struct{
	LinkedList arguments;
}ArgumentParser;

ERROR_CODE argumentParser_init(ArgumentParser*);

ERROR_CODE argumentParser_addArgument(ArgumentParser*, Argument*, const int, ...);

ERROR_CODE argumentParser_parse(ArgumentParser*, const int, const char**);

bool argumentParser_contains(ArgumentParser*, Argument*);

void argumentParser_free(ArgumentParser*);

#endif