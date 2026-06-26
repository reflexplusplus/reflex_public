#define REFLEX_MACOS_TARGET Library
#define REFLEX_INCLUDE_UI false

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla"

#include "osx/build.mm"
#include "common/instance/library.cpp"

#pragma clang diagnostic pop

void Reflex::System::App::Quit()
{
}
