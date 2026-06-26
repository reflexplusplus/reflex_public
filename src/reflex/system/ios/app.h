#pragma once

#include "sdk.h"




#ifdef REFLEX_SYSTEM_AUDIO
#include "app/audio_app.h"
typedef Reflex::System::iOS::AudioApp AppType;
#else
#include "../common/instance/standalone.h"
typedef Reflex::System::Common::NonAudioApp AppType;
#endif
