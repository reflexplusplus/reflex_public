#include "freetype.h"

REFLEX_DISABLE_WARNINGS

#include "ext/freetype-2.9.1/src/base/ftinit.c"
#include "ext/freetype-2.9.1/src/base/ftbase.c"
#include "ext/freetype-2.9.1/src/base/ftbbox.c"
#include "ext/freetype-2.9.1/src/base/ftglyph.c"
#include "ext/freetype-2.9.1/src/base/ftbitmap.c"

#include "ext/freetype-2.9.1/src/raster/raster.c"
#include "ext/freetype-2.9.1/src/autofit/autofit.c"
#include "ext/freetype-2.9.1/src/smooth/smooth.c"

#include "ext/freetype-2.9.1/src/truetype/truetype.c"
#include "ext/freetype-2.9.1/src/sfnt/sfnt.c"

#include "ext/freetype-2.9.1/src/cff/cff.c"
#include "ext/freetype-2.9.1/src/psaux/psaux.c"
#include "ext/freetype-2.9.1/src/psnames/psnames.c"
#include "ext/freetype-2.9.1/src/pshinter/pshinter.c"

#undef OP

REFLEX_ENABLE_WARNINGS




//
//mem handling

FT_CALLBACK_DEF(void*) ft_alloc(FT_Memory memory, long size)
{
	return Reflex::g_default_allocator->Allocate(size, Reflex::AllocInfo("Freetype", "ft_alloc"));
}

FT_CALLBACK_DEF(void*) ft_realloc(FT_Memory memory, long cur_size, long new_size, void * block)
{
	if (cur_size > new_size)
	{
		return block;
	}
	else
	{
		Reflex::TRef allocator = *Reflex::g_default_allocator;

		auto ptr = allocator->Allocate(new_size, Reflex::AllocInfo("Freetype", "ft_realloc"));

		Reflex::MemCopy(block, ptr, cur_size);

		allocator->Free(block);

		return ptr;
	}
}

FT_CALLBACK_DEF(void) ft_free(FT_Memory memory, void * block)
{
	Reflex::g_default_allocator->Free(block);
}

FT_EXPORT_DEF(FT_Memory) FT_New_Memory(void)
{
	FT_Memory memory = Reflex::Detail::Allocate<FT_MemoryRec_>(Reflex::g_default_allocator, Reflex::AllocInfo("Freetype", "FT_New_Memory"));

	if (memory)
	{
		memory->user = 0;
		memory->alloc = ft_alloc;
		memory->realloc = ft_realloc;
		memory->free = ft_free;
	}

	return memory;
}

FT_EXPORT_DEF(void) FT_Done_Memory(FT_Memory memory)
{
	Reflex::g_default_allocator->Free(memory);
}
