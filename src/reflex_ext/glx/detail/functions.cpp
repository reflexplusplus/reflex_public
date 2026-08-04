#include "../../../../include/reflex_ext/glx/detail/functions.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

REFLEX_DECLARE_KEY32(Hide);

UInt32 g_setcontent_id = 0;

struct MouseTracker : public Object::Delegate
{
	MouseTracker(GLX::Object & object, const Function <void(const Point&)> & onmouseover, const Function <void()> & onmouseleave)
		: m_onmouseover(onmouseover),
		m_onmouseleave(onmouseleave)
	{
		if (Core::desktop->GetMouseOver().Adr() == &object)
		{
			m_onclock = Core::desktop->CreateAnimationClock(Bind(&MouseTracker::OnClock, this, _P1));
		}
	}

	~MouseTracker()
	{
		if (IsValid(*object)) object->ClearDelegate(kMouseEnter);
	}

	virtual bool OnEvent(GLX::Object & src, Event & e) override
	{
		auto id = e.id;

		if (id == kMouseEnter)
		{
			m_onclock = Core::desktop->CreateAnimationClock(Bind(&MouseTracker::OnClock, this, _P1));

			return true;
		}
		else if (id == kMouseLeave)
		{
			m_onclock.Clear();

			m_onmouseleave();

			return true;
		}

		return false;
	}

	void OnClock(Float)
	{
		m_onmouseover(GetMousePosition(object));
	}

	Reference <Reflex::Object> m_onclock;

	Function <void(const Point&)> m_onmouseover;

	Function <void()> m_onmouseleave;
};

template <class AXIS> void Exchange(Object & self, Object & current, Object & next, bool far)
{
	constexpr Float st_time = 0.25f;
	constexpr Float st_out = 0.125f;	//0.125f;
	constexpr Float st_in = 0.5f;
	constexpr Float st_max = 192.0f;

	typedef typename AXIS::Ortho Ortho;


	auto & size = self.GetRect().size;

	self.ComputeLayout();


	Float in_distance = Reflex::RoundNearest(Reflex::Min(AXIS::GetSize(size) * st_in, st_max));

	Float out_distance = Reflex::RoundNearest(in_distance * st_out);

	Float time = (in_distance / st_max) * st_time;


	auto out = AddAbsolute(self, Detail::CreateSnapshot());

	out->SetContent(current);	//current is stored

	self.SetProperty(K32("Exchange"), REFLEX_CREATE(StrongRefObject, next));	//do here already in case causing feedback to Detail::SetContent


	auto & rect = current.GetRect();

	auto outpos = rect.origin;

	out->SetRect(rect);

	AXIS::IncPoint(outpos, out_distance * (far ? -1.0f : 1.0f));

	auto moveout = CreatePositionAnimation(AXIS::kY, AXIS::GetPoint(rect.origin), AXIS::GetPoint(outpos));

	moveout->SetTarget(out);

	moveout->SetTime(time);

	moveout->SetEasing(InterpolatedAnimation::kEaseOut3x);

	
	auto fadeout = CreateOpacityAnimation({}, 1.0f, 0.0f);

	fadeout->SetTarget(out);

	fadeout->SetTime(time);


	current.Detach();

	AddStretch(self, next);	//for bg colour detection

	{
		self.ComputeLayout();

		self.Realign();

		self.ComputeLayout();	//workaround for wrap layouts, shouldnt be neccesary
	}

	auto in = AddAbsolute(self, Detail::CreateSnapshot());

	SetOpacity(in, {}, 0.0f, Detail::ComputedStyle::kRenderFalse);

	in->SetContent(next);

	EnableDraw(next, K32("Exchange"), false);


	auto inpos = next.GetRect().origin;

	AXIS::IncPoint(inpos, in_distance * (far ? 1.0f : -1.0f));

	in->SetPosition(inpos);

	auto movein = CreatePositionAnimation(AXIS::kY, AXIS::GetPoint(inpos), AXIS::GetPoint(next.GetRect().origin));

	movein->SetTarget(in);

	movein->SetTime(time);

	movein->SetEasing(InterpolatedAnimation::kEaseOut3x);

	
	auto fadein = CreateOpacityAnimation({}, 0.0f, 1.0f);

	fadein->SetTarget(in);

	fadein->SetTime(time);

	fadein->SetEasing(InterpolatedAnimation::kEaseIn3x);


	TRef multi = REFLEX_CREATE(Multi);
	
	multi->Add(moveout);
	
	multi->Add(fadeout);
	
	multi->Add(movein);
	
	multi->Add(fadein);

	
	auto playlist = REFLEX_CREATE(PlayList);

	playlist->Add(multi);

	playlist->Add(CreateCallbackAnimation([out = Core::WeakReference(out), in = Core::WeakReference(in), next = Core::WeakReference(next)](Object & target)
	{
		EnableDraw(next, K32("Exchange"), true);

		out->Detach();

		in->Detach();
	}));

	self.SetProperty(++g_setcontent_id & 3, playlist);

	playlist->Play();
}

REFLEX_END_INTERNAL

void Reflex::GLX::Detail::BeginMouseTracking(Object & object, const Function <void(const Point&)> & onmouseover, const Function <void()> & onmouseleave)
{
	object.SetDelegate(K32("MouseTracker"), REFLEX_CREATE(MouseTracker, object, onmouseover, onmouseleave));
}

void Reflex::GLX::Detail::EndMouseTracking(Object & object)
{
	object.ClearDelegate(K32("MouseTracker"));
}

Reflex::TRef <Reflex::GLX::TextArea> Reflex::GLX::Detail::BeginTextEdit_DEPRECATED(Object & object, const Style & style, const WString::View & value, const Function <void(const WString&)> & ondone, const Function <void(const WString&)> & onedit, const Function <void()> & oncancel)
{
	auto textarea = New<TextArea>();

	textarea->SetStyle(style);

	AddStretch(object, textarea);

	BindEvent(textarea, kTransaction, [textarea, onedit, ondone, oncancel](GLX::Object & src, Event & e)
	{
		auto ref = AutoRelease(textarea);

		auto textedit = textarea->GetContent();

		switch (GetTransactionStage(e))
		{
		case kTransactionPerform:
			onedit(GetText(textedit));
			break;

		case kTransactionEnd:
			ondone(GetText(textedit));
			textarea->Detach();
			break;

		case kTransactionCancel:
			oncancel();
			textarea->Detach();
			break;
		};

		return true;
	});

	auto textedit = textarea->GetContent();

	SetText(textedit, value);

	textedit->Focus();

	return textarea;
}

Reflex::TRef <Reflex::GLX::TextEditBehaviour> Reflex::GLX::Detail::BeginTextEdit(Object & object, const Function <void(TransactionStage stage, const WString &)> & callback, Key32 dataid)
{
	Object::null.Focus();

	auto textedit = Make<TextEditBehaviour>(dataid);

	object.SetDelegate(TextEditBehaviour::kTextEdit, textedit);

	BindEvent(object, kTransaction, [textedit, callback, dataid](Object & source, Event & e)
	{
		Core::WeakReference src(source);

		auto stage = GetTransactionStage(e);

		callback(stage, GetText(*src, dataid));

		if (stage >= kTransactionEnd)
		{
			textedit->Detach();

			UnbindEvent(src, kTransaction);
		}

		return true;
	});

	object.Focus();

	return textedit.Adr();
}

void Reflex::GLX::Detail::Show(Object & object)
{
	EnableDraw(object, kHide, true);

	UnsetBounds(object, kHide);
}

void Reflex::GLX::Detail::Hide(Object & object)
{
	EnableDraw(object, kHide, false);

	SetBounds(object, kHide, {}, {});
}

void Reflex::GLX::Detail::SetContentAnimated(Object & parent, Object & content, bool yaxis, bool far)
{
	if (auto pref = parent.QueryProperty<StrongRefObject>(K32("Exchange")))
	{
		auto current = pref->ref.Adr();

		if (current != &content)
		{
			auto t = AutoRelease(current);	//can die inside Exchange because it re-sets StrongRefObject

			if (yaxis)
			{
				Exchange<YAxis>(parent, *current, content, far);
			}
			else
			{
				Exchange<XAxis>(parent, *current, content, far);
			}
		}
	}
	else
	{
		content.Accommodate();

		AddStretch(parent, content);

		parent.SetProperty(K32("Exchange"), REFLEX_CREATE(StrongRefObject, content));
	}
}

void Reflex::GLX::Detail::SetContentNotAnimated(Object & parent, Object & content, bool yaxis, bool far)
{
	parent.Clear();

	AddStretch(parent, content);

	parent.SetProperty(K32("Exchange"), REFLEX_CREATE(StrongRefObject, content));
}
