#pragma once

#include "../[require].h"
#include "../../common/instance/audioapp.h"




//
//declarations

REFLEX_NS(Reflex::System::Win)

TRef <Common::DesktopAudioAppBase> CreateASIO();

REFLEX_END