#include "../../../../include/reflex/vm.h"

#define VM_WRAP_INTERNAL(NS) REFLEX_NS(Reflex::NS##VM) namespace {

#include "glx/global.h"

#include "glx/object.h"
#include "glx/data.h"
#include "glx/layout.h"

#include "glx/global.cpp"
#include "glx/geometry.cpp"
#include "glx/object.cpp"
#include "glx/event.cpp"
#include "glx/widgets.cpp"
#include "glx/bitmap.cpp"
#include "glx/animation.cpp"




//TODO

VM_WRAP_INTERNAL(GLX)

struct StyleImpl :
	public GLX::Style,
	public VM::Detail::Circular
{
	StyleImpl(VM::Context & context, VM::TypeRef type)
		: Style(kNullKey, 0),
		VM::Detail::Circular(context, *this)
	{
	}

	static void opGet(VM::Context & context)
	{
		VM_POP(GLX::Style&,Key32);

		VM_RTN(args.a[args.b]);
	};

	virtual void OnSetProperty(Address address, Reflex::Object & object) override
	{
		if (address.type_id == K32("Array@Float32"))
		{
			auto t = AutoRelease(object);

			auto array = Cast<VM::ValueArray>(object);

			SetProperty(address.id, REFLEX_CREATE(Data::ArrayOfFloat32Property, array->GetView<Float>()));
		}
		else if (address.type_id == REFLEX_TYPEID(Data::Float32Property))
		{
			auto t = AutoRelease(object);

			SetProperty(address.id, REFLEX_CREATE(Data::ArrayOfFloat32Property, std::initializer_list<Float>{ Cast<Data::Float32Property>(object)->value }));
		}
		else
		{
			GLX::Style::OnSetProperty(address, object);
		}
	}
};

REFLEX_END_INTERNAL

REFLEX_ASSERT_RAW(Reflex::System::ColourPoint);

const Reflex::VM::Module Reflex::GLXVM::gGLX("GLX", { VM::gDataPropertySet, gGeometry }, VM::kContextFlagUi, [](VM::Compiler::State & cstate, UInt8 flags, Reflex::Object &)
{
	REFLEX_USE(VM);


	auto bindings = cstate.bindings;

	bindings->SetProperty(K32("GLX"), Reflex::The<Library>::Acquire());


	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto uint8_t = bindings->uint8_t;

	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;

	auto string_t = bindings->string_t;


	auto point_t = VM::GetType<GLX::Point>(bindings);

	auto fsize_t = VM::GetType<GLX::Size>(bindings);

	auto rect_t = VM::GetType<GLX::Rect>(bindings);


	auto dynamic_t = VM::GetType<Data::PropertySet>(bindings);


	VM_BEGIN_ENUM(cstate, kGLX, "KeyCode")
	BIND_ENUM(kKeyCodeNull),
	BIND_ENUM(kKeyCodeF1),
	BIND_ENUM(kKeyCodeF2),
	BIND_ENUM(kKeyCodeF3),
	BIND_ENUM(kKeyCodeF4),
	BIND_ENUM(kKeyCodeF5),
	BIND_ENUM(kKeyCodeF6),
	BIND_ENUM(kKeyCodeF7),
	BIND_ENUM(kKeyCodeF8),
	BIND_ENUM(kKeyCodeF9),
	BIND_ENUM(kKeyCodeF10),
	BIND_ENUM(kKeyCodeF11),
	BIND_ENUM(kKeyCodeF12),
	BIND_ENUM(kKeyCodeTab),
	BIND_ENUM(kKeyCodeEnter),
	BIND_ENUM(kKeyCodeEscape),
	BIND_ENUM(kKeyCodeSpace),
	BIND_ENUM(kKeyCodeBackspace),
	BIND_ENUM(kKeyCodeInsert),
	BIND_ENUM(kKeyCodeDelete),
	BIND_ENUM(kKeyCodeHome),
	BIND_ENUM(kKeyCodeEnd),
	BIND_ENUM(kKeyCodePageUp),
	BIND_ENUM(kKeyCodePageDown),
	BIND_ENUM(kKeyCodeUp),
	BIND_ENUM(kKeyCodeDown),
	BIND_ENUM(kKeyCodeLeft),
	BIND_ENUM(kKeyCodeRight),
	BIND_ENUM(kKeyCodeNumericDivide),
	BIND_ENUM(kKeyCodeNumericMultiply),
	BIND_ENUM(kKeyCodeNumericMinus),
	BIND_ENUM(kKeyCodeNumericPlus),
	BIND_ENUM(kKeyCode1),
	BIND_ENUM(kKeyCode2),
	BIND_ENUM(kKeyCode3),
	BIND_ENUM(kKeyCode4),
	BIND_ENUM(kKeyCode5),
	BIND_ENUM(kKeyCode6),
	BIND_ENUM(kKeyCode7),
	BIND_ENUM(kKeyCode8),
	BIND_ENUM(kKeyCode9),
	BIND_ENUM(kKeyCode0),
	BIND_ENUM(kKeyCodeMinus),
	BIND_ENUM(kKeyCodePlus),
	BIND_ENUM(kKeyCodeA),
	BIND_ENUM(kKeyCodeB),
	BIND_ENUM(kKeyCodeC),
	BIND_ENUM(kKeyCodeD),
	BIND_ENUM(kKeyCodeE),
	BIND_ENUM(kKeyCodeF),
	BIND_ENUM(kKeyCodeG),
	BIND_ENUM(kKeyCodeH),
	BIND_ENUM(kKeyCodeI),
	BIND_ENUM(kKeyCodeJ),
	BIND_ENUM(kKeyCodeK),
	BIND_ENUM(kKeyCodeL),
	BIND_ENUM(kKeyCodeM),
	BIND_ENUM(kKeyCodeN),
	BIND_ENUM(kKeyCodeO),
	BIND_ENUM(kKeyCodeP),
	BIND_ENUM(kKeyCodeQ),
	BIND_ENUM(kKeyCodeR),
	BIND_ENUM(kKeyCodeS),
	BIND_ENUM(kKeyCodeT),
	BIND_ENUM(kKeyCodeU),
	BIND_ENUM(kKeyCodeV),
	BIND_ENUM(kKeyCodeW),
	BIND_ENUM(kKeyCodeX),
	BIND_ENUM(kKeyCodeY),
	BIND_ENUM(kKeyCodeZ),
	BIND_ENUM(kKeyCodeBracketOpen),
	BIND_ENUM(kKeyCodeBracketClose),
	VM_END_ENUM
		
	//VM_BEGIN_ENUM(cstate, kGLX, "MouseCapture")
	//VM_BIND_ENUM(GLX::kTrapThru, "kMouseCaptureNone"),
	//VM_BIND_ENUM(GLX::kTrapPassive, "kMouseCaptureClick"),
	//VM_BIND_ENUM(GLX::kTrapActive, "kMouseCaptureDrag"),
	//VM_BIND_ENUM(GLX::kTrapActiveIncremental, "kMouseCaptureIncremental")
	//VM_END_ENUM;

	VM_BEGIN_ENUM(cstate, kGLX, "MouseCursor")
	BIND_ENUM(kMouseCursorInvisible),
	BIND_ENUM(kMouseCursorArrow),
	BIND_ENUM(kMouseCursorWait),
	BIND_ENUM(kMouseCursorMove),
	BIND_ENUM(kMouseCursorTopBottom),
	BIND_ENUM(kMouseCursorLeftRight),
	BIND_ENUM(kMouseCursorTopLeftBottomRight),
	BIND_ENUM(kMouseCursorBottomLeftTopRight),
	BIND_ENUM(kMouseCursorPointer),
	BIND_ENUM(kMouseCursorDrag),
	BIND_ENUM(kMouseCursorText),
	BIND_ENUM(kMouseCursorBlock),
	BIND_ENUM(kMouseCursorZoom),
	VM_END_ENUM;

	VM_BEGIN_ENUM(cstate, kGLX, "FlowFlags")
	BIND_ENUM(kFlowX),
	BIND_ENUM(kFlowY),
	BIND_ENUM(kFlowInvert),
	BIND_ENUM(kFlowCenter),
	VM_END_ENUM;

	VM_BEGIN_ENUM(cstate, kGLX, "Alignment")
	BIND_ENUM(kAlignmentTopLeft),
	BIND_ENUM(kAlignmentTop),
	BIND_ENUM(kAlignmentTopRight),
	BIND_ENUM(kAlignmentLeft),
	BIND_ENUM(kAlignmentCenter),
	BIND_ENUM(kAlignmentRight),
	BIND_ENUM(kAlignmentBottomLeft),
	BIND_ENUM(kAlignmentBottom),
	BIND_ENUM(kAlignmentBottomRight),
	VM_END_ENUM;

	//VM_BEGIN_ENUM(cstate, kGLX, "Positioning")
	//BIND_ENUM(kPositioningInline)
	//BIND_ENUM(kPositioningFloat)
	//BIND_ENUM(kPositioningAbsolute)
	//VM_END_ENUM;

	VM_BEGIN_ENUM(cstate, kGLX, "Orientation")
	BIND_ENUM(kOrientationNear),
	BIND_ENUM(kOrientationCenter),
	BIND_ENUM(kOrientationFar),
	BIND_ENUM(kOrientationFit),
	VM_END_ENUM;

	//VM_BEGIN_ENUM(cstate, kGLX, "EnterFlags")
	//BIND_ENUM(kEnterAnimationFade),
	//BIND_ENUM(kEnterAnimationSize),
	//VM_END_ENUM;

	VM_BIND_ENUM(cstate, GLX, kPointerFlagRightMouseButton);
	VM_BIND_ENUM(cstate, GLX, kPointerFlagDouble);
	VM_BIND_ENUM(cstate, GLX, kPointerFlagMulti);
	VM_BIND_ENUM(cstate, GLX, kClickFlagRmb);
	VM_BIND_ENUM(cstate, GLX, kClickFlagDbl);

	auto array_string_t = cstate.InstantiateTemplateType(kArray, { string_t });

	AddFunction(bindings, kGLX, "ShowFileDialog", string_t, { bool_t, array_string_t, string_t, string_t }, [](Context & context)
	{
		typedef VM::String String;

		VM_POP(bool,ArrayOfNonCircularObjects&,String&,String&);

		Array <WString> strings;

		for (auto & i : args.b.GetRegion<String>()) strings.Push(i->GetView());

		if (auto path = GLX::ShowFileDialog(args.a, strings, args.c.GetView(), args.d.GetView()))
		{
			VM_RTN(New<String>(path));
		}
		else
		{
			VM_RTN(&String::null);
		}
	});

	AddFunction(bindings, kGLX, "ShowFolderDialog", string_t, { bool_t, string_t, string_t }, [](Context & context)
	{
		typedef VM::String String;

		VM_POP(bool,String&,String&);

		if (auto path = GLX::ShowFolderDialog(args.a, args.b.GetView(), args.c.GetView()))
		{
			VM_RTN(New<String>(path));
		}
		else
		{
			VM_RTN(&String::null);
		}
	});


	//for style/cstyle

	auto pair_size_t = cstate.InstantiateTemplateType(VM::kTuple, { fsize_t, fsize_t });


	auto style_t = RegisterObject<GLX::Style>(bindings, kGLX, "Style");

	style_t->members.Push(VM_BIND_MEMBER(GLX::Style, id, cstate, key32_t));

	auto stylesheet_t = RegisterObject<GLX::StyleSheet>(bindings, kGLX, "StyleSheet");


	cstate.RegisterResourceType(K32("stylesheet"), stylesheet_t, &GLX::Detail::DecodeStyleSheet, L"glx");


	VM::Detail::SetTypeCtr(style_t, {}, [](VM_CTR_PARAMS) -> Reflex::Object &
	{
		return *REFLEX_CREATE(StyleImpl, context, type);
	});

	AddConstructor(bindings, style_t, { dynamic_t }, Data::Pack(style_t), [](Context & context)
	{
		auto & stack = context.stack;

		auto & params = VM::Detail::Pop<Data::PropertySet&>(stack);

		auto style = REFLEX_CREATE(StyleImpl, context, ReadFunctionData<TypeRef>(context));

		for (auto & i : params.Iterate()) style->OnSetProperty(i.key, Copy(i.value));

		VM_RTN(style);
	});



	GLXVM::BindIterable<GLX::Style>(cstate, kGLX, style_t);

	AddMethod(bindings, "Refresh", void_t, {style_t}, [](Context & context)
	{
		VM_POP1(GLX::Style&);

		arg.UnsetProperty<GLX::Detail::ComputedStyle>(GLX::Detail::kComputedStyle);
	});

	AddMethod(bindings, "ExtractProperty", bool_t, { style_t, key32_t, bindings->object_t }, [](Context & context)
	{
		VM_POP(GLX::Style&,UInt32,Object&);

		VM_RTN(ExtractProperty(args.a, args.b, args.c));
	});

	AddMethod(bindings, Compiler::opGet, style_t, { style_t, key32_t }, { style_t }, {}, &StyleImpl::opGet);

	AddMethod(bindings, Compiler::opGet, style_t, { style_t, key32_t }, &StyleImpl::opGet);



	//object / layout

	auto object_t = BindObject(cstate, kGLX, point_t, fsize_t, rect_t, style_t);

	AddMethod(bindings, "AddInline", void_t, {object_t, object_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*);

		if (args.a != args.b) GLX::AddInline(*args.a, args.b);
	});

	AddMethod(bindings, "AddInline", void_t, {object_t, object_t, uint8_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*,UInt8);

		if (args.a != args.b) GLX::AddInline(*args.a, args.b, GLX::Orientation(args.c & 3));
	});

	AddMethod(bindings, "AddInlineFlex", void_t, {object_t, object_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*);

		if (args.a != args.b) GLX::AddInlineFlex(*args.a, args.b);
	});

	AddMethod(bindings, "AddInlineFlex", void_t, {object_t, object_t, uint8_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*,UInt8);

		if (args.a != args.b) GLX::AddInlineFlex(*args.a, args.b, GLX::Orientation(args.c & 3));
	});

	AddMethod(bindings, "AddFloat", void_t, {object_t, object_t, uint8_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*,UInt8);

		if (args.a != args.b) GLX::AddFloat(*args.a, args.b, GLX::Alignment(args.c % GLX::kNumAlignment));
	});

	AddMethod(bindings, "AddFloat", void_t, {object_t, object_t, uint8_t, uint8_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*,UInt16);

		args.c &= 771;

		if (args.a != args.b) GLX::AddFloat(*args.a, args.b, Reinterpret<GLX::Orientation>(args.c), Reinterpret<GLX::Orientation>(&args.c)[1]);
	});

	AddMethod(bindings, "AddAbsolute", void_t, {object_t, object_t, point_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*,GLX::Point);

		if (args.a != args.b) GLX::AddAbsolute(*args.a, args.b, args.c);
	});

	AddFunction(bindings, kGLX, "AddStretch", void_t, {object_t, object_t}, [](Context & context)
	{
		VM_POP(GLX::Object*,GLX::Object*);

		if (args.a != args.b) GLX::AddStretch(*args.a, args.b);
	});

	
	AddFunction(bindings, kGLX, "QueryElementById", object_t, { object_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		VM_RTN(GLX::QueryElementById(args.a, args.b, &GLX::Object::null));
	});

	AddFunction(bindings, kGLX, "QueryChildById", object_t, { object_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		VM_RTN(GLX::QueryChildById(args.a, args.b, &GLX::Object::null));
	});

	AddFunction(bindings, kGLX, "BranchContains", bool_t, { object_t, object_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,GLX::Object&);

		VM_RTN(BranchContains(args.a, args.b));
	});

	AddFunction(bindings, kGLX, "SetText", void_t, { object_t, string_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,VM::String&);

		GLX::SetText(args.a, args.b.GetView());
	});

	AddMethod(bindings, "StartDragDrop", void_t, { bindings->object_t, uint8_t, uint8_t }, [](Context & context)
	{
		VM_POP(Reflex::Object&,UInt8,UInt8);

		auto & data = args.a;
		auto dragover = Reflex::System::MouseCursor(args.b);
		auto block = Reflex::System::MouseCursor(args.c);

		auto object_t = data.object_t;

		auto vmtype = object_t->type_id;

		for (auto & i : DragHandler::range)
		{
			if (vmtype == i.vmtype)
			{
				GLX::StartDragDrop(data, dragover, block);

				return;
			}
		}
	});

	AddMethod(bindings, "ComputeContentSize", fsize_t, { object_t }, [](Context & context)
	{
		GLX::AnimationScope scope(false);

		VM_RTN(GLX::Detail::ComputeContentSize(VM::Detail::Pop<GLX::Object&>(context.stack)));
	});


	AddFunction(bindings, kGLX, "UnsetBounds", void_t, { object_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		GLX::UnsetBounds(args.a, args.b);
	});

	AddFunction(bindings, kGLX, "SetBounds", void_t, { object_t, key32_t, fsize_t, fsize_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32,GLX::Size,GLX::Size);

		GLX::SetBounds(args.a, args.b, args.c, args.d);
	});

	AddFunction(bindings, kGLX, "GetBounds", pair_size_t, { object_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		VM_RTN(GLX::GetBounds(args.a, args.b));
	});

	AddFunction(bindings, kGLX, "UnsetOpacity", void_t, { object_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		GLX::UnsetOpacity(args.a, args.b);
	});

	AddFunction(bindings, kGLX, "SetOpacity", void_t, { object_t, key32_t, float32_t }, [](Context & context)
	{
		VM_POP(GLX::Object&,Key32,Float32);

		GLX::SetOpacity(args.a, args.b, args.c);
	});

	//AddFunction(bindings, kGLX, "UnsetMagnification", void_t, { object_t, key32_t }, [](Context & context)
	//{
	//	VM_POP(GLX::Object&,Key32);

	//	GLX::UnsetMagnification(args.a, args.b);
	//});

	//AddFunction(bindings, kGLX, "SetMagnification", void_t, { object_t, key32_t, float32_t }, [](Context & context)
	//{
	//	VM_POP(GLX::Object&,Key32,Float32);

	//	GLX::SetMagnification(args.a, args.b, args.c);
	//});
	


	typedef Pair <UInt8> PairBool;

	auto pair_bool_t = RegisterValue< Pair<UInt8> >(bindings, kGlobal, "Tuple@(UInt8,UInt8)");

	pair_bool_t->members = { VM_BIND_MEMBER(PairBool,a,cstate,bool_t), VM_BIND_MEMBER(PairBool,b,cstate,bool_t) };

	cstate.InstantiateTemplateType( kTuple, {bool_t, bool_t});


	auto objectof_boolpair_t = RegisterObject< ObjectOf<PairBool> >(bindings, kGlobal, "ObjectOf@Tuple@(UInt8,UInt8)");

	objectof_boolpair_t->members.Push({ GLX::kvalue, VM::MakeMember(pair_bool_t, REFLEX_OFFSETOF(ObjectOf<PairBool>,value), false) });



	//msg

	cstate.InstantiateTemplateType(kFn, { object_t, string_t });

	BindEvent(cstate, kGLX, dynamic_t, object_t, point_t);

	BindAnimation(cstate, kGLX, dynamic_t, object_t);

	BindData(cstate, kGLX, dynamic_t, fsize_t, rect_t);



	//widgets

	auto viewport_t = RegisterObject<GLX::AbstractViewPort>(bindings, kGLX, "AbstractViewPort");

	auto scroller_t = RegisterObject<GLX::ScrollArea>(bindings, kGLX, "ScrollArea");

	auto zoomable_t = RegisterObject<GLX::ZoomArea>(bindings, kGLX, "ZoomArea");

	AddConstructor(bindings, zoomable_t, {}, {}, [](Context & context)
	{
		VM_RTN(REFLEX_CREATE(WidgetOf<GLX::ZoomArea>, context));
	});

	AddConstructor(bindings, scroller_t, {}, {}, [](Context & context)
	{
		VM_RTN(REFLEX_CREATE(WidgetOf<GLX::ScrollArea>, context));
	});

	AddMethod(bindings, "SetContent", void_t, { viewport_t, object_t }, [](Context & context)
	{
		VM_POP(GLX::AbstractViewPort&,GLX::Object&);

		args.a.SetContent(args.b);
	});

	AddMethod(bindings, "GetContent", object_t, { viewport_t }, [](Context & context)
	{
		VM_POP1(GLX::AbstractViewPort&);

		VM_RTN(arg.GetContent().Adr());
	});

	AddMethod(bindings, "SetView", void_t, { viewport_t, VM::ByRef(rect_t) }, [](Context & context)
	{
		VM_POP(GLX::AbstractViewPort&,GLX::Rect&);

		args.a.SetView(args.b);
	});

	AddMethod(bindings, "GetView", rect_t, { viewport_t }, [](Context & context)
	{
		auto & args = VM::Detail::Pop<GLX::AbstractViewPort&>(context.stack);

		VM_RTN(args.GetView());
	});

	AddMethod(bindings, "InvertAxis", void_t, { zoomable_t, bool_t, bool_t }, [](Context & context)
	{
		VM_POP(GLX::ZoomArea&,bool,bool);

		args.a.InvertAxis(args.b, args.c);
	});

	//AddFunction(bindings, kGLX, "SyncViewPorts", void_t, { viewport_t, viewport_t, bool_t, bool_t }, [](Context & context)
	//{
	//	VM_POP_SAFE(GLX::AbstractViewPort&,GLX::AbstractViewPort&,bool,bool);

	//	GLX::SyncViewports(args.a, args.b, args.c, args.d);
	//});



	AddFunction(bindings, kGLX, "RetrieveStyleSheet", style_t, { string_t }, [](Context & context)
	{
		VM_POP1(VM::String&);

		auto & base = context.program->sources[context.instructionptr->file].pathview;

		auto sheet = GLX::RetrieveStyleSheet(File::ResolveIncludePath(File::SplitFilename(base).a, arg.GetView()));

		sheet.RemoveConst()->SetProperty(kNullKey, REFLEX_CREATE(StyleSheetCircular, context, sheet.RemoveConst()));

		VM_RTN(sheet);
	});

	//AddFunction(bindings, kGLX, "CreateObject", object_t, { key32_t }, [](Context & context)
	//{
	//	auto object = CreateObject(context, VM::Detail::Pop<Key32>(context.stack));

	//	REFLEX_ASSERT(object->GetContextID() == context.GetContextID());

	//	VM_RTN(object);
	//});



	AddFunction(bindings, kGLX, "Init", object_t, { object_t, style_t }, [](Context & context)
	{
		VM_POP_SAFE(GLX::Object&,GLX::Style&);

		args.a.SetStyle(args.b);

		VM_RTN(args.a);
	});

	AddFunction(bindings, kGLX, "Init", object_t, { object_t, style_t, string_t }, [](Context & context)
	{
		VM_POP_SAFE(GLX::Object&,GLX::Style&,VM::String&);

		args.a.SetStyle(args.b);

		GLX::SetText(args.a, args.c.GetView());

		VM_RTN(args.a);
	});
});
