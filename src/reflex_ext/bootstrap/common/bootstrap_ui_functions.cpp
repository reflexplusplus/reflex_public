#include "reflex_ext/bootstrap/common/ui/functions.h"




REFLEX_NS(Reflex::Bootstrap)
using CreateOptionsFn = FunctionPointer <Unretained<Data::PropertySet>(const GLX::Object & client)>;
REFLEX_END

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

//void RefreshChildStyles(GLX::StyleSheet & stylesheet, GLX::Object & node)
//{
//	for (auto & child, node)
//	{
//		auto child_style = child.GetStyle();
//
//		if (auto t = DynamicCast<GLX::StyleSheet>(child_style))
//		{
//			File::ResourcePool::Lock lock(global->resourcepool);
//
//			if (auto reloaded = lock.Query(MakeAddress<GLX::StyleSheet>(t->path)))
//			{
//				auto sub_stylesheet = Cast<GLX::StyleSheet>(reloaded->object);
//
//				child.SetStyle(sub_stylesheet);
//
//				RefreshChildStyles(sub_stylesheet, child);
//			}
//		}
//		else if (IsValid(child_style))
//		{
//			auto style_itr = child_style.Adr();
//
//			Array <Key32> path;
//
//			while (style_itr)
//			{
//				if (auto t = DynamicCast<GLX::StyleSheet>(*style_itr))
//				{
//					break;
//				}
//
//				path.Push(style_itr->id);
//
//				style_itr = style_itr->GetParent();
//			}
//
//			ConstAlreadyRetained <GLX::Style> style = stylesheet;
//
//			for (auto & id : ReverseIterate(path))
//			{
//				if (auto pchild_style = style->QuerySubStyle(id))
//				{
//					style = pchild_style;
//				}
//				else
//				{
//					style = {};
//
//					break;
//				}
//			}
//
//			REFLEX_ASSERT(path.GetFirst() == child_style->id);
//
//			child.SetStyle(style);
//		}
//
//		RefreshChildStyles(stylesheet, child);
//	}
//}

ConstAlreadyRetained <GLX::StyleSheet> ApplyStyleSheet(GLX::Object & view, const WString::View & path, ArrayView <Key32> substyle, CreateOptionsFn create_options)
{
	auto stylesheet = GLX::RetrieveStyleSheet(path, AutoRelease(create_options(view)));

	ConstAlreadyRetained <GLX::Style> style = stylesheet;

	for (auto & id : substyle) style = style[id];

	view.SetStyle(style);

	return stylesheet;
}

REFLEX_END_INTERNAL

Reflex::GLX::Rect Reflex::Bootstrap::Detail::ConstrainRectToDisplay(const GLX::Rect & rect, GLX::Size min)
{
	GLX::Point near, far;

	for (auto & irect : System::GetScreens())
	{
		auto from = GLX::Detail::ToFloat(irect.origin);

		auto to = from + GLX::Detail::ToFloat(Reinterpret<System::iPoint>(irect.size));

		near = Min(near, from);

		far = Max(far, to);
	}

	near.y += 32.0f;

	auto size = Reinterpret<GLX::Size>(far - near);

	GLX::Rect available = { near, size };

	auto rtn = GLX::Detail::ConstrainRect(available, rect);

	rtn.size = Max(Min(rtn.size, size), min);

	return rtn;
}

void Reflex::Bootstrap::Detail::SetStyle(GLX::Object & view, const WString::View & path, const ArrayView <Key32> & substyle, CreateOptionsFn create_options)
{
	auto stylesheet = ApplyStyleSheet(view, path, substyle, create_options);

	auto resourcepool = GLX::Core::desktop->resourcepool;

	Key32 uid = path;

	for (auto id : substyle) Reflex::Detail::IncrementHash(Reinterpret<UInt32>(uid), id.value);	//TODO workaround solution, correct solution is one instance per-sheet and multiple listeners

	auto monitor = IDE::ResourceGroup::Create(resourcepool, uid, File::SplitFilename(path).b, [&view, path = WString(path), substyle = Array<Key32>(substyle), create_options](IDE::ResourceGroup & self)
	{
		GLX::Core::Context context;

		view.SetStyle(GLX::Style::null);

		auto stylesheet = ApplyStyleSheet(view, path, substyle, create_options);

		//RefreshChildStyles(stylesheet, view);

		IDE::AddStyleSheet(self, stylesheet);
	});

	SetAbstractProperty(view, K32("AutoRefresh"), monitor);

	IDE::AddStyleSheet(monitor, stylesheet);
}

Reflex::Unretained <Reflex::Data::PropertySet> Reflex::Bootstrap::Detail::CreateStylesheetOptions(bool dark_theme, Float font_scale, System::iSize screen_size, bool resizable)
{
	constexpr Key32 kmobile = K32("mobile");

	constexpr Key32 kOperatingSystems[System::kNumPlatform] = { K32("windows"), K32("macos"), K32("linux"), K32("android"), K32("ios"), K32("webasm") };
	const Key32 kEnvironments[System::kNumEnvironmentType] = { kNullKey, kNullKey, GLX::kIsMobile ? kmobile : K32("desktop"), kmobile, K32("plugin") };

	auto options = New<Data::PropertySet>();

	Data::SetKey32(options, K32("platform"), kOperatingSystems[System::kPlatform]);
	Data::SetKey32(options, K32("environment"), kEnvironments[System::kEnvironmentType]);
	Data::SetKey32(options, K32("plugin_format"), System::AudioPlugin::GetFormat());

	Data::SetInt32(options, K32("screen_width"), screen_size.w);
	Data::SetInt32(options, K32("screen_height"), screen_size.h);

	Data::SetKey32(options, K32("theme"), dark_theme ? K32("dark") : K32("light"));
	Data::SetFloat32(options, K32("font_scale"), font_scale);

	Data::SetKey32(options, GLX::kresizable, Reflex::Detail::kFalseTrue[resizable]);

	return options;
}
