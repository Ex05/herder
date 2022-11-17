#ifndef ARGUMENT_PARSER_TEST_C
#define ARGUMENT_PARSER_TEST_C

#include "../test.c"

TEST_TEST_FUNCTION(argumentParser_parse){
	ERROR_CODE error;

	ArgumentParser parser;
	argumentParser_init(&parser);

	#if !defined __GNUC__ && !defined __GNUG__
		// TODO:(jan); Add preprocessor macros for other compiler.
	#endif

	ARGUMENT_PARSER_ADD_ARGUMENT(Test_0, 2, "-t", "--test");
	ARGUMENT_PARSER_ADD_ARGUMENT(Import_0, 1, "-i");
	ARGUMENT_PARSER_ADD_ARGUMENT(Rename_1, 1, "--rename");
	
	const char* argc_0[] = {"./test", "--rename", "/home/ex05", "/home/29a", "-t", "-t", "-v", "--add", "American Dad"};
	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_DUPLICATE_ENTRY);
	if((error = argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_0), argc_0)) != ERROR_DUPLICATE_ENTRY){
		return TEST_FAILURE("'arguemntParser_parse' did not fail with '%s'. '%s'.", "ERROR_DUPLICATE_ENTRY", util_toErrorString(error));
	}

	argumentParser_free(&parser);

	argumentParser_init(&parser);

	ARGUMENT_PARSER_ADD_ARGUMENT(Test_1, 2, "-t", "--test");
	ARGUMENT_PARSER_ADD_ARGUMENT(Import_1, 1, "-i");

	const char* argc_1[] = {"./test", "-i"};
	if(argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_1), argc_1) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to parse arguments. '%s'.", util_toErrorString(error));
	}

	if(argumentImport_1.numValues != 0){
		return TEST_FAILURE("Failed to parse argument: '%s' '%s'.", argumentImport_1.arguments[0], util_toErrorString(error));
	}

	argumentParser_free(&parser);

	argumentParser_init(&parser);

	ARGUMENT_PARSER_ADD_ARGUMENT(Test_2, 2, "-t", "--test");
	ARGUMENT_PARSER_ADD_ARGUMENT(Import_2, 1, "-i");
	ARGUMENT_PARSER_ADD_ARGUMENT(Rename_0, 1, "--rename");

	const char* argc_2[] = {"./test", "-v", "-i", "test", "12", "--test", "abbab", "---Tester", "12", "--rename", "/home/123.mpv", "/home/456.lmpv"};
	if(argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_2), argc_2) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to parse arguments. '%s'.", util_toErrorString(error));
	}

	if(!argumentParser_contains(&parser, &argumentTest_2)){
	 return TEST_FAILURE("Failed to parse argument: '%s'.", argumentTest_2.arguments[0]);
	}else{
		if(argumentTest_2.numValues != 1){
			return TEST_FAILURE("Failed to parse argument: '%s'.", argumentTest_2.arguments[0]);
		}
	}

	if(!argumentParser_contains(&parser, &argumentImport_2)){
		return TEST_FAILURE("Failed to parse argument: '%s'.", argumentImport_2.arguments[0]);
	}else{
		if(argumentImport_2.numValues != 2){
			return TEST_FAILURE("Failed to parse argument: '%s'.", argumentImport_2.arguments[0]);
		}
	}

	if(!argumentParser_contains(&parser, &argumentRename_0)){
		return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
	}else{
		if(argumentRename_0.numValues != 2){
			return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
		}else{
			if(argumentRename_0.numValues != 2){
				return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
			}

			if(strncmp(argumentRename_0.values[0], "/home/123.mpv", strlen("/home/123.mpv") + 1) != 0){
				return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
			}

			if(strncmp(argumentRename_0.values[1], "/home/456.lmpv", strlen("/home/456.lmpv") + 1) != 0){
				return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
			}
		}
	}

	argumentParser_free(&parser);

	return TEST_SUCCESS;
}

#endif