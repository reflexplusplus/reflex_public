#if defined REFLEX_LIBRARY
#pragma message(" * REFLEX_LIBRARY *")
#else
#pragma message(" * REFLEX_FRAMEWORK *")
#endif

#if (REFLEX_64BIT)
#pragma message(" * REFLEX_64BIT = True *")
#else
#pragma message(" * REFLEX_64BIT = False *")
#endif

#if (REFLEX_DEBUG)
#pragma message(" * REFLEX_DEBUG = True *")
#else
#pragma message(" * REFLEX_DEBUG = False *")
#endif

#if (REFLEX_MINIMAL)
#pragma message(" * REFLEX_MINIMAL = True *")
#else
#pragma message(" * REFLEX_MINIMAL = False *")
#endif
