#pragma once

#include "../event.h"




//
//TextEditBehaviour

namespace Reflex::GLX
{

	REFLEX_GLX_EVENT_ID(Complete);

	struct TextEditBehaviour :
		public Object::Delegate,
		public Data::History
	{
		REFLEX_OBJECT(GLX::TextEditBehaviour, Delegate);

		static TextEditBehaviour & null;

		REFLEX_DECLARE_KEY32(TextEdit);

		REFLEX_USE_ENUM(System, VirtualKeyboardInputType);


		static TRef <TextEditBehaviour> Create(Key32 dataid = kvalue);


		virtual void SetInputType(VirtualKeyboardInputType type) = 0;	//TODO currently kVirtualKeyboardInputMultiLine is ignored, that depends on Text type

		virtual void SetCaret(UInt position, UInt selection_start, UInt selection_length) = 0;

		virtual Tuple <UInt,UInt,UInt> GetCaret() const = 0;

		virtual void Update() = 0;

		virtual void Reveal() = 0;

		virtual Float GetLineHeight() const = 0;

		virtual Pair <Float> GetLineCoordinates(UInt idx) const = 0;


		using History::Reset;

		using History::Deserialize;

		using History::Serialize;



	protected:

		TextEditBehaviour() : Data::History(kMaxUInt16) {}

	};

	TRef <Reflex::Object> AcquireVirtualKeyboard();

}
