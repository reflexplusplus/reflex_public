#pragma once

#include "../[require].h"
#include <d3d11.h>




//
//Internal

REFLEX_NS(Reflex::System::Win)

struct D3dNullSwapChain : public IDXGISwapChain
{
	static D3dNullSwapChain self;


	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR * __RPC_FAR * ppvObject) { return (HRESULT)(-1); }

	virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }

	virtual ULONG STDMETHODCALLTYPE Release(void) { return 0; }


	virtual HRESULT STDMETHODCALLTYPE SetPrivateData( REFGUID Name, UINT DataSize, _In_reads_bytes_(DataSize)  const void *pData) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData( REFGUID Name,UINT *pDataSize, _Out_writes_bytes_(*pDataSize)  void *pData) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) { return (HRESULT)(-1); }


	virtual HRESULT STDMETHODCALLTYPE GetDevice( REFIID riid, void **ppDevice) { return (HRESULT)(-1); }


	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT flags) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void ** ppSurface) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput * pTarget) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL * pFullscreen, IDXGIOutput ** ppTarget) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC * pDesc) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC * pNewTargetParameters) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput * *ppOutput) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS * pStats) { return (HRESULT)(-1); }

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT * pLastPresentCount) { return (HRESULT)(-1); }
};

REFLEX_END
