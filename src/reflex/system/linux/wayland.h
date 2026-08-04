#pragma once

#include "[require].h"

#if REFLEX_INCLUDE_UI
#include <wayland-client.h>
#include <wayland-client-protocol.h>

#if __has_include(<xdg-shell-client-protocol.h>)
#include <xdg-shell-client-protocol.h>
#define REFLEX_LINUX_HAS_XDG_SHELL 1
#elif __has_include("ext/xdg-shell-client-protocol.h")
#include "ext/xdg-shell-client-protocol.h"
#define REFLEX_LINUX_HAS_XDG_SHELL 1
#else
#define REFLEX_LINUX_HAS_XDG_SHELL 0
#endif

#endif
