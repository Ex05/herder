#ifndef ARRAY_LIST_TEST_C
#define ARRAY_LIST_TEST_C

#include "../test.c"

TEST_TEST_FUNCTION(arraylist_iteration){
	ArrayList list;
	arrayList_initFixedSizeList(&list, 12, sizeof(uint8_t));

	ARRAY_LIST_ADD(&list, 0, uint8_t);
	ARRAY_LIST_ADD(&list, 1, uint8_t);
	ARRAY_LIST_ADD(&list, 2, uint8_t);
	ARRAY_LIST_ADD(&list, 3, uint8_t);

	uint8_t buf[4] = {0};

	ArrayListIterator it;
	arrayList_initIterator(&it, &list);

	uint_fast64_t i = 0;
	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
		buf[i++] = *ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
	}

	if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
		return TEST_FAILURE("%s", "Failed to iterate over array list.");
	}

	arrayList_free(&list);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_fixedSizedStackList){
	ArrayList list;
	ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(uint8_t));

	ARRAY_LIST_ADD(&list, 0, uint8_t);
	ARRAY_LIST_ADD(&list, 1, uint8_t);
	ARRAY_LIST_ADD(&list, 2, uint8_t);
	ARRAY_LIST_ADD(&list, 3, uint8_t);

	uint8_t buf[4] = {0};

	ArrayListIterator it;
	arrayList_initIterator(&it, &list);

	uint_fast64_t i = 0;
	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
		buf[i++] = *ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
	}

	if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
		return TEST_FAILURE("%s", "Failed to create fix sized stack list.");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_get){
	// Test_0.
	{
		ArrayList list;
		ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(uint8_t));

		ARRAY_LIST_ADD(&list, 0, uint8_t);
		ARRAY_LIST_ADD(&list, 1, uint8_t);
		ARRAY_LIST_ADD(&list, 2, uint8_t);
		ARRAY_LIST_ADD(&list, 3, uint8_t);

		uint8_t ret;
		if((ret = ARRAY_LIST_GET(&list, 1, uint8_t)) != 1){
			return TEST_FAILURE("Return value '%" PRIu8 " != '%" PRIu8 "'.", ret, 1);
		}
	}

	// Test_1.
	{
		struct testStruct{
			uint8_t a;
			char b;
		};

		ArrayList list;
		ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(struct testStruct));

		struct testStruct b2 = {1, 'c'};
		struct testStruct b3 = {2, 'd'};
		struct testStruct b4 = {3, 'g'};

		ARRAY_LIST_ADD(&list, b2, struct testStruct);
		ARRAY_LIST_ADD(&list, b3, struct testStruct);
		ARRAY_LIST_ADD(&list, b4, struct testStruct);

		struct testStruct* ret;
		ret = ARRAY_LIST_GET_PTR(&list, 0, struct testStruct);

		if(ret->a != 1 && ret->b != 'c'){
			return TEST_FAILURE("%s", "Failed to get arrayList value.");
		}

		ret = ARRAY_LIST_GET_PTR(&list, 2, struct testStruct);

		if(ret->a != 3 && ret->b != 'g'){
			return TEST_FAILURE("%s", "Failed to get arrayList value.");
		}
	}

	// Test_2.
	{
		struct testStruct{
			uint8_t a;
			char b;
		};

		ArrayList list;
		ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(struct testStruct));

		struct testStruct b2 = {1, 'c'};
		struct testStruct* b3 = malloc(sizeof(struct testStruct));
		b3->a = 7;
		b3->b = 'h';

		ARRAY_LIST_ADD(&list, b2, struct testStruct);
		ARRAY_LIST_ADD_PTR(&list, b3, struct testStruct);
		
		free(b3);

		struct testStruct* ret;
		ret = ARRAY_LIST_GET_PTR(&list, 0, struct testStruct);

		if(ret->a != 1 && ret->b != 'c'){
			return TEST_FAILURE("%s", "Failed to get arrayList value.");
		}

		ret = ARRAY_LIST_GET_PTR(&list, 1, struct testStruct);

		if(ret->a != 7 && ret->b != 'h'){
			return TEST_FAILURE("%s", "Failed to get arrayList value.");
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_expand){
	ArrayList list;
	arrayList_init(&list, 1, sizeof(uint8_t), arrayList_defaultExpandFunction);

	ARRAY_LIST_ADD(&list, 0, uint8_t);
	ARRAY_LIST_ADD(&list, 1, uint8_t);
	ARRAY_LIST_ADD(&list, 2, uint8_t);
	ARRAY_LIST_ADD(&list, 3, uint8_t);

	uint8_t buf[4] = {0};

	ArrayListIterator it;
	arrayList_initIterator(&it, &list);

	uint_fast64_t i = 0;
	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
		buf[i++] = *ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
	}

	if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
		return TEST_FAILURE("%s", "Failed to iterate over array list.");
	}

	arrayList_free(&list);

	return TEST_SUCCESS;
}

#endif