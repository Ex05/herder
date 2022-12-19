#ifndef STRING_BUILDER_C
#define STRING_BUILDER_C

#include "stringBuilder.h"
#include "util.h"

ERROR_CODE stringBuilder_append(StringBuilder* b, char* s){
	const uint_fast64_t stringLength = strlen(s);

	return stringBuilder_append_s(b, s, stringLength);
}

ERROR_CODE stringBuilder_append_s(StringBuilder* b, char* s, const uint_fast64_t length){
	if(b->string == NULL){
		b->string = malloc(sizeof(*b->string) * (length + 1));
		if(b->string == NULL){
			return ERROR(ERROR_OUT_OF_MEMORY);
		}
	}else{
		char* string = realloc(b->string, b->stringLength + length + 1);
		if(string == NULL){
			return ERROR(ERROR_OUT_OF_MEMORY);
		}

		b->string = string;
	}

	memcpy(b->string + b->stringLength, s, length);

	b->stringLength += length;

	b->string[b->stringLength] = '\0';

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE stringBuilder_appendColor(StringBuilder* b, char* s, StringBuilder_TextModifier* modifier){
	return stringBuilder_appendColor_s(b, s, strlen(s), modifier);
}

ERROR_CODE stringBuilder_appendColor_s(StringBuilder* b, char* s, uint_fast64_t stringLength, StringBuilder_TextModifier* modifier){
#ifdef TRUE_COLOR
	// Example String: /x1b[38;2;255;0;0;48;2;0;255;1;0;5m

	// If no modifiers were given, we can just call append.
	if(modifier == NULL || (modifier->foregroundColor != NULL && modifier->backgroundColor != NULL && modifier->numParameters == 0)){
		return ERROR(stringBuilder_append(b, s));
	}

	// Escape character for control functions.
	uint_fast64_t bufferLength = snprintf(NULL, 0, "%s[", STRING_BUILDER_ANSI_ESCAPE_CHARACTER);

	char* buffer = alloca(sizeof(*buffer) * (bufferLength + 1));
	bufferLength = snprintf(buffer, bufferLength + 1, "%s[", STRING_BUILDER_ANSI_ESCAPE_CHARACTER);

	stringBuilder_append_s(b, buffer, bufferLength);

	// Foreground.
	if(modifier->foregroundColor != NULL){
		bufferLength = snprintf(NULL, 0, "38;2;%d;%d;%d", modifier->foregroundColor->r, modifier->foregroundColor->g, modifier->foregroundColor->b);

		buffer = alloca(sizeof(*buffer) * (bufferLength + 1));
		bufferLength = snprintf(buffer, bufferLength + 1, "38;2;%d;%d;%d", modifier->foregroundColor->r, modifier->foregroundColor->g, modifier->foregroundColor->b);

		stringBuilder_append_s(b, buffer, bufferLength);
	}

	// Background.
	if(modifier->backgroundColor != NULL){
		bufferLength = snprintf(buffer, 0, ";48;2;%d;%d;%d", modifier->backgroundColor->r, modifier->backgroundColor->g, modifier->backgroundColor->b);

		buffer = alloca(sizeof(*buffer) * (bufferLength + 1));

		bufferLength = snprintf(buffer, bufferLength + 1, ";48;2;%d;%d;%d", modifier->backgroundColor->r, modifier->backgroundColor->g, modifier->backgroundColor->b);

		stringBuilder_append_s(b, buffer, bufferLength);
	}

	if(bufferLength < 5){
		buffer = alloca(sizeof(*buffer) * 5);
	}

	// SGR_Parameters.
	register uint_fast64_t i;
	for(i = 0; i < modifier->numParameters; i++){
		bufferLength = snprintf(buffer, 5, ";%d", modifier->SGR_Parameters[i]);

		stringBuilder_append_s(b, buffer, bufferLength);
	}

	// End controll function call.
	stringBuilder_append_s(b, "m", 1);
#endif

	// Text.
	stringBuilder_append_s(b, s, stringLength);

#ifdef TRUE_COLOR
	// Clear text modifiers.
	stringBuilder_append_s(b, STRING_BUILDER_CLEAR_PARAMETER, strlen(STRING_BUILDER_CLEAR_PARAMETER));
#endif

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE stringBuilder_append_f(StringBuilder* b, char* format, ...){
	// Calculate needed buffer length.
	va_list args;
	va_start(args, format);

	uint_fast64_t stringLength = vsnprintf(NULL, 0, format, args);

	va_end(args);

	char* buffer = alloca(sizeof(*buffer) * (stringLength + 1));

	va_start(args, format);

	vsnprintf(buffer, stringLength + 1, format, args);

	va_end(args);

	return stringBuilder_append(b, buffer);
}

ERROR_CODE stringBuilder_appendColor_f(StringBuilder* b, StringBuilder_TextModifier* modifier, char* format, ...){
	// Calculate needed buffer length.
	va_list args;
	va_start(args, format);

	uint_fast64_t stringLength = vsnprintf(NULL, 0, format, args);

	va_end(args);

	char* buffer = alloca(sizeof(*buffer) * (stringLength + 1));

	va_start(args, format);

	vsnprintf(buffer, stringLength + 1, format, args);

	va_end(args);

	return stringBuilder_appendColor(b, buffer, modifier);
}

void stringBuilder_free(StringBuilder* b){
	free(b->string);

	b->string = NULL;
	b->stringLength = 0;
}

inline char* stringBuilder_toString(StringBuilder* b){
	return b->string;
}

void stringBuilder_initTextModifier(StringBuilder_TextModifier* textModifier, StringBuilder_Color* foregroundColor, StringBuilder_Color* backgroundColor, const uint_fast64_t numParameters, ...){

	textModifier->numParameters = numParameters;
	
	textModifier->foregroundColor = foregroundColor;
	textModifier->backgroundColor = backgroundColor;

	register uint_fast64_t i;
	va_list args;
	va_start(args, numParameters);

	for(i = 0; i < numParameters; i++){
		textModifier->SGR_Parameters[i] = va_arg(args, StringBuilder_SelectGraphicRenditionParamaeters);
	}

	va_end(args);
}

#endif
