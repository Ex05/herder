#ifndef STRING_BUILDER_C
#define STRING_BUILDER_C

#include "stringBuilder.h"

inline ERROR_CODE stringBuilder_append(StringBuilder* b, char* s){
	const uint_fast64_t stringLength = strlen(s);

	return stringBuilder_append_s(b, s, stringLength);
}

inline ERROR_CODE stringBuilder_append_s(StringBuilder* b, char* s, const uint_fast64_t length){
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

inline ERROR_CODE stringBuilder_appendColor(StringBuilder* b, StringBuilder_TextModifier* modifier, char* s){
	return stringBuilder_appendColor_s(b, modifier, s, strlen(s));
}

inline ERROR_CODE stringBuilder_appendColor_s(StringBuilder* b, StringBuilder_TextModifier* modifier, char* s, uint_fast64_t stringLength){
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

inline ERROR_CODE stringBuilder_append_f(StringBuilder* b, char* format, ...){
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

inline ERROR_CODE stringBuilder_appendColor_f(StringBuilder* b, StringBuilder_TextModifier* modifier, char* format, ...){
	// Calculate needed buffer length.
	va_list args;
	va_start(args, format);

	uint_fast64_t stringLength = vsnprintf(NULL, 0, format, args);

	va_end(args);

	char* buffer = alloca(sizeof(*buffer) * (stringLength + 1));

	va_start(args, format);

	vsnprintf(buffer, stringLength + 1, format, args);

	va_end(args);

	return stringBuilder_appendColor(b, modifier, buffer);
}

inline void stringBuilder_free(StringBuilder* b){
	free(b->string);

	b->string = NULL;
	b->stringLength = 0;
}

inline char* stringBuilder_toString(StringBuilder* b){
	return b->string;
}

inline void stringBuilder_initTextModifier(StringBuilder_TextModifier* textModifier, StringBuilder_Color* foregroundColor, StringBuilder_Color* backgroundColor, const uint_fast64_t numParameters, ...){

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

inline void stringBuilder_printColorChart(void){
	StringBuilder b = {0};

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MAROON);
	stringBuilder_appendColor(&b, MAROON, "MAROON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_RED);
	stringBuilder_appendColor(&b, DARK_RED, "DARK_RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BROWN);
	stringBuilder_appendColor(&b, BROWN, "BROWN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(FIREBRICK);
	stringBuilder_appendColor(&b, FIREBRICK, "FIREBRICK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CRIMSON);
	stringBuilder_appendColor(&b, CRIMSON, "CRIMSON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(RED);
	stringBuilder_appendColor(&b, RED, "RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(TOMATO);
	stringBuilder_appendColor(&b, TOMATO, "TOMATO\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CORAL);
	stringBuilder_appendColor(&b, CORAL, "CORAL\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(INDIAN_RED);
	stringBuilder_appendColor(&b, INDIAN_RED, "INDIAN_RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_CORAL);
	stringBuilder_appendColor(&b, LIGHT_CORAL, "LIGHT_CORAL\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_SALMON);
	stringBuilder_appendColor(&b, DARK_SALMON, "DARK_SALMON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SALMON);
	stringBuilder_appendColor(&b, SALMON, "SALMON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_SALMON);
	stringBuilder_appendColor(&b, LIGHT_SALMON, "LIGHT_SALMON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ORANGE_RED);
	stringBuilder_appendColor(&b, ORANGE_RED, "ORANGE_RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_ORANGE);
	stringBuilder_appendColor(&b, DARK_ORANGE, "DARK_ORANGE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ORANGE);
	stringBuilder_appendColor(&b, ORANGE, "ORANGE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GOLD);
	stringBuilder_appendColor(&b, GOLD, "GOLD\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_GOLDEN_ROD);
	stringBuilder_appendColor(&b, DARK_GOLDEN_ROD, "DARK_GOLDEN_ROD\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GOLDEN_ROD);
	stringBuilder_appendColor(&b, GOLDEN_ROD, "GOLDEN_ROD\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PALE_GOLDEN_ROD);
	stringBuilder_appendColor(&b, PALE_GOLDEN_ROD, "PALE_GOLDEN_ROD\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_KHAKI);
	stringBuilder_appendColor(&b, DARK_KHAKI, "DARK_KHAKI\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(KHAKI);
	stringBuilder_appendColor(&b, KHAKI, "KHAKI\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(OLIVE);
	stringBuilder_appendColor(&b, OLIVE, "OLIVE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(YELLOW);
	stringBuilder_appendColor(&b, YELLOW, "YELLOW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(YELLOW_GREEN);
	stringBuilder_appendColor(&b, YELLOW_GREEN, "YELLOW_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_OLIVE_GREEN);
	stringBuilder_appendColor(&b, DARK_OLIVE_GREEN, "DARK_OLIVE_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(OLIVE_DRAB);
	stringBuilder_appendColor(&b, OLIVE_DRAB, "OLIVE_DRAB\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LAWN_GREEN);
	stringBuilder_appendColor(&b, LAWN_GREEN, "LAWN_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CHARTREUSE);
	stringBuilder_appendColor(&b, CHARTREUSE, "CHARTREUSE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GREEN_YELLOW);
	stringBuilder_appendColor(&b, GREEN_YELLOW, "GREEN_YELLOW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_GREEN);
	stringBuilder_appendColor(&b, DARK_GREEN, "DARK_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GREEN);
	stringBuilder_appendColor(&b, GREEN, "GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(FOREST_GREEN);
	stringBuilder_appendColor(&b, FOREST_GREEN, "FOREST_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIME);
	stringBuilder_appendColor(&b, LIME, "LIME\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIME_GREEN);
	stringBuilder_appendColor(&b, LIME_GREEN, "LIME_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_GREEN);
	stringBuilder_appendColor(&b, LIGHT_GREEN, "LIGHT_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PALE_GREEN);
	stringBuilder_appendColor(&b, PALE_GREEN, "PALE_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_SEA_GREEN);
	stringBuilder_appendColor(&b, DARK_SEA_GREEN, "DARK_SEA_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_SPRING_GREEN);
	stringBuilder_appendColor(&b, MEDIUM_SPRING_GREEN, "MEDIUM_SPRING_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SPRING_GREEN);
	stringBuilder_appendColor(&b, SPRING_GREEN, "SPRING_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SEA_GREEN);
	stringBuilder_appendColor(&b, SEA_GREEN, "SEA_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_AQUA_MARINE);
	stringBuilder_appendColor(&b, MEDIUM_AQUA_MARINE, "MEDIUM_AQUA_MARINE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_SEA_GREEN);
	stringBuilder_appendColor(&b, MEDIUM_SEA_GREEN, "MEDIUM_SEA_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_SEA_GREEN);
	stringBuilder_appendColor(&b, LIGHT_SEA_GREEN, "LIGHT_SEA_GREEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_SLATE_GRAY);
	stringBuilder_appendColor(&b, DARK_SLATE_GRAY, "DARK_SLATE_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(TEAL);
	stringBuilder_appendColor(&b, TEAL, "TEAL\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_CYAN);
	stringBuilder_appendColor(&b, DARK_CYAN, "DARK_CYAN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(AQUA);
	stringBuilder_appendColor(&b, AQUA, "AQUA\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CYAN);
	stringBuilder_appendColor(&b, CYAN, "CYAN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_CYAN);
	stringBuilder_appendColor(&b, LIGHT_CYAN, "LIGHT_CYAN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_TURQUOISE);
	stringBuilder_appendColor(&b, DARK_TURQUOISE, "DARK_TURQUOISE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(TURQUOISE);
	stringBuilder_appendColor(&b, TURQUOISE, "TURQUOISE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_TURQUOISE);
	stringBuilder_appendColor(&b, MEDIUM_TURQUOISE, "MEDIUM_TURQUOISE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PALE_TURQUOISE);
	stringBuilder_appendColor(&b, PALE_TURQUOISE, "PALE_TURQUOISE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(AQUA_MARINE);
	stringBuilder_appendColor(&b, AQUA_MARINE, "AQUA_MARINE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(POWDER_BLUE);
	stringBuilder_appendColor(&b, POWDER_BLUE, "POWDER_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CADET_BLUE);
	stringBuilder_appendColor(&b, CADET_BLUE, "CADET_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(STEEL_BLUE);
	stringBuilder_appendColor(&b, STEEL_BLUE, "STEEL_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CORN_FLOWER_BLUE);
	stringBuilder_appendColor(&b, CORN_FLOWER_BLUE, "CORN_FLOWER_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DEEP_SKY_BLUE);
	stringBuilder_appendColor(&b, DEEP_SKY_BLUE, "DEEP_SKY_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DODGER_BLUE);
	stringBuilder_appendColor(&b, DODGER_BLUE, "DODGER_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_BLUE);
	stringBuilder_appendColor(&b, LIGHT_BLUE, "LIGHT_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SKY_BLUE);
	stringBuilder_appendColor(&b, SKY_BLUE, "SKY_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_SKY_BLUE);
	stringBuilder_appendColor(&b, LIGHT_SKY_BLUE, "LIGHT_SKY_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MIDNIGHT_BLUE);
	stringBuilder_appendColor(&b, MIDNIGHT_BLUE, "MIDNIGHT_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(NAVY);
	stringBuilder_appendColor(&b, NAVY, "NAVY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_BLUE);
	stringBuilder_appendColor(&b, DARK_BLUE, "DARK_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_BLUE);
	stringBuilder_appendColor(&b, MEDIUM_BLUE, "MEDIUM_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BLUE);
	stringBuilder_appendColor(&b, BLUE, "BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ROYAL_BLUE);
	stringBuilder_appendColor(&b, ROYAL_BLUE, "ROYAL_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BLUE_VIOLET);
	stringBuilder_appendColor(&b, BLUE_VIOLET, "BLUE_VIOLET\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(INDIGO);
	stringBuilder_appendColor(&b, INDIGO, "INDIGO\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_SLATE_BLUE);
	stringBuilder_appendColor(&b, DARK_SLATE_BLUE, "DARK_SLATE_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SLATE_BLUE);
	stringBuilder_appendColor(&b, SLATE_BLUE, "SLATE_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_SLATE_BLUE);
	stringBuilder_appendColor(&b, MEDIUM_SLATE_BLUE, "MEDIUM_SLATE_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_PURPLE);
	stringBuilder_appendColor(&b, MEDIUM_PURPLE, "MEDIUM_PURPLE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_MAGENTA);
	stringBuilder_appendColor(&b, DARK_MAGENTA, "DARK_MAGENTA\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_VIOLET);
	stringBuilder_appendColor(&b, DARK_VIOLET, "DARK_VIOLET\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_ORCHID);
	stringBuilder_appendColor(&b, DARK_ORCHID, "DARK_ORCHID\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_ORCHID);
	stringBuilder_appendColor(&b, MEDIUM_ORCHID, "MEDIUM_ORCHID\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PURPLE);
	stringBuilder_appendColor(&b, PURPLE, "PURPLE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(THISTLE);
	stringBuilder_appendColor(&b, THISTLE, "THISTLE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PLUM);
	stringBuilder_appendColor(&b, PLUM, "PLUM\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(VIOLET);
	stringBuilder_appendColor(&b, VIOLET, "VIOLET\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MAGENTA);
	stringBuilder_appendColor(&b, MAGENTA, "MAGENTA\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(FUCHSIA);
	stringBuilder_appendColor(&b, FUCHSIA, "FUCHSIA\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ORCHID);
	stringBuilder_appendColor(&b, ORCHID, "ORCHID\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MEDIUM_VIOLET_RED);
	stringBuilder_appendColor(&b, MEDIUM_VIOLET_RED, "MEDIUM_VIOLET_RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PALE_VIOLET_RED);
	stringBuilder_appendColor(&b, PALE_VIOLET_RED, "PALE_VIOLET_RED\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DEEP_PINK);
	stringBuilder_appendColor(&b, DEEP_PINK, "DEEP_PINK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(HOT_PINK);
	stringBuilder_appendColor(&b, HOT_PINK, "HOT_PINK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_PINK);
	stringBuilder_appendColor(&b, LIGHT_PINK, "LIGHT_PINK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PINK);
	stringBuilder_appendColor(&b, PINK, "PINK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ANTIQUE_WHITE);
	stringBuilder_appendColor(&b, ANTIQUE_WHITE, "ANTIQUE_WHITE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BEIGE);
	stringBuilder_appendColor(&b, BEIGE, "BEIGE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BISQUE);
	stringBuilder_appendColor(&b, BISQUE, "BISQUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BLANCHED_ALMOND);
	stringBuilder_appendColor(&b, BLANCHED_ALMOND, "BLANCHED_ALMOND\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(WHEAT);
	stringBuilder_appendColor(&b, WHEAT, "WHEAT\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CORN_SILK);
	stringBuilder_appendColor(&b, CORN_SILK, "CORN_SILK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LEMON_CHIFFON);
	stringBuilder_appendColor(&b, LEMON_CHIFFON, "LEMON_CHIFFON\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_GOLDEN_ROD_YELLOW);
	stringBuilder_appendColor(&b, LIGHT_GOLDEN_ROD_YELLOW, "LIGHT_GOLDEN_ROD_YELLOW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_YELLOW);
	stringBuilder_appendColor(&b, LIGHT_YELLOW, "LIGHT_YELLOW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SADDLE_BROWN);
	stringBuilder_appendColor(&b, SADDLE_BROWN, "SADDLE_BROWN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SIENNA);
	stringBuilder_appendColor(&b, SIENNA, "SIENNA\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(CHOCOLATE);
	stringBuilder_appendColor(&b, CHOCOLATE, "CHOCOLATE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PERU);
	stringBuilder_appendColor(&b, PERU, "PERU\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SANDY_BROWN);
	stringBuilder_appendColor(&b, SANDY_BROWN, "SANDY_BROWN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BURLY_WOOD);
	stringBuilder_appendColor(&b, BURLY_WOOD, "BURLY_WOOD\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(TAN);
	stringBuilder_appendColor(&b, TAN, "TAN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ROSY_BROWN);
	stringBuilder_appendColor(&b, ROSY_BROWN, "ROSY_BROWN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MOCCASIN);
	stringBuilder_appendColor(&b, MOCCASIN, "MOCCASIN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(NAVAJO_WHITE);
	stringBuilder_appendColor(&b, NAVAJO_WHITE, "NAVAJO_WHITE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PEACH_PUFF);
	stringBuilder_appendColor(&b, PEACH_PUFF, "PEACH_PUFF\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MISTY_ROSE);
	stringBuilder_appendColor(&b, MISTY_ROSE, "MISTY_ROSE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LAVENDER_BLUSH);
	stringBuilder_appendColor(&b, LAVENDER_BLUSH, "LAVENDER_BLUSH\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LINEN);
	stringBuilder_appendColor(&b, LINEN, "LINEN\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(OLD_LACE);
	stringBuilder_appendColor(&b, OLD_LACE, "OLD_LACE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(PAPAYA_WHIP);
	stringBuilder_appendColor(&b, PAPAYA_WHIP, "PAPAYA_WHIP\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SEA_SHELL);
	stringBuilder_appendColor(&b, SEA_SHELL, "SEA_SHELL\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(MINT_CREAM);
	stringBuilder_appendColor(&b, MINT_CREAM, "MINT_CREAM\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SLATE_GRAY);
	stringBuilder_appendColor(&b, SLATE_GRAY, "SLATE_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_SLATE_GRAY);
	stringBuilder_appendColor(&b, LIGHT_SLATE_GRAY, "LIGHT_SLATE_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_STEEL_BLUE);
	stringBuilder_appendColor(&b, LIGHT_STEEL_BLUE, "LIGHT_STEEL_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LAVENDER);
	stringBuilder_appendColor(&b, LAVENDER, "LAVENDER\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(FLORAL_WHITE);
	stringBuilder_appendColor(&b, FLORAL_WHITE, "FLORAL_WHITE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(ALICE_BLUE);
	stringBuilder_appendColor(&b, ALICE_BLUE, "ALICE_BLUE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GHOST_WHITE);
	stringBuilder_appendColor(&b, GHOST_WHITE, "GHOST_WHITE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(HONEYDEW);
	stringBuilder_appendColor(&b, HONEYDEW, "HONEYDEW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(IVORY);
	stringBuilder_appendColor(&b, IVORY, "IVORY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(AZURE);
	stringBuilder_appendColor(&b, AZURE, "AZURE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SNOW);
	stringBuilder_appendColor(&b, SNOW, "SNOW\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(BLACK);
	stringBuilder_appendColor(&b, BLACK, "BLACK\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DIM_GRAY);
	stringBuilder_appendColor(&b, DIM_GRAY, "DIM_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GRAY);
	stringBuilder_appendColor(&b, GRAY, "GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(DARK_GRAY);
	stringBuilder_appendColor(&b, DARK_GRAY, "DARK_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(SILVER);
	stringBuilder_appendColor(&b, SILVER, "SILVER\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(LIGHT_GRAY);
	stringBuilder_appendColor(&b, LIGHT_GRAY, "LIGHT_GRAY\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(GAINSBORO);
	stringBuilder_appendColor(&b, GAINSBORO, "GAINSBORO\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(WHITE_SMOKE);
	stringBuilder_appendColor(&b, WHITE_SMOKE, "WHITE_SMOKE\n");

	STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(WHITE);
	stringBuilder_appendColor(&b, WHITE, "WHITE\n");

	printf("%s", stringBuilder_toString(&b));
	
	stringBuilder_free(&b);
}

#endif
