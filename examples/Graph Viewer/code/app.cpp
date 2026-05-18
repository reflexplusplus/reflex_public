#include "app.h"




//
// Graph Viewer App implementation

REFLEX_BEGIN_INTERNAL(GraphViewer)

struct TempLocator : public File::VirtualFileSystem::Locator
{
	TempLocator(Key32 domain, Key32 path, const Data::Archive::View & data)
		: File::VirtualFileSystem::Locator(domain),
		m_path(path),
		m_data(data)
	{
	}

	TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, File::Attributes & attributes) const
	{
		if (m_path == Key32(path))
		{
			return File::Detail::CreateMemoryReader(m_data);
		}
		else
		{
			return {};
		}
	}

	Key32 m_path;

	Data::Archive::View m_data;
};

struct AppImpl :
	public App,
	public Bootstrap::Streamable
{
	static constexpr UInt16 kChunkVersion = 2;

	AppImpl()
		: App(K32("GraphViewer")),
		Bootstrap::Streamable(session, K32("app"), kChunkVersion),
		m_vm(VM::Start()),
		m_compiler(VM::Compiler::Create())
	{
		output.Log("Graph Viewer constructed");
	}



	//interface

	void AddGraph() override
	{
		m_graphs.Push().m_code = Data::Pack(CString::View("Float32 Plot(Float32 x) { return x * x; }"));

		Notify(true);
	}

	void RemoveGraph(UInt idx) override
	{
		if (idx < m_graphs.GetSize())
		{
			m_graphs.Remove(idx);

			if (m_graphs.Empty()) AddGraph();

			Notify(true);
		}
	}

	UInt GetNumGraph() const override
	{
		return m_graphs.GetSize();
	}

	void SetCode(UInt idx, const Data::Archive::View & code) override
	{
		if (idx < m_graphs.GetSize())
		{
			auto & graph = m_graphs[idx];

			if (graph.m_code != code)
			{
				graph.m_code = code;

				graph.m_compiled = false;

				Notify(true);
			}
		}
	}

	Data::Archive::View GetCode(UInt idx) const override
	{
		if (idx < m_graphs.GetSize())
		{
			return m_graphs[idx].m_code;
		}

		return {};
	}

	bool IsCompiled() const override
	{
		//TODO although compilation is incredibly fast for small scripts, it would be theoretically nice to compile all scripts on bg

		return true;
	}

	bool Compute(UInt idx, ArrayView <Float> x_in, ArrayRegion <Float> y_out) override
	{
		if (x_in.size == y_out.size)
		{
			auto & graph = m_graphs[idx];

			if (SetFiltered(graph.m_compiled, true))
			{
				graph.m_context.Clear();

				graph.m_plot = 0;

				File::ResourcePool::Lock lock(Bootstrap::global->resourcepool);

				lock.Remove(MakeAddress<Data::ArchiveObject>(K32(":GraphViewer/temp")));

				auto locator = New<TempLocator>(K32("GraphViewer"), K32("temp"), graph.m_code);

				lock.lock.Attach(locator);

				if (auto program = AutoRelease(m_compiler->Compile(lock, L":GraphViewer/temp", VM::kContextFlagMain, Reflex::Object::null, { VM::gCore })))
				{
					graph.m_plot = VM::QueryFunction<Float32, Float32>(program, { VM::kGlobal, K32("Plot") });

					auto context = VM::Context::Create();

					graph.m_context = context;

					graph.m_context->Initialise(program);
				}

				lock.lock.Detach(locator);
			}

			if (graph.m_plot)
			{
				auto yptr = y_out.data;

				for (auto & x : x_in)
				{
					*yptr++ = VM::CallReturningValue<Float32>(graph.m_context, *graph.m_plot, x);
				}

				return true;
			}
			else
			{
				output.Error("not compiled");
			}
		}

		return false;
	}



	//Data::iStreamable callbacks

	void OnReset(Key32 context) override
	{
		m_graphs.Clear();

		AddGraph();
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		Data::Deserialize(stream, m_graphs);
	}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override
	{
		return false;
	}

	void OnStore(Data::Archive & stream) const override
	{
		//if you change/add stored data here, you will need to increment kChunkVersion, as previous chunks will be invalid
		//after changing the kChunkVersion, previous version chunks will be restored by the OnImport callback

		Data::Serialize(stream, m_graphs);
	}


	Reference <Object> m_vm;

	Reference <VM::Compiler> m_compiler;


	struct State
	{
		State()
			: m_compiled(false),
			m_plot(0)
		{
		}

		void Serialize(Data::Archive & stream) const
		{
			Data::Serialize(stream, m_code);
		}

		void Deserialize(Data::Archive::View & stream)
		{
			Data::Deserialize(stream, m_code);

			m_compiled = false;

			m_plot = 0;
		}

		Data::Archive m_code;

		bool m_compiled;

		Reference <VM::Context> m_context;

		const VM::ScriptFunction * m_plot;
	};

	Array <State> m_graphs;
};

REFLEX_END_INTERNAL

TRef <GraphViewer::App> GraphViewer::App::Create()
{
	return New<GraphViewer::AppImpl>();
}

Reflex::Output GraphViewer::output(" Graph Viewer");
