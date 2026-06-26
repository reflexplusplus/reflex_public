#include "../../../../include/reflex/glx/functions/layouts.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

//struct ClipFrame : public GLX::Object
//{
//	ClipFrame(UInt8 flags);
//
//	~ClipFrame();
//
//	virtual void OnSetLayoutX(Reflex::Object & data, LayoutFn & accommodate, LayoutFn & align);
//
//	static void OnAccommodate(ClipFrame & self, GLX::Detail::LayoutData & layoutdata);
//
//	static void OnAlign(ClipFrame & self, GLX::Detail::LayoutData & layoutdata);
//
//	template <bool ORIGIN, bool SIZE, bool Y, bool MULTI> static void Align(ClipFrame & self, GLX::Detail::LayoutData & layoutdata);
//
//	REFLEX_TBINDER_4P(Align);
//
//
//	UInt8 m_flags, m_currentflags;
//
//	Pair <LayoutFn> m_standard;
//};

//struct ZoomFit : public GLX::Object
//{
//	ZoomFit();
//
//	~ZoomFit();
//
//	//virtual void DoneSetLayout(LayoutFn & accommodate, LayoutFn & align);
//
//	static void OnAccommodate(ZoomFit & self, Core::LayoutInfo & layoutdata);
//
//	static void OnAlign(ZoomFit & self, Core::LayoutInfo & data);
//
//
//	WeakReference m_content;
//
//	Pair <LayoutFn> m_standard;
//};

//ClipFrame::ClipFrame(UInt8 flags)
//	: m_flags(flags & 3)
//{
//}
//
//ClipFrame::~ClipFrame()
//{
//	for (auto & i : *this) UnsetBounds(i, kID);
//}
//
//void ClipFrame::DoneSetLayout(LayoutFn & accommodate, LayoutFn & align)
//{
//	m_standard = Detail::GetStandardLayout(ApplyStyle());
//
//	GLX_LEGACY_SetAccommodateFn(reinterpret_cast<LayoutFn>(&ClipFrame::OnAccommodate));
//
//	GLX_LEGACY_SetAlignFn(reinterpret_cast<LayoutFn>(&ClipFrame::OnAlign));
//}
//
//void ClipFrame::OnAccommodate(ClipFrame & self, Detail::LayoutData & layoutdata)
//{
//	self.m_standard.a(self, layoutdata);
//
//	bool y = GLX::GetAxis(self);
//
//	bool multi = self.GetNumItem() != 1;
//
//	self.m_currentflags = self.m_flags | y << 2 | multi << 3;
//
//	//self.GLX_LEGACY_SetAlignFn(reinterpret_cast<LayoutFn>(OnAlignBinder::Bind(flags)));
//}
//
//void ClipFrame::OnAlign(ClipFrame & self, Detail::LayoutData & data)
//{
//	AlignBinder::Bind(self.m_currentflags)(self, data);
//}
//
//template <bool ORIGIN, bool SIZE, bool Y, bool MULTI> void ClipFrame::Align(ClipFrame & self, GLX::Detail::LayoutData & layoutdata)
//{
//	REFLEX_USE(Detail);
//
//	auto & padding = layoutdata.cstyle->GetPadding();
//
//	auto & first = *self.GetFirst();
//
//	if (SIZE)
//	{
//		auto max = Indent(self.GetRect().size, padding).size;
//
//		ConditionalType<Y,Detail::XAxis,Detail::YAxis>::SetSize(max, kLarge.w);
//
//		if (MULTI)
//		{
//			for (auto & i : self) SetBounds(i, kID, kSmall, max);
//		}
//		else
//		{
//			SetBounds(first, kID, kSmall, max);
//		}
//
//		self.m_standard.b(self, layoutdata);
//	}
//
//	self.m_standard.b(self, layoutdata);
//
//	if (ORIGIN)
//	{
//		auto & origin = padding.near;
//
//		if (MULTI)
//		{
//			for (auto & i : self)
//			{
//				auto min = Reinterpret<Point>(origin + i.ComputeLayout().cstyle->GetMargin().near);
//
//				i.SetPosition(Max(i.GetRect().point, min));
//			}
//		}
//		else
//		{
//			auto min = Reinterpret<Point>(origin + first.ComputeLayout().cstyle->GetMargin().near);
//
//			first.SetPosition(Max(first.GetRect().point, min));
//		}
//	}
//}

//ZoomFit::ZoomFit()
//{
//}
//
//ZoomFit::~ZoomFit()
//{
//	m_content->UnsetMagnification(kID);
//}
//
//void ZoomFit::DoneSetLayout(LayoutFn & accommodate, LayoutFn & align)
//{
//	m_standard = Detail::GetStandardLayout(ApplyStyle());
//
//	GLX_LEGACY_SetAccommodateFn(reinterpret_cast<LayoutFn>(&ZoomFit::OnAccommodate));
//
//	GLX_LEGACY_SetAlignFn(reinterpret_cast<LayoutFn>(&ZoomFit::OnAlign));
//}
//
//void ZoomFit::OnAccommodate(ZoomFit & self, Core::LayoutInfo & data)
//{
//	bool y = GetAxis(self);
//
//	self.m_content->UnsetMagnification(kID);
//
//	if (auto first = self.GetFirst())
//	{
//		auto & content = *first;
//
//		self.m_content = content;
//
//		data.contentsize = content.ComputeLayout().contentsize;
//
//		//self.GLX_LEGACY_SetAlignFn(reinterpret_cast<LayoutFn>(OnAlignBinder::Bind(y)));
//	}
//	else
//	{
//		self.m_content->Clear();
//
//		data.contentsize = kSmall;
//
//		//self.GLX_LEGACY_SetAlignFn(self.m_standard.b);
//	}
//
//	Detail::AccommodateRenderer(self, data);
//
//	data.isresponsive = true;
//}
//
//void ZoomFit::OnAlign(ZoomFit & self, GLX::Core::LayoutInfo & data)
//{
//	if (self.m_content)
//	{
//		bool y = GetAxis(self);
//
//		auto & content = *self.m_content;
//
//		auto & size = self.GetRect().size;
//
//		content.UnsetMagnification(kID);
//
//		auto & content_contentsize = content.ComputeLayout().contentsize;
//
//		if (auto axissize = Detail::GetSize(y, size))
//		{
//			Float scale = Reflex::Max(axissize / Detail::GetSize(y, content_contentsize), 0.0009765625f);
//
//			if (scale < (1.0f - Core::kRoundingTolerance)) SetMagnification(content, kID, scale);	//FUCK XCODE
//		}
//	}
//
//	self.m_standard.b(self, data);
//}

REFLEX_END_INTERNAL




//
//public

//Reflex::GLX::Object & Reflex::GLX::CreateZoomFrame()
//{
//	return *REFLEX_CREATE(ZoomFit);
//}
