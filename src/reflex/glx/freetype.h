#pragma once

#include "../../../include/reflex/core/preprocessor.h"
#include "../../../include/reflex/core/allocator.h"
#include "../../../include/reflex/core/detail/functions/allocate.h"
#include "../../../include/reflex/core/functions/memory.h"

REFLEX_DISABLE_WARNINGS

#define FT2_BUILD_LIBRARY
#include "freetype/config/ftheader.h"
#include "freetype/freetype.h"
#include "freetype/ftglyph.h"

#include FT_FREETYPE_H
#include FT_GLYPH_H

REFLEX_ENABLE_WARNINGS

REFLEX_STATIC_ASSERT(sizeof(FT_Int32) == 4);
REFLEX_STATIC_ASSERT(sizeof(FT_UInt32) == 4);
