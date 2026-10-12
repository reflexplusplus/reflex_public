#include "widgets.h"




//
//RangeBar demo

namespace Widgets { namespace {

struct RangeBarDemo : public GLX::Object
{
	static constexpr Float kExtent = 100.0f;

	RangeBarDemo()
	{
		GLX::SetFlow(*this, GLX::kFlowY);
		GLX::SetFlow(m_options, GLX::kFlowX);
		GLX::SetFlow(m_constraints, GLX::kFlowX);

		GLX::SetText(resize_button, L"Resize");
		GLX::SetText(invert_button, L"Invert");
		GLX::SetText(vertical_button, L"Vertical");
		GLX::SetText(jump_button, L"Jump");
		GLX::SetText(snap_button, L"Snap 1/8");
		GLX::SetText(min_length, L"Min length", K32("label"));
		GLX::SetText(min_start, L"Min start", K32("label"));

		GLX::Button * buttons[] = { &resize_button, &invert_button, &vertical_button, &jump_button, &snap_button };
		for (auto button : buttons) GLX::AddInline(m_options, *button);

		min_length.SetRange(0.0f, kExtent, 0.01f);
		min_length.SetPrecision(2);
		min_length.SetValue(0.0f);
		min_start.SetRange(0.0f, kExtent, 0.01f);
		min_start.SetPrecision(2);
		min_start.SetValue(0.0f);

		GLX::AddInline(m_constraints, min_length);
		GLX::AddInline(m_constraints, min_start);

		GLX::AddInline(*this, m_options, GLX::kOrientationCenter);
		GLX::AddInline(*this, m_constraints, GLX::kOrientationCenter);
		m_rangebar.SetParent(m_container);
		UpdateFlow();
		GLX::AddInlineFlex(*this, m_container);
		GLX::AddInline(*this, m_status);

		GLX::BindClick(resize_button, [this]()
		{
			m_resize = !m_resize;
			m_rangebar.EnableResize(m_resize);
			GLX::Select(resize_button, m_resize);
			UpdateStatus();
		});
		GLX::BindClick(invert_button, [this]()
		{
			m_inverted = !m_inverted;
			GLX::Select(invert_button, m_inverted);
			UpdateFlow();
		});
		GLX::BindClick(vertical_button, [this]()
		{
			m_vertical = !m_vertical;
			GLX::Select(vertical_button, m_vertical);
			UpdateFlow();
		});
		GLX::BindClick(jump_button, [this]()
		{
			m_jump_enabled = !m_jump_enabled;
			GLX::Select(jump_button, m_jump_enabled);
		});
		GLX::BindClick(snap_button, [this]()
		{
			m_snap = !m_snap;
			GLX::Select(snap_button, m_snap);
		});

		GLX::Select(jump_button, m_jump_enabled);
		m_rangebar.SetRange(kExtent, { 25.0f, 25.0f });
		UpdateStatus();
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kTransaction && src == m_rangebar)
		{
			auto [extent, region] = m_rangebar.GetRange();
			auto stage = GLX::GetTransactionStage(e);
			auto edge_property = e.QueryProperty<ObjectOf<GLX::Orientation>>(K32("edge"));
			auto edge = edge_property ? edge_property->value : GLX::kOrientationCenter;
			auto length_property = e.QueryProperty<Data::Float32Property>(GLX::krange);
			GLX::Range requested = { Data::GetFloat32(e, GLX::koffset), length_property ? length_property->value : region.length };

			output.Log("range stage=", UInt(stage), " edge=", UInt(edge), " requested=", requested.start, ",", requested.length, " published=", extent, ",", region.start, ",", region.length);

			if (stage == GLX::kTransactionStagePerform) AcceptRange(SnapRange(requested, edge), edge);
			return true;
		}
		else if (e.id == GLX::RangeBar::kJump && src == m_rangebar)
		{
			auto [extent, region] = m_rangebar.GetRange();
			if (m_jump_enabled)
			{
				GLX::Range requested = { Data::GetFloat32(e, GLX::koffset), region.length };
				AcceptRange(SnapRange(requested, GLX::kOrientationCenter), GLX::kOrientationCenter);
			}
			return true;
		}
		else if (e.id == GLX::kTransaction && (src == min_length || src == min_start))
		{
			auto stage = GLX::GetTransactionStage(e);
			if (stage == GLX::kTransactionStagePerform || stage == GLX::kTransactionStageEnd)
			{
				ApplyMinimums(src == min_start);
			}
			return true;
		}

		return Object::OnEvent(src, e);
	}

	void OnSetStyle(const GLX::Style & style) override
	{
		m_options.SetStyle(style["Options"]);
		m_constraints.SetStyle(style["Constraints"]);
		m_container.SetStyle(style["Track"]);
		m_status.SetStyle(style["Status"]);
		min_length.SetStyle(style["Field"]);
		min_start.SetStyle(style["Field"]);

		for (auto & i : m_options) i.SetStyle(style["Option"]);

		m_rangebar.SetStyle(style["Range"]);
	}

	void UpdateFlow()
	{
		GLX::SetFlow(m_rangebar, (m_vertical ? GLX::kFlowY : GLX::kFlowX) | (m_inverted ? GLX::kFlowInvert : 0));

		const GLX::Orientation orientations[] = { GLX::kOrientationFit , GLX::kOrientationCenter};

		GLX::EnableFloat(m_rangebar, orientations[m_vertical], orientations[!m_vertical]);

		UpdateStatus();
	}

	GLX::Range SnapRange(GLX::Range requested, GLX::Orientation edge) const
	{
		if (!m_snap) return requested;

		constexpr Float step = kExtent / 8.0f;
		bool moving_start = (edge == GLX::kOrientationNear && !m_inverted) || (edge == GLX::kOrientationFar && m_inverted);
		bool moving_end = (edge == GLX::kOrientationFar && !m_inverted) || (edge == GLX::kOrientationNear && m_inverted);

		if (moving_start)
		{
			Float end = requested.start + requested.length;
			requested.start = Clip(Quantise(requested.start, step), 0.0f, end);
			requested.length = end - requested.start;
		}
		else if (moving_end)
		{
			Float end = Clip(Quantise(requested.start + requested.length, step), requested.start, kExtent);
			requested.length = end - requested.start;
		}
		else requested.start = Quantise(requested.start, step);

		return requested;
	}

	void ApplyMinimums(bool start_changed)
	{
		auto [extent, region] = m_rangebar.GetRange();
		Float start = Clip(min_start.GetValue(), 0.0f, kExtent);
		Float length = Clip(min_length.GetValue(), 0.0f, kExtent);

		if (start + length > kExtent)
		{
			if (start_changed) start = kExtent - length;
			else length = kExtent - start;
		}

		min_start.SetValue(start);
		min_length.SetValue(length);
		AcceptRange(region, GLX::kOrientationCenter);
	}

	GLX::Range Constrain(GLX::Range requested, GLX::Orientation edge) const
	{
		Float minstart = min_start.GetValue();
		Float minlength = min_length.GetValue();
		bool moving_start = (edge == GLX::kOrientationNear && !m_inverted) || (edge == GLX::kOrientationFar && m_inverted);
		bool moving_end = (edge == GLX::kOrientationFar && !m_inverted) || (edge == GLX::kOrientationNear && m_inverted);

		if (moving_start)
		{
			Float end = Clip(requested.start + requested.length, minstart + minlength, kExtent);
			Float start = Clip(requested.start, minstart, end - minlength);
			return { start, end - start };
		}
		else if (moving_end)
		{
			Float start = Clip(requested.start, minstart, kExtent - minlength);
			Float end = Clip(requested.start + requested.length, start + minlength, kExtent);
			return { start, end - start };
		}

		Float length = Clip(requested.length, minlength, kExtent - minstart);
		Float start = Clip(requested.start, minstart, kExtent - length);
		return { start, length };
	}

	void AcceptRange(GLX::Range requested, GLX::Orientation edge)
	{
		auto [extent, region] = m_rangebar.GetRange();
		auto accepted = Constrain(requested, edge);
		m_rangebar.SetRange(kExtent, accepted);
		output.Log("accepted=", region.start, ",", region.length);
		UpdateStatus();
	}

	void UpdateStatus()
	{
		auto [extent, region] = m_rangebar.GetRange();
		GLX::SetText(m_status, Join(L"Extent 100    Start ", ToWString(region.start, 2),
			L"    Length ", ToWString(region.length, 2),
			L"    ", m_vertical ? L"Vertical" : L"Horizontal", m_inverted ? L" m_inverted" : L""));
	}

	GLX::Object m_options, m_constraints, m_container, m_status;
	GLX::Button resize_button, invert_button, vertical_button, jump_button, snap_button;
	GLX::DragEdit min_length, min_start;
	GLX::RangeBar m_rangebar;
	bool m_resize = false, m_inverted = false, m_vertical = false, m_snap = false, m_jump_enabled = false;
};

} }

const Widgets::DemoDesc Widgets::g_range_bar = Widgets::MakeDemo("rangebar", L"RangeBar", true, []() -> Unretained <GLX::Object>
{
	return New<RangeBarDemo>();
});
