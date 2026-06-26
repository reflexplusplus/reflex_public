#define REFLEX_MACOS_TARGET Console
#define REFLEX_INCLUDE_UI 0

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla"

#include "osx/build.mm"
#include "common/instance/console.cpp"

#pragma clang diagnostic pop
