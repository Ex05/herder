#ifndef STRING_BUILDER_TEST_C
#define STRING_BUILDER_TEST_C

#include "../test.c"

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(stringBuilder, b){
	*b = calloc(1, sizeof(StringBuilder));
		
	if(*b == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(stringBuilder, StringBuilder, b){
	stringBuilder_free(b);

	free(b);

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(stringBuilder_append, StringBuilder, b){
	#define STRING_BUILDER_STRING_FRAGMENT_0 "0123"
	#define STRING_BUILDER_STRING_FRAGMENT_1 "4567"
	#define STRING_BUILDER_STRING_FRAGMENT_2 "8910"

	#define STRING_BUILDER_TEST_STRING STRING_BUILDER_STRING_FRAGMENT_0 STRING_BUILDER_STRING_FRAGMENT_1 STRING_BUILDER_STRING_FRAGMENT_2

	ERROR_CODE error;
	if((error = stringBuilder_append(b, STRING_BUILDER_STRING_FRAGMENT_0)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to append to string builder. (%s)", util_toErrorString(error));
	}

	if((error = stringBuilder_append(b, STRING_BUILDER_STRING_FRAGMENT_1)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to append to string builder. (%s)", util_toErrorString(error));
	}

	if((error = stringBuilder_append(b, STRING_BUILDER_STRING_FRAGMENT_2)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to append to string builder. (%s)", util_toErrorString(error));
	}

	char* string = stringBuilder_toString(b);

	if(strcmp(string, STRING_BUILDER_TEST_STRING) != 0){
		return TEST_FAILURE("Failed to append to string builder '%s' != '%s'.", string, STRING_BUILDER_TEST_STRING);
	}

	#undef STRING_BUILDER_STRING_FRAGMENT_0
	#undef STRING_BUILDER_STRING_FRAGMENT_1
	#undef STRING_BUILDER_STRING_FRAGMENT_2

	#undef STRING_BUILDER_TEST_STRING

	return TEST_SUCCESS;
}

#endif