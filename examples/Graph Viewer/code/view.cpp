#include "view.h"




//
// Graph Viewer View implementation

REFLEX_BEGIN_INTERNAL(GraphViewer)

ConstTRef <GLX::Style> SetSubStyle(const GLX::Style & parent_style, GLX::Object & child, Key32 style_Id)
{
	auto style = parent_style[style_Id];

	child.SetStyle(style);

	return style;
}

struct ViewImpl : public View
{
public:

	static constexpr UInt16 kChunkVersion = 1;

	ViewImpl(App & app);



	//Bootstrap::View state-persistence callbacks

	virtual void OnResetState(Key32 context) override;

	virtual void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	virtual void OnStoreState(Data::Archive & stream) const override;


	
	//GLX::Object callbacks

	virtual bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual void OnUpdate() override;



	//links

	const TRef <App> app;

	UInt m_idx;


	//content

	GLX::Object m_header;

	GLX::Button m_new, m_open, m_save;

	GLX::Button m_ide;


	GLX::Split m_body;

	GLX::Object m_left_split;

	GLX::TextArea m_textarea;

	GLX::Button m_add, m_delete;

	GLX::Object m_tabs_content;

	GLX::Scroller m_tabs_scroller;

	GLX::Object m_button_bar;

	
	GLX::Object m_graph;

	GLX::Zoomable m_viewport, m_viewport_ybar;


	//workspace

	Float32 m_line_width;

	Array <Float> m_compute_workspace;

	Array <System::ColourPoint> m_colourpoints;
};

ViewImpl::ViewImpl(App & app)
	: View(app, kChunkVersion, L":res:GraphViewer/styles.txt"),
	app(app),
	m_idx(0),
	m_new(L"New"),
	m_open(L"Open"),
	m_save(L"Save"),
	m_ide(L"Console"),
	m_textarea(true),
	m_add(L"Add"),
	m_delete(L"Delete"),
	m_line_width(1.0f)
{
	//layout

	Data::SetBool(*this, GLX::kresize, true);


	m_viewport.InvertAxis(false, true);

	m_viewport_ybar.InvertAxis(false, true);


	Data::SetBool(m_left_split, GLX::kresize, true);

	GLX::SetFlow(m_left_split, GLX::kFlowY);

	GLX::SetFlow(m_tabs_content, GLX::kFlowX);

	GLX::SetFlow(m_tabs_scroller, GLX::kFlowX);

	GLX::SetBounds(m_graph, {}, { 1.0f, 1.0f });


	//GLX::ViewPort ("floats" the primary axis bar over the content, we want it outside, so we use a 2nd one and sync them

	GLX::EnableAutoFit(m_viewport_ybar, true, true);	//make it accommodate its ybar (off by default on ViewPort)

	GLX::SetFlow(m_viewport_ybar, GLX::kFlowX);

	m_viewport_ybar.SetContent(New<GLX::Object>());

	GLX::SetBounds(m_viewport_ybar.GetContent(), {}, {1.0f, 1.0f});

	m_viewport.SetViewBarCtr(true, GLX::Zoomable::kNullBar);

	m_viewport_ybar.SetViewBarCtr(false, GLX::Zoomable::kNullBar);

	GLX::SyncViewports(m_viewport, m_viewport_ybar, false, true);

	GLX::SyncViewports(m_viewport_ybar, m_viewport, false, true);


	GLX::AddInline(m_header, m_new, GLX::kOrientationCenter);

	GLX::AddInline(m_header, m_open, GLX::kOrientationCenter);

	GLX::AddInline(m_header, m_save, GLX::kOrientationCenter);

	if constexpr (REFLEX_DEBUG) GLX::AddFloat(m_header, m_ide, GLX::kAlignmentRight);

	GLX::AddInline(*this, m_header);


	GLX::AddInlineFlex(m_left_split, m_textarea);

	
	m_tabs_scroller.SetContent(m_tabs_content);

	GLX::AddInlineFlex(m_button_bar, m_tabs_scroller);

	GLX::AddInline(m_button_bar, m_delete);

	GLX::AddInline(m_button_bar, m_add);

	GLX::AddInline(m_left_split, m_button_bar);


	GLX::AddInline(m_body, m_left_split);

	m_viewport.SetContent(m_graph);

	GLX::AddInlineFlex(m_body, m_viewport);

	GLX::AddInline(m_body, m_viewport_ybar);

	GLX::AddInlineFlex(*this, m_body);


	
	//setup drawing

	GLX::SetGraphicCanvas(m_graph, {}, [this](GLX::GraphicCanvasContext & ctx)
	{
		constexpr auto colour = GLX::RGB(0, 128, 255, 200);


		auto pix = m_viewport.GetPixelsPerUnit();

		auto visible = m_viewport.GetView();


		auto start = Max(visible.origin.x - 0.25f, 0.0f);	//start to left of window to make sure curve angle is good

		auto end = Min(visible.origin.x + visible.size.w, 1.0f);

		auto x_start = Quantise(start, pix.w);

		auto xstep = pix.w * 4.0f;


		//allocate 1 memory buffer and use 2 portions of it

		Int allocsize = Truncate((end - start) / xstep) + 8;

		if (allocsize < 10) return;


		m_compute_workspace.SetSize(allocsize * 2);

		auto xptr = m_compute_workspace.GetData();

		auto yptr = xptr + allocsize;

		UInt n = 0;

		{
			auto x = x_start;

			while (x < end)
			{
				*xptr++ = x;

				x += xstep;
			}

			*xptr++ = end;

			n = UInt(xptr - m_compute_workspace.GetData());

			REFLEX_ASSERT(Int(n) < allocsize);

			xptr = m_compute_workspace.GetData();
		}

		if (!ViewImpl::app->Compute(m_idx, { xptr, n }, { yptr, n })) return;

		
		//prealloc output and draw graph from x + y's

		m_colourpoints.Allocate(n * 4);

		m_colourpoints.Clear();

		auto xend = xptr + n;

		GLX::Point prev = { *xptr++, *yptr++ };
		
		Float thickness = pix.h * m_line_width;

		while (xptr < xend)
		{
			GLX::Point curr = { *xptr++, *yptr++ };

			auto dir = GLX::Detail::Normalise(curr - prev);

			GLX::Point normal = { -dir.y, dir.x };

			auto offset = normal * MakeSize(thickness * 0.5f);

			auto ptr = Extend(m_colourpoints, 4);

			ptr[0] = { { prev - offset }, colour };
			ptr[1] = { { prev + offset }, colour };
			ptr[2] = { { curr - offset }, colour };
			ptr[3] = { { curr + offset }, colour };

			prev = curr;
		}

		ctx.output = GLX::CreateGraphic(m_colourpoints, System::Renderer::kPrimitiveTypeTriangleStrip);
	});
}

void ViewImpl::OnResetState(Key32 context)
{
	m_idx = 0;
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
	auto [idx, split, vo] = Data::Deserialize<UInt32,Float32,Float32>(stream);

	m_idx = Min(idx, app->GetNumGraph() - 1);
	
	m_body.SetSplitSize(m_left_split, split);
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	Data::Serialize(stream, m_idx, m_body.GetSplitSize(m_left_split), 0.0f);	//reserve vo
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	constexpr auto Open = [](App & app)
	{
		if (auto path = GLX::ShowFileDialog(false, { App::kFileExt }, app.GetFilename()))
		{
			app.Open(path);
		}
	};

	constexpr auto Save = [](App & app)
	{
		if (auto path = GLX::ShowFileDialog(true, { App::kFileExt }, app.GetFilename()))
		{
			app.Save(path);
		}
	};

	if (e.id == GLX::kTransaction)
	{
		auto content = m_textarea.GetContent();

		if (src == content)
		{
			switch (GLX::GetTransactionStage(e))
			{
			case GLX::kTransactionStagePerform:
			case GLX::kTransactionStageEnd:
				app->SetCode(m_idx, Data::EncodeUTF8(GLX::GetText(content)));
				break;
			}
		}
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (src == m_new)
		{
			app->Reset();
		}
		else if (src == m_open)
		{
			Open(app);
		}
		else if (src == m_save)
		{
			Save(app);
		}
		else if (src == m_add)
		{
			m_idx = app->GetNumGraph();

			app->AddGraph();

			return true;
		}
		else if (src == m_delete)
		{
			app->RemoveGraph(m_idx);

			return true;
		}
		else if (src.GetParent() == m_tabs_content)
		{
			m_idx = GLX::LookupIndex(src).value;

			Update();

			return true;
		}
		if constexpr (REFLEX_DEBUG)
		{
			if (src == m_ide) Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());
		}

		return true;
	}
	else if (e.id == GLX::kKeyDown)
	{
		auto modifiers = GLX::GetModifierKeys(e);

		if (modifiers & GLX::kModifierKeyPrimary)
		{
			switch (GLX::GetKeyCode(e))
			{
			case GLX::kKeyCodeN:
				app->Reset();
				return true;

			case GLX::kKeyCodeO:
				Open(app);
				return true;

			case GLX::kKeyCodeS:
				Save(app);
				return true;
			}
		}
	}

	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto button = style["Button"];

	if constexpr (REFLEX_DEBUG) m_ide.SetStyle(button);

	m_header.SetStyle(style["Header"]);

	m_new.SetStyle(button);

	m_open.SetStyle(button);

	m_save.SetStyle(button);


	auto editor = style["Editor"];

	m_left_split.SetStyle(editor);

	SetSubStyle(editor, m_textarea, "TextEditor");

	auto button_bar = SetSubStyle(editor, m_button_bar, "ButtonBar");

	auto tabs = SetSubStyle(button_bar, m_tabs_scroller, "Tabs");

	SetSubStyle(button_bar, m_add, "Add");

	SetSubStyle(button_bar, m_delete, "Delete");

	auto tab = tabs["Tab"];

	for (auto & i : m_tabs_content) i.SetStyle(tab);


	m_viewport.SetStyle(style["Graph"]);

	m_viewport_ybar.SetStyle(style["YBar"]);


	//custom properties for graph

	auto graph = m_graph.GetStyle();

	if (auto values = Data::GetFloat32Array(graph, "line_width"))	//in GLX StyleSheets all numbers are ArrayOfFloats type
	{
		m_line_width = values.GetFirst();
	}
	else
	{
		m_line_width = 1.0f;
	}
}

void ViewImpl::OnUpdate()
{
	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());

	
	auto filename = File::SplitFilename(app->GetFilename()).b;

	WString display = filename ? filename : ToView(L"Untitled");

	if (app->IsEdited()) display.Append(L" *");

	GLX::SetText(m_header, display);


	auto n = app->GetNumGraph();

	m_idx = Min(m_idx, n - 1);	//assume App always has 1 graph


	auto tab_style = m_tabs_scroller.GetStyle()["Tab"];

	GLX::Detail::Recycler recycler(m_tabs_content, tab_style, GLX::kEnterAnimationFade | GLX::kEnterAnimationSize);

	for (UInt idx = 0; idx < n; ++idx)
	{
		auto tab = GLX::AddInline(m_tabs_content, recycler.Acquire(idx));

		GLX::Select(tab, idx == m_idx);
	}
		

	GLX::SetText(m_textarea.GetContent(), Data::DecodeUTF8(app->GetCode(m_idx)));

	m_graph.Realign();
}

REFLEX_END_INTERNAL

TRef <GraphViewer::View> GraphViewer::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
