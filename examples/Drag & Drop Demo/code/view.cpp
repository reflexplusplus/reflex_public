#include "view.h"




//
//DragDropDemo::View implementation

namespace DragDropDemo { namespace {	//begin internal namespace

using namespace Reflex;

struct ViewImpl : public View
{
public:

	static constexpr UInt16 kChunkVersion = 0;

	ViewImpl(App & app);


	//Bootstrap::View callbacks

	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;

	
	//GLX::Object callbacks

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;



	const TRef <App> app;


	//put your GLX::Object members here


	GLX::Button m_ide;


	Reflex::Detail::WeakRef <GLX::Object> m_drop_target;	//weak ref is relatively expensive, but this allows us to safely support non-heaped object

	Reference <Reflex::Object> m_drag_drop_visualiser;

	Reference <GLX::Object> m_boxes[4] = { kNewObject, kNewObject, kNewObject, kNewObject };

	GLX::Object m_rows[2];
};

ViewImpl::ViewImpl(App & app)
	: View(app, kChunkVersion, L":res:DragDropDemo/styles.glx")
	, app(app)
{
	Data::SetBool(*this, GLX::kresize, true);

	if constexpr (REFLEX_DEBUG)
	{
		GLX::SetText(m_ide, L"Console");
		
		GLX::AddFloat(*this, m_ide, GLX::kAlignmentTopRight);
	}

	GLX::SetFlow(*this, GLX::kFlowY | GLX::kFlowCenter);

	UInt idx = 0;

	for (auto & i : m_boxes)
	{
		GLX::SetText(i, ToWString(idx++ + 1));

		GLX::EnableMouseCapture(i, true);

		GLX::AddInline(m_rows[0], i);
	}

	for (auto & i : m_rows) GLX::AddInline(*this, i);

	m_drag_drop_visualiser = GLX::CreateDragDropBeginListener([this](Reflex::Object & drag_data)
	{
		auto origin = GLX::Core::desktop->GetMouseOver();
		TRef <GLX::WindowClient> window = origin->GetWindow();

		//create drag cursor
		auto cursor = New<GLX::Object>();
		GLX::SetText(cursor, GLX::GetText(origin));	
		cursor->SetStyle(GLX::FindStyle(origin, "DragCursor"));
		GLX::EnableMouse(cursor, false, true);	//ensure doesnt intercept mouse
		GLX::Enter(cursor, GLX::kEnterAnimationFade);
		GLX::AddAbsolute(window->GetForeground(), cursor, window->GetMousePosition());

		//attach callbacks to window, so if window is destroyed they wont be called
		SetAbstractProperty(window, "dragdrop_clock", GLX::CreateAnimationClock([window, cursor](Float)
		{
			cursor->SetPosition(window->GetMousePosition());
		}));

		auto run_opacity_animation = [cursor](GLX::Object & drop_target)
		{
			auto fade = GLX::CreateOpacityAnimation("opacity", 0.75f, 0.9f);
			if (IsNull(drop_target)) fade->Flip();
			GLX::Run(cursor, "opacity", 0.25f, fade);
		};
		
		run_opacity_animation(Null<GLX::Object>());	//call now to apply initial fade to cursor
	
		SetAbstractProperty(window, "dragdrop_target", GLX::CreateDragDropTargetListener([this, run_opacity_animation](GLX::Object & drop_target)
		{
			m_drop_target.Load()->ClearState("dragover");
			m_drop_target.Store(drop_target);
			drop_target.SetState("dragover");

			run_opacity_animation(drop_target);
		}));

		SetAbstractProperty(window, "dragdrop_end", GLX::CreateDragDropEndListener([this, window, cursor]()
		{
			GLX::Exit(cursor, true);	//detach cursor with fade
		
			m_drop_target.Clear();
		
			UnsetAbstractProperty(window, "dragdrop_clock");
			UnsetAbstractProperty(window, "dragdrop_target");
			UnsetAbstractProperty(window, "dragdrop_end");	//important always delete containing lambda last
		}));
	});
}

void ViewImpl::OnResetState(Key32 context)
{
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	//if you change/add data here, you will need to increment kChunkVersion, as previous chunks will be incompatible
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		if constexpr (REFLEX_DEBUG)
		{
			if (src == m_ide) Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());
		}

		return true;
	}
	else if (e.id == GLX::kMouseDrag)
	{
		if (auto row_idx = Search(m_rows, src.GetParent()))
		{
			if (GLX::ExceedsDragThreshold(GLX::GetMouseDelta(e)))
			{
				TRef src_row = m_rows[row_idx.value];

				TRef dst_row = m_rows[(row_idx.value + 1) & 1];

				GLX::SetEventDelegate(dst_row, "drag_drop_handler", [this, dst_row, &src](GLX::Object & src, GLX::Event & e)
				{
					switch (e.id.value)
					{
					case GLX::kDragDropTender:
						return true;	//this allows src to receive kDragEnter/kDragLeave/kDragDrop

					case GLX::kDragDropEnter:
						dst_row->SetState("dragover");
						return true;

					case GLX::kDragDropLeave:
						dst_row->ClearState("dragover");
						return true;

					case GLX::kDragDropReceive:
					if (auto drag_src = GLX::QueryDragDropData<GLX::Object>(e))
					{
						GLX::AddInline(dst_row, *drag_src);

						GLX::Enter(*drag_src);

						Data::SetBool(*this, "dropped", true);

						output.Log(Data::GetCString(*drag_src, "payload"));
						
						return true;
					}

					default:
						return false;
					}

					return false;
				});

				SetAbstractProperty(*this, "global_drag_end_listener", GLX::CreateDragDropEndListener([this, src_row, &src]()
				{
					if (!Data::GetBool(*this, "dropped"))
					{
						//no one received the drop

						GLX::AddInline(src_row, src);

						GLX::Enter(src);
					}

					Data::UnsetBool(*this, "dropped");

					UnsetAbstractProperty(*this, "global_drag_end_listener");
				}));

				GLX::UnbindEvent(src_row, "drag_drop_handler");

				Data::SetCString(src, "payload", "example payload");	//attach data to the drag source via generic properties 

				//typically data (first arg) would be actual data, ie a file path etc, but for this contrived example we use a GLX::Object

				GLX::StartDragDrop(src, GLX::kMouseCursorInvisible, GLX::kMouseCursorInvisible);

				GLX::Exit(src, true, GLX::kEnterAnimationFade | GLX::kEnterAnimationSize);
			}

			return true;
		}

		return true;
	}

	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto button = style["Button"];

	if constexpr (REFLEX_DEBUG) m_ide.SetStyle(button);

	auto row_style = style["Row"];

	auto box_style = row_style["Box"];

	for (auto & i : m_rows) i.SetStyle(row_style);

	for (auto & i : m_boxes) i->SetStyle(box_style);
}

void ViewImpl::OnUpdate()
{
	//this is called automatically when app state changes

	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());
}

} }	//end internal namespace

Reflex::TRef <DragDropDemo::View> DragDropDemo::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
