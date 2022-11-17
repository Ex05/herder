#ifndef QUE_TEST_C
#define QUE_TEST_C

#include "../test.c"

TEST_TEST_FUNCTION(que_deAndEnque){
	Que que = {0};

	int a = 0;
	int b = 1;
	int c = 2;

	que_enque(&que, &a);
	que_enque(&que, &b);
	que_enque(&que, &c);

	int value;
	if((value = *((int*) que_deque(&que))) != 0){
		return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 0);
	}

	if((value = *((int*) que_deque(&que))) != 1){
		return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 1);
	}

	if((value = *((int*) que_deque(&que))) != 2){
		return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 2);
	}

	que_clear(&que);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(que_clear){
	Que que = {0};

	int a = 0;
	int b = 1;
	int c = 2;

	que_enque(&que, &a);
	que_enque(&que, &b);
	que_enque(&que, &c);

	que_clear(&que);

	if(que_deque(&que) != NULL){
		return TEST_FAILURE("Failed to clear que, 'que_deque' did not return: '%s'", "NULL");
	}

	return TEST_SUCCESS;
}

#endif