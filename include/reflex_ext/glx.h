#pragma once

#include "reflex/glx.h"

#if REFLEX_INCLUDE_UI

#include "glx/behaviours/split.h"
#include "glx/behaviours/popup.h"
#include "glx/behaviours/floating_dialog.h"

#include "glx/widgets/menu.h"
#include "glx/widgets/split.h"
#include "glx/widgets/selector.h"
#include "glx/widgets/abstractlist.h"
#include "glx/widgets/list.h"
#include "glx/widgets/virtuallist.h"
#include "glx/widgets/tree.h"
#include "glx/widgets/label.h"
#include "glx/widgets/button.h"
#include "glx/widgets/popup.h"
#include "glx/widgets/rotaryslider.h"
#include "glx/widgets/textarea.h"
#include "glx/widgets/dragedit.h"
#include "glx/widgets/form.h"
#include "glx/widgets/flow_dialog.h"
#include "glx/widgets/accordion.h"
#include "glx/widgets/tabgroup.h"
#include "glx/widgets/paginator.h"

#include "glx/animation/playlist.h"
#include "glx/animation/multi.h"
#include "glx/animation/functions.h"

#include "glx/functions/dialogs.h"
#include "glx/functions/window.h"
#include "glx/functions/state.h"
#include "glx/functions/focus.h"
#include "glx/functions/enter_exit.h"
#include "glx/functions/hotkey.h"
#include "glx/functions/viewport.h"
#include "glx/functions/overlay.h"
#include "glx/functions/legacy.h"

#include "glx/detail/functions.h"
#include "glx/detail/recycler.h"

#endif
