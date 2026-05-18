#include "../../../../include/reflex_ext/glx/functions/enter_exit.h"
#include "../../../../include/reflex_ext/glx/animation/playlist.h"
#include "../../../../include/reflex_ext/glx/animation/multi.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_DECLARE_KEY32(entering);

constexpr Float kEnterExitFadeTime = 0.25f;

constexpr UInt32 kentering_opacity = Reflex::Detail::MergeHashes(kentering, kopacity);

struct EnterExitState : public Reflex::Object
{
	REFLEX_IF_DEBUG(REFLEX_OBJECT(GLX::EnterExitState, Reflex::Object));

	enum Stage
	{
		kStageInit,
		kStageEntered,
		kStageExited,
	};

	//static constexpr Float st_in = 0.5f;
	//static constexpr Float st_out = 0.125f;	//0.125f;
	//static constexpr Float st_max = 192.0f;


	EnterExitState(GLX::Object & object, bool entering, bool fade)
		: mouse(MouseEnabled(object)),
		stage(kStageInit),
		flags(kEnterAnimationFade | kEnterAnimationSize),
		render(object.GetMod(kentering_opacity)->GetRender())
	{
		if (entering)
		{
			if (fade)
			{
				SetOpacity(object, kentering, 0.0f, render);
			}

			current_opacity = fade ? 0.0f : 1.0f;

			current_size = {};
		}
		else
		{
			current_opacity = GetOpacity(object, kentering);

			current_size = object.GetRect().size;
		}
	}

	static TRef <EnterExitState> Acquire(GLX::Object & object, bool entering, bool fade)
	{
		if (auto data = object.QueryProperty<EnterExitState>(kentering))
		{
			data->current_opacity = GetOpacity(object, kentering);

			data->current_size = object.GetRect().size;

			return *data;
		}
		else
		{
			auto state = REFLEX_CREATE(EnterExitState, object, entering, fade);

			object.SetProperty(kentering, state);

			return state;
		}
	}

	void ApplyClip(GLX::Object & object, bool y)
	{
		bool clip = !True(flags & kEnterNoClip);

		SetClip(object, kentering, !y && clip, y && clip);
	}


	Pair <bool> mouse;

	Stage stage = kStageInit;

	UInt8 flags;

	Detail::ComputedStyle::Render render;

	Float current_opacity;

	Size current_size;
};

REFLEX_END_INTERNAL

void Reflex::GLX::Enter(Object & object, UInt8 flags)
{
	auto data = EnterExitState::Acquire(object, true, flags & kEnterAnimationFade);

	data->flags = flags;

	if (SetFiltered(data->stage, EnterExitState::kStageEntered))
	{
		auto & mouse = data->mouse;

		EnableMouse(object, mouse.a, mouse.b);


		Stop(object, kentering);

		TRef playlist = REFLEX_CREATE(PlayList);

		TRef multi = AddScene(playlist, REFLEX_CREATE(Multi));


		Detail::Show(object);

		
		//fade

		if (flags & kEnterAnimationFade)
		{
			auto fade = CreateOpacityAnimation(kentering, 0.0f, 1.0f, data->render);

			fade->SetTime(kEnterExitFadeTime);

			fade->SetEasing(InterpolatedAnimation::kEaseIn2x);

			AddScene(multi, fade);
		}
		else
		{
			UnsetOpacity(object, kentering);
		}


		//size

		TRef <Animation> finish;

		UnsetBounds(object, kentering);

		bool y = GetAxis(object.GetParent());

		if (flags & kEnterAnimationSize)
		{
			data->ApplyClip(object, y);

			Float from = Detail::GetSize(y, data->current_size);

			Float to = Detail::GetSize(y, Detail::ComputeContentSize(object));

			multi->Add(CreateMaxBoundsAnimation(kentering, y, from, to));

			finish = CreateCallbackAnimation([](Object & object)
			{
				UnsetOpacity(object, kentering);

				UnsetClip(object, kentering);

				UnsetBounds(object, kentering);

				object.ClearState(kentering);
			});
		}
		else
		{
			finish = CreateCallbackAnimation([](Object & object)
			{
				UnsetOpacity(object, kentering);

				object.ClearState(kentering);
			});
		}

		playlist->Add(finish);

		object.SetState(kentering);

		Run(object, kentering, playlist);
	}
}

void Reflex::GLX::Exit(Object & object, bool detach, UInt8 or_flags)
{
	auto data = EnterExitState::Acquire(object, false, false);

	if (SetFiltered(data->stage, EnterExitState::kStageExited))
	{
		Stop(object, kentering);

		TRef playlist = REFLEX_CREATE(PlayList);

		TRef multi = AddScene(playlist, REFLEX_CREATE(Multi));


		//object.ComputeLayout();	//cant see why needed, removed 5/5/2025

		EnableMouse(object, false, true);

		auto flags = data->flags | or_flags;



		//fade

		if (flags & kEnterAnimationFade)
		{
			auto fade = CreateOpacityAnimation(kentering, 1.0f, 0.0f, data->render);

			fade->SetTime(kEnterExitFadeTime);

			AddScene(multi, fade);
		}

		
		//size
		
		bool y = GetAxis(object.GetParent());

		if (flags & kEnterAnimationSize)
		{
			data->ApplyClip(object, y);

			AddScene(multi, CreateMaxBoundsAnimation(kentering, y, Detail::GetSize(y, data->current_size), 0.0f));
		}


		//finish

		TRef <Animation> finish;

		if (detach)
		{
			finish = CreateCallbackAnimation([](Object & object)
			{
				UnsetClip(object, kentering);

				object.ClearState(kentering);

				object.Detach();
			});
		}
		else if (data->flags & kEnterAnimationFade)
		{
			finish = CreateCallbackAnimation([](Object & object)
			{
				UnsetClip(object, kentering);

				//EnableMouse(object, false, true);

				Detail::Hide(object);

				object.ClearState(kentering);
			});
		}
		else
		{
			finish = CreateCallbackAnimation([](Object & object)
			{
				//EnableMouse(object, false, false);

				object.ClearState(kentering);
			});
		}

		playlist->Add(finish);

		object.SetState(kentering);

		Run(object, kentering, playlist);
	}
}

void Reflex::GLX::SkipEnter(Object & object, UInt8 flags)
{
	auto data = EnterExitState::Acquire(object, true, false);

	data->flags = flags;

	if (SetFiltered(data->stage, EnterExitState::kStageEntered))
	{
		auto & mouse = data->mouse;

		EnableMouse(object, mouse.a, mouse.b);

		Stop(object, kentering);

		UnsetOpacity(object, kentering);

		UnsetBounds(object, kentering);

		UnsetClip(object, kentering);

		Detail::Show(object);
	}
}

void Reflex::GLX::SkipExit(Object & object, bool detach, UInt8 or_flags)
{
	auto data = EnterExitState::Acquire(object, false, false);

	if (SetFiltered(data->stage, EnterExitState::kStageExited))
	{
		EnableMouse(object, false, true);

		Stop(object, kentering);

		UnsetClip(object, kentering);

		SetOpacity(object, kentering, 0.0f, data->render);

		if (data->flags & kEnterAnimationSize)
		{
			SetBounds(object, kentering, {}, {});
		}

		if (detach)
		{
			object.Detach();
		}
		else if (data->flags & kEnterAnimationFade)
		{
			Detail::Hide(object);
		}
		//else
		//{
		//	EnableMouse(object, false, true);
		//}
	}
}

bool Reflex::GLX::HasEntered(const Object & object)
{
	if (auto data = object.QueryProperty<EnterExitState>(kentering))
	{
		return data->stage == EnterExitState::kStageEntered;
	}

	return false;
}
