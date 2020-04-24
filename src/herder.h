#ifndef HERDER_H
#define HERDER_H

#include "util.h"

#define HERDER_CONSTRUCT_RELATIVE_FILE_PATH(path, stringLength, episodeInfo) \
    char* _noWhiteSpaceShowName = alloca(sizeof(*_noWhiteSpaceShowName) * ((episodeInfo)->showNameLength + 1)); \
    strncpy(_noWhiteSpaceShowName, (episodeInfo)->showName, (episodeInfo)->showNameLength + 1); \
    util_replaceAllChars(_noWhiteSpaceShowName, ' ', '_'); \
    *stringLength = ((episodeInfo)->showNameLength * 3) + ((episodeInfo)->nameLength) + (3/*" - "*/ + 7/*"Season_"*/ + 2/*"/"*/) + (UTIL_UINT16_STRING_LENGTH * 3) + 1/*.*/ + (episodeInfo)->fileExtensionLength; \
     \
    *(path) = alloca(sizeof(**(path)) * (*stringLength + 1)); \
    *stringLength = snprintf(*path, *stringLength, "%s/%s - Season_%02" PRIdFAST16 "/%s_s%02" PRIdFAST16 "e%02" PRIdFAST16 "_%s.%s", _noWhiteSpaceShowName, _noWhiteSpaceShowName, (episodeInfo)->season, _noWhiteSpaceShowName, (episodeInfo)->season, (episodeInfo)->episode, (episodeInfo)->name, (episodeInfo)->fileExtension); \
     \
    util_replaceAllChars(*path + (*stringLength - ((episodeInfo)->showNameLength + (2 * UTIL_UINT16_STRING_LENGTH) + (episodeInfo)->nameLength + 4)), ' ', '_')

ERROR_CODE herder_removeShow(Property*, Property*, const char*, const uint_fast64_t);

ERROR_CODE herder_pullShowList(LinkedList*, Property*, Property*);

ERROR_CODE herder_addShow(Property*, Property*, const char*, const uint_fast64_t);

ERROR_CODE herder_extractShowInfo(Property*, Property*, EpisodeInfo*);

ERROR_CODE herder_addEpisode(Property*, Property*, Property*, EpisodeInfo*);

ERROR_CODE herder_pullShowInfo(Property*, Property*, Show*);

ERROR_CODE herder_add(Property*, Property*, Property*, EpisodeInfo*);

ERROR_CODE herder_renameEpisode(Property*, Property*, Property*, Show*, Season*, Episode*, char*, const uint_fast64_t);

#endif