#include "library.h"

#include "reflex/glx/functions/drawing.h"

#include "animation/player.h"
#include "style/detail/bitmap.h"

#include "../data/formats/propertysheet/standardformatimpl.h"




//
//

Reflex::Detail::Module Reflex::GLX::module("GLX");	//no dependencies, initalised by Start, use as parent of UI nulls

REFLEX_BEGIN_INTERNAL(Reflex::GLX)
#if REFLEX_DEBUG
constexpr CString::View kEventIDs[] =
{
	"WindowResize",
	"CloseRequest",
	"PointerDown",
	"PointerDrag",
	"PointerUp",
	"MouseEnter",
	"MouseLeave",
	"MouseDown",
	"MouseDrag",
	"MouseUp",
	"MouseWheel",
	"Focus",
	"LoseFocus",
	"KeyDown",
	"KeyUp",
	"Character",
	"DragOver",
	"DragEnter",
	"DragLeave",
	"DragDrop",
	"DropExternal",
	"Transaction",

	"LongTapGesture",
	"PanGesture",
	"SwipeGesture"
};
#endif
REFLEX_END_INTERNAL

REFLEX_NS(Reflex::GLX::Detail)

UIntNative WakeLayers();

REFLEX_END

Reflex::TypeID Reflex::GLX::g_array_cstring_property_typeid;
Reflex::TypeID Reflex::GLX::g_cstring_property_typeid;
Reflex::TypeID Reflex::GLX::g_key32_property_typeid;
Reflex::TypeID Reflex::GLX::g_resource_desc_typeid;
Reflex::TypeID Reflex::GLX::g_font_typeid;
Reflex::TypeID Reflex::GLX::g_imageset_typeid;
Reflex::TypeID Reflex::GLX::g_layer_desc_typeid;
Reflex::TypeID Reflex::GLX::g_array_of_layer_desc_typeid;

Reflex::UInt8 Reflex::GLX::kCreateRendererFlags;

const Reflex::Data::DecompressionAlgorithm * Reflex::GLX::gLZO = 0;

Reflex::GLX::Library::Library(File::ResourcePool & resourcepool, const System::Renderer::Config & config)
	: Core::DesktopImpl(resourcepool, config),
	m_allocator(CreateAllocator(kRecycleAllocID, Reflex::Object::null)),
	m_onchangedisplays(System::CreateListener(System::kNotificationChangeDisplays, 0, [](void * self)
	{
		Restart();
	})),
	m_onclock(System::CreateListener(System::kNotificationClock, 0, [](void * pself)
	{
		if (!g_library->m_onclock_filter++ && g_library->m_resourcepool_monitor.Poll())
		{
			auto & cache = g_library->cache.fonts;

			REFLEX_RLOOP(idx, cache.GetSize())
			{
				auto & item = cache.GetData()[idx];

				if (item->value->GetRetainCount() == 1)
				{
					Core::Context context;

					cache.Unset(item->key);
				}
			}
		}
	})),
	m_resourcepool_monitor(resourcepool),
	m_animationscope(True(System::Common::GetValue(config, kAnimations, true))),
	m_pixeldensity(System::GetMaxPixelDensity()),
	m_fdpifactor(Float(m_pixeldensity)),
	m_pixelsize(1.0f / m_fdpifactor),
	m_system_theme({ System::IsDarkTheme(), System::GetFontScale() }),
	m_mobile(System::Common::GetValue(config, kEmulateMobile, System::kEnvironmentType == System::kEnvironmentTypeMobileApp)),
	m_onclock_filter(1),
	null_viewport_coordinates(AbstractViewPort::null),
	null_text(false),
	//m_resourcepool_notification(REFLEX_CREATE(File::ResourcePool::Listener, resourcepool, [](File::ResourcePoolNotification n, Address adr, const WString&)
	//{
	//	if (n && adr.type_id == REFLEX_TYPEID(Detail::FontFile))
	//	{
	//		Core::Context context;

	//		auto & cache = g_library->cache.fonts;

	//		REFLEX_RLOOP(idx, cache.GetSize())
	//		{
	//			if (cache[idx].value->GetRetainCount() == 1)
	//			{
	//				cache.Remove(idx);
	//			}
	//		}
	//	}
	//})),
	null_event(kNullKey),
	kStyleSheetFormat(Data::Detail::CreateCustomFormat(istylesheet, {})),
	m_restart_clock(0)
{
	REFLEX_ENABLE_FTZ();	//TEMP solution, this was because was previously set on main thread by SIMD.  should be pushed pop before render operations

	RemoveConst(Detail::LegacyWeakReferenceObject::kDynamicTypeInfo).tname = "{WeakRef}";

	m_config.Set(kAnimations, AnimationScope::IsEnabled());

	GLX::Core::Context ctx;

	m_renderer->EnableBlend(true);

	g_current_stylesheet = { {}, &StyleSheet::null, &Data::PropertySet::null };

	Retain(kStyleSheetFormat);


	null_stylesheet.Init(kNullKey);

	null_object.Init();

	null_window.Init();

	null_delegate.Init(kvalue);

	null_viewbar.Init();

	null_scroller.Init();

	null_zoomable.Init();

	null_zoomable->InvertAxis(false, true);

	null_scroller->GetBody().RemoveConst()->SetRect({ kOrigin, kLarge });	//HACK fix clipping in TextEdit when not in viewport

	SetRender(Cast<Detail::ComputedStyleImpl>(Detail::ComputedStyle::null), Detail::ComputedStyle::kRenderFalse);


	if (!m_renderer->Status())
	{
		WString::View title;

		if (System::kPlatform == System::kPlatformMacOS || System::kPlatform == System::kPlatformIOS)
		{
			title = L"Could not start Metal or OpenGL";
		}
		else
		{
			title = L"Could not start Direct3D 11 or OpenGL";
		}

		System::ShowMessageBox(0, title, { L"Please ensure your graphics drivers are up-to-date.", L"The latest drivers can be downloaded from your GPU manufacturer's website." }, 0);
	}

	for (auto & i : cache.point_workspaces) i.Allocate(1024);

	for (auto & i : cache.colourpoint_workspaces) i.Allocate(1024);


	Detail::kRenderModes[Detail::ComputedStyle::kRenderAuto].d = UInt8(m_pixeldensity);

	Detail::kRenderModes[Detail::ComputedStyle::kRenderTrue].d = UInt8(m_pixeldensity);


	GLX_GET_POINT_WORKSPACE(points);

	AddRectFill(points, { kOrigin, kNormal });

	Detail::g_solid_rectangle = CreateGraphic(points);

	Retain(Detail::g_solid_rectangle);

	m_canrendertotexture = True(System::Common::GetValue(m_config, System::Renderer::kTX, 0));

	if (m_canrendertotexture)
	{
		kCreateRendererFlags = kMaxUInt8;
	}
	else
	{
		kCreateRendererFlags = BitClear(kMaxUInt8, 7);
	}

	REFLEX_LOOP(format, System::kNumImageFormat) m_supportsimageformat[format] = m_renderer->SupportsImageFormat(System::ImageFormat(format));


	//g_startupaccess.Deinit();

	Detail::WakeLayers();

	for (auto & cls : Detail::Layer::Class::range)
	{
		REFLEX_ASSERT(!m_layerfactory.Search(cls.id));

		if (cls.enabled)
		{
			Key32 uid = cls.id;

			m_layerfactory.Insert(uid, &cls);

			cls.schema = cls.initschema(uid);

			Retain(cls.schema);
		}

#if REFLEX_DEBUG
		Data::RegisterKey(keymap, cls.id);
#endif
	}

	g_array_cstring_property_typeid = GetTypeID<Data::ArrayOfCStringProperty>();
	g_cstring_property_typeid = GetTypeID<Data::CStringProperty>();
	g_key32_property_typeid = GetTypeID<Data::Key32Property>();
	g_resource_desc_typeid = GetTypeID<Detail::ResourceDesc>();
	g_font_typeid = GetTypeID<Detail::Font>();
	g_imageset_typeid = GetTypeID<Detail::ImageSet>();
	g_layer_desc_typeid = GetTypeID<Detail::LayerDesc>();
	g_array_of_layer_desc_typeid = GetTypeID<Detail::ArrayOfLayerDesc>();

	Detail::InitialiseCStyleSetters(m_cstylesetters);

#if REFLEX_DEBUG
	for (auto & i : kEventIDs) Data::RegisterKey(keymap, i);
#endif

	null_frame.c = kNormal;


	//dependencies

	module.Init();
}

Reflex::GLX::Library::~Library()
{
	TheLibrary::st_initalised = true;

	Core::Context access;

	
	module.Deinit();		//deinit dependencies


	if (m_restart_clock)
	{
		Release(m_restart_clock);

		m_restart_clock = 0;
	}


	m_globalplayer.m_animations.Clear();

	m_mouseover.Clear();


	null_zoomable.Deinit();

	null_scroller.Deinit();

	null_viewbar.Deinit();

	null_window.Deinit();

	null_delegate.Deinit();

	null_object.Deinit();


#if REFLEX_DEBUG
	REFLEX_ASSERT(!GLX::Object::st_tokens);
	if (GLX::Object::st_tokens)
	{
		for (auto & i : GLX::Object::st_tokens)
		{
			auto & object = i.self;

			System::DebugLog(false, object.object_t->tname);
		}
	}
#endif

	StyleSheet & null = null_stylesheet;

	null.Clear();

	Cast<StyleAccessor>(Cast<Style>(null))->Clear();

	//fonts.Clear();
	cache = {};

	File::ResourcePool::Lock lock(Core::desktop->resourcepool);

	lock.Flush();

	if constexpr (REFLEX_DEBUG)
	{
		TRef <Reflex::Object> null_resources[] = { StyleSheet::null, System::Renderer::Canvas::null, Detail::ImageSet::null, Detail::Font::null };

		for (auto & i : null_resources)
		{
			lock.Enumerate(i->object_t->type_id, [i](const File::ResourcePool::Token & token)
			{
				if (token.object != i)
				{
					REFLEX_ASSERT(false);
				}
			});
		}
	}

	null_stylesheet.Deinit();


	//release graphics while valid context exists

	Release(Detail::g_solid_rectangle);

	Release(kStyleSheetFormat);

	TheLibrary::st_initalised = false;


	for (auto & cls : Detail::Layer::Class::range)
	{
		if (cls.enabled)
		{
			Release(cls.schema);
		}
	}
}

Reflex::TRef <Reflex::GLX::Library> Reflex::GLX::g_library = TheLibrary::Get<true>();

const Reflex::TRef <Reflex::GLX::Core::Desktop> Reflex::GLX::Core::desktop = kNoValue;

const bool & Reflex::GLX::Detail::kCanRenderToTexture = g_library->m_canrendertotexture;

const bool * const Reflex::GLX::Detail::kSupportsImageFormat = g_library->m_supportsimageformat;

const Reflex::Float32 & Reflex::GLX::Detail::kPixelSize = g_library->m_pixelsize;

Reflex::ConstTRef <Reflex::System::Renderer::Graphic> Reflex::GLX::Detail::g_solid_rectangle(kNoValue);




//
//nulls

#define REFLEX_GLX_NULL(TYPE, x) Reflex::GLX::TYPE & Reflex::GLX::TYPE::null = g_library->x

REFLEX_GLX_NULL(Core::Renderer, null_renderer);
REFLEX_GLX_NULL(Detail::ImageSet, null_image_set);
REFLEX_GLX_NULL(Detail::ComputedStyle, null_cstyle);
REFLEX_GLX_NULL(Text, null_text);
REFLEX_GLX_NULL(Detail::Font, null_font);
REFLEX_GLX_NULL(AbstractViewPort::ViewState, null_viewport_coordinates);
REFLEX_GLX_NULL(StyleSheet, null_stylesheet);
REFLEX_GLX_NULL(Style, null_stylesheet);

REFLEX_GLX_NULL(Object, null_object);
REFLEX_GLX_NULL(Object::Delegate, null_delegate);
REFLEX_GLX_NULL(TextEditBehaviour, null_delegate);
REFLEX_GLX_NULL(AbstractViewPort, null_scroller);
REFLEX_GLX_NULL(ScrollArea, null_scroller);
REFLEX_GLX_NULL(ZoomArea, null_zoomable);
REFLEX_GLX_NULL(WindowClient, null_window);
REFLEX_GLX_NULL(Animation, null_animation);
REFLEX_GLX_NULL(InterpolatedAnimation, null_animation);
REFLEX_GLX_NULL(ContainerAnimation, null_container_animation);
REFLEX_GLX_NULL(AbstractViewBar, null_viewbar);
REFLEX_GLX_NULL(Event, null_event);

Reflex::UInt Reflex::GLX::Detail::g_point_workspace_counter = 0;
Reflex::UInt Reflex::GLX::Detail::g_colourpoint_workspace_counter = 0;

const Reflex::ArrayRegion <Reflex::GLX::Points> Reflex::GLX::Detail::g_point_workspace(g_library->cache.point_workspaces, 4);
const Reflex::ArrayRegion <Reflex::GLX::ColourPoints> Reflex::GLX::Detail::g_colourpoint_workspace(g_library->cache.colourpoint_workspaces, 4);

Reflex::Data::Detail::PropertySheetInterface & Reflex::GLX::Detail::iresource = Reflex::GLX::g_library->iresource;

Reflex::Data::Detail::PropertySheetInterface & Reflex::GLX::Detail::istylesheet = Reflex::GLX::g_library->istylesheet;

const bool & Reflex::GLX::kIsMobile = Reflex::GLX::TheLibrary::Get<true>()->m_mobile;

const Reflex::Pair <bool,Reflex::Float> & Reflex::GLX::kSystemTheme = Reflex::GLX::TheLibrary::Get<true>()->m_system_theme;

Reflex::Reference <Reflex::Object> Reflex::GLX::Start(File::ResourcePool & resourcepool, const System::Renderer::Config & config)
{
	return TheLibrary::Acquire(resourcepool, config);
}

void Reflex::GLX::Restart()
{
	auto & pclock = g_library->m_restart_clock;

	if (module.IsInitalised() && System::App::list && !pclock)	//cant work without System::App to reopen
	{
		pclock = System::CreateListener(System::kNotificationClock, nullptr, [](void * client)
		{
			Array <System::App*> instances;

			auto pinstances = Extend(instances, System::App::list.GetNumItem()).data;

			for (auto & i : System::App::list)
			{
				i.CloseEditor();

				*pinstances++ = &i;
			}

			REFLEX_ASSERT(!module.IsInitalised());

			for (auto & i : instances) i->OpenEditor();

			REFLEX_ASSERT(!g_library->m_restart_clock);
		}).Adr();

		Retain(pclock);
	}
}

#if REFLEX_DEBUG
Reflex::Key32 Reflex::GLX::RegisterKey(CString::View key)
{
	REFLEX_ASSERT(module.IsInitalised());

	return Data::RegisterKey(g_library->keymap, key);
}

Reflex::CString::View Reflex::GLX::GetKey(Key32 id)
{
	REFLEX_ASSERT(module.IsInitalised());

	return Data::GetKey(g_library->keymap, id);
}

Reflex::ConstTRef <Reflex::Data::KeyMap> Reflex::GLX::GetKeyMap()
{
	REFLEX_ASSERT(module.IsInitalised());

	return g_library->keymap;
}
#endif
