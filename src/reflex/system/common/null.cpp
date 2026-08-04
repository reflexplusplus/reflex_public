#include "[require].h"

#include "graphics/graphics.h"




//
//nulls

REFLEX_BEGIN_INTERNAL(Reflex::System)

struct NullDiskIterator : public DiskIterator
{
	bool GetNext(bool & removable, WString & filename, WString & displayname) override { return false; }
}
g_null_disk_iterator;

struct NullDirectoryIterator : public DirectoryIterator
{
	bool GetNext(Item & item) override { return false; }
}
g_null_directory_iterator;

struct NullDynamicLibrary : public DynamicLibrary
{
	bool Status() const override { return false; }
	void * GetBundleRef() const override { return 0; }
	void * GetFunction(const CString & function) const override { return 0; }
}
g_null_dynamic_library;

struct NullThread : public Thread
{
	bool Completed() const override { return true; }
	void Wait() override {}
}
g_null_thread;

struct NullCriticalSection : public CriticalSection
{
	int WorkaroundVisualStudioBug() { return 1; }
	void Enter() override {}
	void Leave() override {}
}
g_null_critical_section;

struct NullProcess : public Process
{
	bool Status() const override { return false; }
	bool Completed() const override { return true; }
	void Wait() override {}
	void Terminate() override {}
}
g_null_process;

struct NullFileHandle : public FileHandle
{
	bool IsWriteable() const override { return true; }
	UInt64 GetSize() const override { return 0; }
	void SetPosition(UInt64 size) override {}
	UInt64 GetPosition() const override { return 0; }
	UInt32 Read(void * ptr, UInt32 size) override { return 0; }
	UInt32 Write(const void * ptr, UInt32 size) override { return 0; }
	bool Flush(bool commit) override { return false; }
	bool Truncate() override { return false; }
}
g_null_file_handle;

struct NullHttpConnection : public HttpConnection
{
	void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) override {}
	Response Request(const CString::View &, const CString::View &, const ArrayView < Pair <CString> > &, const ArrayView <UInt8> &, const ReceiveHeaderFn &, const ReceiveDataFn &) override
	{
		return kResponseNoConnection;
	}
}
g_null_http_connection;

struct NullWindow : public Window
{
	void SetClient(TRef <Client> client) override { REFLEX_ASSERT(false); AutoRelease(client); }
	TRef <Client> GetClient() override { return Client::null; }
	void Attach(System::Window & parent) override {}
	void Detach() override {}
	void SetTitle(const WString & label) override {}
	void SetDisplayMode(WindowDisplay state) override {}
	void SetRect(const iRect & rect) override {}
	void SendTop() override {}
	void EnableInput(bool enable) override {}
	void SetMouseCursor(MouseCursor mousecursor) override {}
	void SetMousePosition(const iPoint & point) override {}
	void BeginDragDropFiles(const ArrayView <WString> & files) override {}
	TRef < ObjectOf <RawBitmap> > CreateExportBitmapBuffer(UInt8 flags) const override
	{
		auto rtn = REFLEX_CREATE(ObjectOf<RawBitmap>);
		auto & info = rtn->value.a;
		info.format = kImageFormatRGBA;
		info.size = { 0, 0 };
		info.pixdensity = 1;
		return rtn;
	}
	void ExportBitmap(TRef < ObjectOf <RawBitmap> > rtn) const override
	{
		auto & info = rtn->value.a;
		info.format = kImageFormatRGBA;
		info.size = { 0, 0 };
		info.pixdensity = 1;
	}
	void * GetSystemHandle() override { return 0; }
}
g_null_window;

struct NullWindowClient : public Window::Client
{
	void OnSetOwner(Window * window) override {}
	iSize OnGetContentSize() override { return {}; }
	void OnSetRect(WindowDisplay state, const iRect & rect, const System::iRect & interactable, Int32 dpifactor) override {}
	void OnSetFocus() override {}
	void OnLoseFocus() override {}
	void OnMouseEnter() override {}
	void OnMouseLeave() override {}
	void OnMouseMove(iPoint position) override {}
	void OnMouseDown(bool rmb, bool dbl) override {}
	void OnMouseUp(bool rmb) override {}
	void OnMouseWheel(fPoint delta, bool high_res, bool inverted) override {}
	void OnTouchBegin(UIntNative touch_id, Float64 timestamp, fPoint position) override {}
	void OnTouchMove(UIntNative touch_id, Float64 timestamp, fPoint position) override {}
	void OnTouchEnd(UIntNative touch_id, Float64 timestamp) override {}
	void OnTouchCancel(UIntNative touch_id, Float64 timestamp) override {}
	bool OnKeyPress(KeyCode keycode, bool repeat) override { return false; }
	void OnKeyRelease(KeyCode keycode) override {}
	bool OnCharacter(WChar character) override { return false; }
	void OnDrop(const Object & object) override {}
	ScreenOrientation OnGetScreenOrientation() override { return kScreenOrientationDefault; }
	void OnRequestClose() override {}
}
g_null_window_client;

struct NullAudioCallbacks : public AudioPlugin::Callbacks
{
	Array <UInt8> OnGetPluginChunk() override { return {}; }
	void OnSetPluginChunk(const ArrayView <UInt8> & pluginchunk) override {}
	void OnGetParameterInfo(UInt32 idx, CString & name) const override {}
	Float32 OnGetParameterValue(UInt32 idx) const override { return 0.0f; }
	void OnSetParameterValue(UInt32 idx, Float32 value) override {}
	FunctionPointer <void(Callbacks&, UInt)> OnPrepare(UInt32 maxbuffersize, Float32 samplerate, ConstTRef <AudioPlugin::EventBuffer> eventsin, TRef <AudioPlugin::EventBuffer> eventsout, const ArrayView <const Float32*> & inputs, const ArrayView <Float32*> & outputs) override
	{
		return [](Callbacks&, UInt) {};
	}
}
g_null_audio_callbacks;

struct NullAudioDevice : public AudioDevice
{
	ArrayView <UInt8> GetID() const override { return {}; }
	Array <Float> GetAvailableSampleRates() const override { return {}; }
	Array <UInt32> GetAvailableBufferSizes() const override { return {}; }
	bool Init(Object & owner, const Config & config) override { return false; }
	Float GetSampleRate() const override { return 1.0f; }
	UInt32 GetBufferSize() const override { return 1024; }
	bool Start() override { return false; }
	void Stop() override {}
}
g_null_audio_device;

struct NullExternalResourceRef : public ExternalResourceRef
{
	Array <UInt8> GetPersistentToken() override { return {}; }
	TRef <FileHandle> Open(FileHandle::Mode mode) override { return FileHandle::null; }
}
g_null_external_resource_ref;

struct NullRenderer : public Renderer
{
	bool Status() const override { return false; }
	CString::View GetEngineName() const override { return "NoRenderer"; }
	Config GetConfig() const override
	{
		Config config;
		return config;
	}
	bool SupportsImageFormat(ImageFormat format) const override { return false; }
	void BeginAccess() override {}
	void EndAccess() override {}
	TRef <Canvas> CreateCanvas(void * systemwindow) override { return Canvas::null; }
	TRef <Canvas> CreateBitmap(bool alphachannel, bool antialias) override { return Canvas::null; }
	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <fPoint> & points) override { return Graphic::null; }
	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <ColourPoint> & points) override { return Graphic::null; }
	void EnableBlend(bool) override {}
	void SetDitheringAmount(Float) override {}
	void SetColourTransform(const ArrayView <Float> & m) override {}
	void Clear(const Colour & colour) override {}
	void SetClip(const iRect & rect) override {}
	void SetMask(const Graphic & mask, const Transform & transform, bool invert) override {}
	void ClearMask() override {}
	Canvas * GetCurrent() override { return 0; }
}
g_null_renderer;

struct NullGraphic : public Renderer::Graphic
{
	void Render(const Renderer::Transform & transform, const Colour & colour) const override {}
}
g_null_graphic;

struct NullCanvas : public Common::CanvasBase
{
	bool SetSize(const iSize & size, Int32 dpifactor) override { return true; }
	void SetCurrent() override {}
}
g_null_canvas;

REFLEX_END_INTERNAL

Reflex::System::DiskIterator & Reflex::System::DiskIterator::null = Reflex::System::g_null_disk_iterator;
Reflex::System::DirectoryIterator & Reflex::System::DirectoryIterator::null = Reflex::System::g_null_directory_iterator;
Reflex::System::DynamicLibrary & Reflex::System::DynamicLibrary::null = Reflex::System::g_null_dynamic_library;
Reflex::System::Task & Reflex::System::Task::null = Reflex::System::g_null_thread;
Reflex::System::Thread & Reflex::System::Thread::null = Reflex::System::g_null_thread;
Reflex::System::CriticalSection & Reflex::System::CriticalSection::null = Reflex::System::g_null_critical_section;
Reflex::System::Process & Reflex::System::Process::null = Reflex::System::g_null_process;
Reflex::System::FileHandle & Reflex::System::FileHandle::null = Reflex::System::g_null_file_handle;
Reflex::System::HttpConnection & Reflex::System::HttpConnection::null = Reflex::System::g_null_http_connection;
Reflex::System::Window::Client & Reflex::System::Window::Client::null = Reflex::System::g_null_window_client;
Reflex::System::Window & Reflex::System::Window::null = Reflex::System::g_null_window;
Reflex::System::AudioPlugin::Callbacks & Reflex::System::AudioPlugin::Callbacks::null = Reflex::System::g_null_audio_callbacks;
Reflex::System::AudioDevice & Reflex::System::AudioDevice::null = Reflex::System::g_null_audio_device;
Reflex::System::ExternalResourceRef & Reflex::System::ExternalResourceRef::null = Reflex::System::g_null_external_resource_ref;
Reflex::System::Renderer & Reflex::System::Renderer::null = Reflex::System::g_null_renderer;
Reflex::System::Renderer::Graphic & Reflex::System::Renderer::Graphic::null = Reflex::System::g_null_graphic;
Reflex::System::Renderer::Canvas & Reflex::System::Renderer::Canvas::null = Reflex::System::g_null_canvas;
