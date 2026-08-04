#pragma once

#include "../common/instance/audioapp.h"




//
//declarations

REFLEX_NS(Reflex::System::OSX)

TRef <Common::DesktopAudioAppBase> CreateCoreAudio();

REFLEX_END
