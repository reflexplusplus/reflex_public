#include "widget.h"




//
//

using namespace Reflex;

REFLEX_BEGIN_INTERNAL(CustomDrawing)

GLX::Colour Mul(const GLX::Colour & c, Float x)
{
	return { c.r * x, c.g * x, c.b * x, c.a * x };
}

void MakeHexagon(GLX::Point output[6], GLX::Size size)
{
	auto cx = size.w * 0.5f;
	auto cy = size.h * 0.5f;

	auto r = Min(size.w, size.h) * 0.5f;

	auto start = -kPif * 0.5f;

	REFLEX_LOOP(idx, 6)
	{
		auto a = start + k2Pif * Float(idx) / 6.0f;

		output[idx] = { cx + Cos(a) * r, cy + Sin(a) * r };
	}
}

struct WidgetImpl : public GLX::Object
{
	WidgetImpl();

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;


	Float32 m_value = 0.0f;
};

WidgetImpl::WidgetImpl()
{
	GLX::EnableMouseCapture(*this, true, true);

	Update();
}

bool WidgetImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		Data::SetFloat32(*this, "init", m_value);

		return true;
	}
	else if (e.id == GLX::kMouseDrag)
	{
		auto delta = GLX::GetMouseDelta(e);

		auto value = Data::GetFloat32(*this, "init");

		m_value = Clip(value - (delta.y / 256.0f), -1.0f, 1.0f);

		//Redraw();	//would be sufficient to retrigger the OnDraw callback

		Update();	//but.. here im using Update instead (which will also schedule Redraw) as doing SetText in OnUpdate

		return true;
	}
	else if (e.id == GLX::kMouseUp)
	{
		return true;
	}

	return GLX::Object::OnEvent(src, e);
}

void WidgetImpl::OnSetStyle(const GLX::Style & style)
{
	auto width = Data::GetFloat32(style, "width", 2.0f);
	auto fill = GLX::GetColour(style, "fill");		//unused in this demo

	//Typically do SetGraphicCanvas here in OnSetStyle, or in OnUpdate if it depends on app state

	auto workspace = Make<ObjectOf<Pair<GLX::Points,GLX::ColourPoints>>>();	//an optimisation, reuse buffers on each OnDraw to avoid allocations

	GLX::SetColourCanvas(*this, {}, [this, width, fill, size_z = Make<GLX::SizeProperty>(), workspace](GLX::ColourCanvasContext & ctx)
	{
		//this does *not* get called every frame, what we are doing here is preparing a VBO for GPU
		//to manually reschedule need to call Redraw()

		auto size = ctx.size;

		auto & [a, b] = workspace->value;

		if (SetFiltered(size_z->value, size))	//as an optimistion, only rebuild the hexagon when size changes
		{
			a.Clear();
			b.Clear();

			GLX::Point hexagon[6];
			
			MakeHexagon(hexagon, size);

			GLX::AddPath(a, hexagon, true, width, GLX::kPathJoinRound, GLX::kPathCapButt);

			auto pb = Extend(b, a.GetSize()).data;

			auto inv_w = 1.0f / size.w;
			auto inv_h = 1.0f / size.h;

			constexpr GLX::Colour tl = { 1.0f, 0.0f, 0.0f, 1.0f };
			constexpr GLX::Colour tr = { 0.0f, 1.0f, 0.0f, 1.0f };
			constexpr GLX::Colour bl = { 0.0f, 0.0f, 1.0f, 1.0f };
			constexpr GLX::Colour br = { 0.0f, 1.0f, 1.0f, 1.0f };

			GLX::Colour dt = tr - tl;
			GLX::Colour db = br - bl;

			for (const auto & p : a)
			{
				auto x = p.x * inv_w;
				auto y = p.y * inv_h;

				GLX::Colour top = tl + Mul(dt, x);
				GLX::Colour bot = bl + Mul(db, x);
				GLX::Colour c = top + Mul((bot - top), y);

				*pb++ = { p, c };
			}
		}

		UInt start = ctx.output.GetSize();	//!important, do not assume ctx.output is empty, Canvas calls may be batched together and share same output
		
		ctx.output.Append(b);

		ArrayRegion <GLX::ColourPoint> region = { ctx.output.GetData() + start, b.GetSize() };
		
		GLX::Rotate(region, { size.w * 0.5f, size.h * 0.5f }, m_value * k2Pif);
	});
}

void WidgetImpl::OnUpdate()
{
	GLX::SetText(*this, Join(ToWString(m_value * 100.0f, 1), L'%'));
}

REFLEX_END_INTERNAL

TRef <GLX::Object> CustomDrawing::CreateWidget()
{
	return New<WidgetImpl>();
}
