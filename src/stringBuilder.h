#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include "util.h"

// Helpfull reading material regarding ANSI_ESCAPE_CODES.
// https://notes.burke.libbey.me/ansi-escape-codes/
// Full standard text: (ECMA-48 1991)
// https://www.ecma-international.org/wp-content/uploads/ECMA-48_5th_edition_june_1991.pdf

// Note: \x1b, \e and \033 are all just different ways to represent '27', the value of ESC in the ASCI-Table.
#define STRING_BUILDER_ANSI_ESCAPE_CHARACTER "\x1b"

#define STRING_BUILDER_CLEAR_PARAMETER STRING_BUILDER_ANSI_ESCAPE_CHARACTER "[0m"

#define STRING_BUILDER_INIT_SINGLE_COLOR_TEXT_MODIFIER(color) StringBuilder_TextModifier* color; do{ \
	color = alloca(sizeof(*color)); \
 \
	stringBuilder_initTextModifier(color, &COLOR_ ## color, NULL, 0); \
}while(0)

#define STRING_BUILDER_INIT_TEXT_MODIFIER(name, color, numParameter, ...) StringBuilder_TextModifier* name; do{ \
	name = alloca(sizeof(*name)); \
 \
	stringBuilder_initTextModifier(name, &COLOR_ ## color, NULL, numParameter, __VA_ARGS__); \
}while(0)

#define STRING_BUILDER_INIT_TEXT_MODIFIER_(name, foreGround, backGround, numParameter, ...) StringBuilder_TextModifier* name; do{ \
	name = alloca(sizeof(*name)); \
 \
	stringBuilder_initTextModifier(name, &COLOR_ ## foreGround, &COLOR_ ## backGround, numParameter, __VA_ARGS__); \
}while(0)

typedef struct{
	char* string;
	uint_fast64_t stringLength;
}StringBuilder;

typedef struct{
	union{
		uint8_t red;
		uint8_t r;
	};
	union{
		uint8_t green;
		uint8_t g;
	};
	union{
		uint8_t blue;
		uint8_t b;
	};
}StringBuilder_Color;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
local StringBuilder_Color COLOR_MAROON = {.r = 128, .g = 0, .b = 0};
local StringBuilder_Color COLOR_DARK_RED = {.r = 139, .g = 0, .b = 0};
local StringBuilder_Color COLOR_BROWN = {.r = 165, .g = 42, .b = 42};
local StringBuilder_Color COLOR_FIREBRICK = {.r = 178, .g = 34, .b = 34};
local StringBuilder_Color COLOR_CRIMSON = {.r = 220, .g = 20, .b = 60};
local StringBuilder_Color COLOR_RED = {.r = 255, .g = 0, .b = 0};
local StringBuilder_Color COLOR_TOMATO = {.r = 255, .g = 99, .b = 71};
local StringBuilder_Color COLOR_CORAL = {.r = 255, .g = 127, .b = 80};
local StringBuilder_Color COLOR_INDIAN_RED = {.r = 205, .g = 92, .b = 92};
local StringBuilder_Color COLOR_LIGHT_CORAL = {.r = 240, .g = 128, .b = 128};
local StringBuilder_Color COLOR_DARK_SALMON = {.r = 233, .g = 150, .b = 122};
local StringBuilder_Color COLOR_SALMON = {.r = 250, .g = 128, .b = 114};
local StringBuilder_Color COLOR_LIGHT_SALMON = {.r = 255, .g = 160, .b = 122};
local StringBuilder_Color COLOR_ORANGE_RED = {.r = 255, .g = 69, .b = 0};
local StringBuilder_Color COLOR_DARK_ORANGE = {.r = 255, .g = 140, .b = 0};
local StringBuilder_Color COLOR_ORANGE = {.r = 255, .g = 165, .b = 0};
local StringBuilder_Color COLOR_GOLD = {.r = 255, .g = 215, .b = 0};
local StringBuilder_Color COLOR_DARK_GOLDEN_ROD = {.r = 184, .g = 134, .b = 11};
local StringBuilder_Color COLOR_GOLDEN_ROD = {.r = 218, .g = 165, .b = 32};
local StringBuilder_Color COLOR_PALE_GOLDEN_ROD = {.r = 238, .g = 232, .b = 170};
local StringBuilder_Color COLOR_DARK_KHAKI = {.r = 189, .g = 183, .b = 107};
local StringBuilder_Color COLOR_KHAKI = {.r = 240, .g = 230, .b = 140};
local StringBuilder_Color COLOR_OLIVE = {.r = 128, .g = 128, .b = 0};
local StringBuilder_Color COLOR_YELLOW = {.r = 255, .g = 255, .b = 0};
local StringBuilder_Color COLOR_YELLOW_GREEN = {.r = 154, .g = 205, .b = 50};
local StringBuilder_Color COLOR_DARK_OLIVE_GREEN = {.r = 85, .g = 107, .b = 47};
local StringBuilder_Color COLOR_OLIVE_DRAB = {.r = 107, .g = 142, .b = 35};
local StringBuilder_Color COLOR_LAWN_GREEN = {.r = 124, .g = 252, .b = 0};
local StringBuilder_Color COLOR_CHARTREUSE = {.r = 127, .g = 255, .b = 0};
local StringBuilder_Color COLOR_GREEN_YELLOW = {.r = 173, .g = 255, .b = 47};
local StringBuilder_Color COLOR_DARK_GREEN = {.r = 0, .g = 100, .b = 0};
local StringBuilder_Color COLOR_GREEN = {.r = 0, .g = 128, .b = 0};
local StringBuilder_Color COLOR_FOREST_GREEN = {.r = 34, .g = 139, .b = 34};
local StringBuilder_Color COLOR_LIME = {.r = 0, .g = 255, .b = 0};
local StringBuilder_Color COLOR_LIME_GREEN = {.r = 50, .g = 205, .b = 50};
local StringBuilder_Color COLOR_LIGHT_GREEN = {.r = 144, .g = 238, .b = 144};
local StringBuilder_Color COLOR_PALE_GREEN = {.r = 152, .g = 251, .b = 152};
local StringBuilder_Color COLOR_DARK_SEA_GREEN = {.r = 143, .g = 188, .b = 143};
local StringBuilder_Color COLOR_MEDIUM_SPRING_GREEN = {.r = 0, .g = 250, .b = 154};
local StringBuilder_Color COLOR_SPRING_GREEN = {.r = 0, .g = 255, .b = 127};
local StringBuilder_Color COLOR_SEA_GREEN = {.r = 46, .g = 139, .b = 87};
local StringBuilder_Color COLOR_MEDIUM_AQUA_MARINE = {.r = 102, .g = 205, .b = 170};
local StringBuilder_Color COLOR_MEDIUM_SEA_GREEN = {.r = 60, .g = 179, .b = 113};
local StringBuilder_Color COLOR_LIGHT_SEA_GREEN = {.r = 32, .g = 178, .b = 170};
local StringBuilder_Color COLOR_DARK_SLATE_GRAY = {.r = 47, .g = 79, .b = 79};
local StringBuilder_Color COLOR_TEAL = {.r = 0, .g = 128, .b = 128};
local StringBuilder_Color COLOR_DARK_CYAN = {.r = 0, .g = 139, .b = 139};
local StringBuilder_Color COLOR_AQUA = {.r = 0, .g = 255, .b = 255};
local StringBuilder_Color COLOR_CYAN = {.r = 0, .g = 255, .b = 255};
local StringBuilder_Color COLOR_LIGHT_CYAN = {.r = 224, .g = 255, .b = 255};
local StringBuilder_Color COLOR_DARK_TURQUOISE = {.r = 0, .g = 206, .b = 209};
local StringBuilder_Color COLOR_TURQUOISE = {.r = 64, .g = 224, .b = 208};
local StringBuilder_Color COLOR_MEDIUM_TURQUOISE = {.r = 72, .g = 209, .b = 204};
local StringBuilder_Color COLOR_PALE_TURQUOISE = {.r = 175, .g = 238, .b = 238};
local StringBuilder_Color COLOR_AQUA_MARINE = {.r = 127, .g = 255, .b = 212};
local StringBuilder_Color COLOR_POWDER_BLUE = {.r = 176, .g = 224, .b = 230};
local StringBuilder_Color COLOR_CADET_BLUE = {.r = 95, .g = 158, .b = 160};
local StringBuilder_Color COLOR_STEEL_BLUE = {.r = 70, .g = 130, .b = 180};
local StringBuilder_Color COLOR_CORN_FLOWER_BLUE = {.r = 100, .g = 149, .b = 237};
local StringBuilder_Color COLOR_DEEP_SKY_BLUE = {.r = 0, .g = 191, .b = 255};
local StringBuilder_Color COLOR_DODGER_BLUE = {.r = 30, .g = 144, .b = 255};
local StringBuilder_Color COLOR_LIGHT_BLUE = {.r = 173, .g = 216, .b = 230};
local StringBuilder_Color COLOR_SKY_BLUE = {.r = 135, .g = 206, .b = 235};
local StringBuilder_Color COLOR_LIGHT_SKY_BLUE = {.r = 135, .g = 206, .b = 250};
local StringBuilder_Color COLOR_MIDNIGHT_BLUE = {.r = 25, .g = 25, .b = 112};
local StringBuilder_Color COLOR_NAVY = {.r = 0, .g = 0, .b = 128};
local StringBuilder_Color COLOR_DARK_BLUE = {.r = 0, .g = 0, .b = 139};
local StringBuilder_Color COLOR_MEDIUM_BLUE = {.r = 0, .g = 0, .b = 205};
local StringBuilder_Color COLOR_BLUE = {.r = 0, .g = 0, .b = 255};
local StringBuilder_Color COLOR_ROYAL_BLUE = {.r = 65, .g = 105, .b = 225};
local StringBuilder_Color COLOR_BLUE_VIOLET = {.r = 138, .g = 43, .b = 226};
local StringBuilder_Color COLOR_INDIGO = {.r = 75, .g = 0, .b = 130};
local StringBuilder_Color COLOR_DARK_SLATE_BLUE = {.r = 72, .g = 61, .b = 139};
local StringBuilder_Color COLOR_SLATE_BLUE = {.r = 106, .g = 90, .b = 205};
local StringBuilder_Color COLOR_MEDIUM_SLATE_BLUE = {.r = 123, .g = 104, .b = 238};
local StringBuilder_Color COLOR_MEDIUM_PURPLE = {.r = 147, .g = 112, .b = 219};
local StringBuilder_Color COLOR_DARK_MAGENTA = {.r = 139, .g = 0, .b = 139};
local StringBuilder_Color COLOR_DARK_VIOLET = {.r = 148, .g = 0, .b = 211};
local StringBuilder_Color COLOR_DARK_ORCHID = {.r = 153, .g = 50, .b = 204};
local StringBuilder_Color COLOR_MEDIUM_ORCHID = {.r = 186, .g = 85, .b = 211};
local StringBuilder_Color COLOR_PURPLE = {.r = 128, .g = 0, .b = 128};
local StringBuilder_Color COLOR_THISTLE = {.r = 216, .g = 191, .b = 216};
local StringBuilder_Color COLOR_PLUM = {.r = 221, .g = 160, .b = 221};
local StringBuilder_Color COLOR_VIOLET = {.r = 238, .g = 130, .b = 238};
local StringBuilder_Color COLOR_MAGENTA = {.r = 255, .g = 0, .b = 255};
local StringBuilder_Color COLOR_FUCHSIA = {.r = 255, .g = 0, .b = 255};
local StringBuilder_Color COLOR_ORCHID = {.r = 218, .g = 112, .b = 214};
local StringBuilder_Color COLOR_MEDIUM_VIOLET_RED = {.r = 199, .g = 21, .b = 133};
local StringBuilder_Color COLOR_PALE_VIOLET_RED = {.r = 219, .g = 112, .b = 147};
local StringBuilder_Color COLOR_DEEP_PINK = {.r = 255, .g = 20, .b = 147};
local StringBuilder_Color COLOR_HOT_PINK = {.r = 255, .g = 105, .b = 180};
local StringBuilder_Color COLOR_LIGHT_PINK = {.r = 255, .g = 182, .b = 193};
local StringBuilder_Color COLOR_PINK = {.r = 255, .g = 192, .b = 203};
local StringBuilder_Color COLOR_ANTIQUE_WHITE = {.r = 250, .g = 235, .b = 215};
local StringBuilder_Color COLOR_BEIGE = {.r = 245, .g = 245, .b = 220};
local StringBuilder_Color COLOR_BISQUE = {.r = 255, .g = 228, .b = 196};
local StringBuilder_Color COLOR_BLANCHED_ALMOND = {.r = 255, .g = 235, .b = 205};
local StringBuilder_Color COLOR_WHEAT = {.r = 245, .g = 222, .b = 179};
local StringBuilder_Color COLOR_CORN_SILK = {.r = 255, .g = 248, .b = 220};
local StringBuilder_Color COLOR_LEMON_CHIFFON = {.r = 255, .g = 250, .b = 205};
local StringBuilder_Color COLOR_LIGHT_GOLDEN_ROD_YELLOW = {.r = 250, .g = 250, .b = 210};
local StringBuilder_Color COLOR_LIGHT_YELLOW = {.r = 255, .g = 255, .b = 224};
local StringBuilder_Color COLOR_SADDLE_BROWN = {.r = 139, .g = 69, .b = 19};
local StringBuilder_Color COLOR_SIENNA = {.r = 160, .g = 82, .b = 45};
local StringBuilder_Color COLOR_CHOCOLATE = {.r = 210, .g = 105, .b = 30};
local StringBuilder_Color COLOR_PERU = {.r = 205, .g = 133, .b = 63};
local StringBuilder_Color COLOR_SANDY_BROWN = {.r = 244, .g = 164, .b = 96};
local StringBuilder_Color COLOR_BURLY_WOOD = {.r = 222, .g = 184, .b = 135};
local StringBuilder_Color COLOR_TAN = {.r = 210, .g = 180, .b = 140};
local StringBuilder_Color COLOR_ROSY_BROWN = {.r = 188, .g = 143, .b = 143};
local StringBuilder_Color COLOR_MOCCASIN = {.r = 255, .g = 228, .b = 181};
local StringBuilder_Color COLOR_NAVAJO_WHITE = {.r = 255, .g = 222, .b = 173};
local StringBuilder_Color COLOR_PEACH_PUFF = {.r = 255, .g = 218, .b = 185};
local StringBuilder_Color COLOR_MISTY_ROSE = {.r = 255, .g = 228, .b = 225};
local StringBuilder_Color COLOR_LAVENDER_BLUSH = {.r = 255, .g = 240, .b = 245};
local StringBuilder_Color COLOR_LINEN = {.r = 250, .g = 240, .b = 230};
local StringBuilder_Color COLOR_OLD_LACE = {.r = 253, .g = 245, .b		 = 230};
local StringBuilder_Color COLOR_PAPAYA_WHIP = {.r = 255, .g = 239, .b = 213};
local StringBuilder_Color COLOR_SEA_SHELL = {.r = 255, .g = 245, .b = 238};
local StringBuilder_Color COLOR_MINT_CREAM = {.r = 245, .g = 255, .b = 250};
local StringBuilder_Color COLOR_SLATE_GRAY = {.r = 112, .g = 128, .b = 144};
local StringBuilder_Color COLOR_LIGHT_SLATE_GRAY = {.r = 119, .g = 136, .b = 153};
local StringBuilder_Color COLOR_LIGHT_STEEL_BLUE = {.r = 176, .g = 196, .b = 222};
local StringBuilder_Color COLOR_LAVENDER = {.r = 230, .g = 230, .b = 250};
local StringBuilder_Color COLOR_FLORAL_WHITE = {.r = 255, .g = 250, .b = 240};
local StringBuilder_Color COLOR_ALICE_BLUE = {.r = 240, .g = 248, .b = 255};
local StringBuilder_Color COLOR_GHOST_WHITE = {.r = 248, .g = 248, .b = 255};
local StringBuilder_Color COLOR_HONEYDEW = {.r = 240, .g = 255, .b = 240};
local StringBuilder_Color COLOR_IVORY = {.r = 255, .g = 255, .b = 240};
local StringBuilder_Color COLOR_AZURE = {.r = 240, .g = 255, .b = 255};
local StringBuilder_Color COLOR_SNOW = {.r = 255, .g = 250, .b = 250};
local StringBuilder_Color COLOR_BLACK = {.r = 0, .g = 0, .b = 0};
local StringBuilder_Color COLOR_DIM_GRAY = {.r = 105, .g = 105, .b = 105};
local StringBuilder_Color COLOR_GRAY = {.r = 128, .g = 128, .b = 128};
local StringBuilder_Color COLOR_DARK_GRAY = {.r = 169, .g = 169, .b = 169};
local StringBuilder_Color COLOR_SILVER = {.r = 192, .g = 192, .b = 192};
local StringBuilder_Color COLOR_LIGHT_GRAY = {.r = 211, .g = 211, .b = 211};
local StringBuilder_Color COLOR_GAINSBORO = {.r = 220, .g = 220, .b = 220};
local StringBuilder_Color COLOR_WHITE_SMOKE = {.r = 245, .g = 245, .b = 245};
local StringBuilder_Color COLOR_WHITE = {.r = 255, .g = 255, .b = 255};
#pragma GCC diagnostic pop

typedef enum{
	SELECT_GRAPHIC_RENDITION_PARAMETER_RESET = 0,
	SELECT_GRAPHIC_RENDITION_PARAMETER_NORMAL = 0,
	SELECT_GRAPHIC_RENDITION_PARAMETER_BOLD = 1,
	SELECT_GRAPHIC_RENDITION_PARAMETER_FAINT = 2,
	SELECT_GRAPHIC_RENDITION_PARAMETER_ITALIC = 3,
	SELECT_GRAPHIC_RENDITION_PARAMETER_UNDERLINE = 4,
	SELECT_GRAPHIC_RENDITION_PARAMETER_SLOW_BLINK = 5,
}StringBuilder_SelectGraphicRenditionParamaeters;

typedef struct{
	StringBuilder_Color* foregroundColor;
	StringBuilder_Color* backgroundColor;
	uint_fast64_t numParameters;
	StringBuilder_SelectGraphicRenditionParamaeters SGR_Parameters[];
}StringBuilder_TextModifier;

void stringBuilder_initTextModifier(StringBuilder_TextModifier*, StringBuilder_Color*, StringBuilder_Color*, const uint_fast64_t, ...);

ERROR_CODE stringBuilder_append(StringBuilder*, char*);

ERROR_CODE stringBuilder_append_f(StringBuilder*, char*, ...);

ERROR_CODE stringBuilder_append_s(StringBuilder*, char*, const uint_fast64_t);

ERROR_CODE stringBuilder_appendColor(StringBuilder*, StringBuilder_TextModifier*, char*);

ERROR_CODE stringBuilder_appendColor_s(StringBuilder*, StringBuilder_TextModifier*, char*, uint_fast64_t);

ERROR_CODE stringBuilder_appendColor_f(StringBuilder*, StringBuilder_TextModifier*, char* , ...);

char* stringBuilder_toString(StringBuilder*);

void stringBuilder_free(StringBuilder*);

void stringBuilder_printColorChart(void);

#endif
