#include "../demolib/demolib.h"
#include "../libs/utf8.h"
#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

#define CHECK_HRESULT(x)                                                                                                         \
	{                                                                                                                            \
		auto result = (x);                                                                                                       \
		if (FAILED(result))                                                                                                      \
		{                                                                                                                        \
			ErrorMessage(result, "Bad result from\n" #x "\n\nHRESULT 0x%08X", result);                                           \
		}                                                                                                                        \
	}

#define FRAME_COUNT 2

struct Vertex
{
	Vec4 position;
	Vec4 color;
};

class Demo3
{
  public:
	Demo3() = default;
	~Demo3() = default;

	// set up d3d12
	void Init();

	// load assets
	void LoadAssets();

	// draw
	void Draw();

  private:
	ComPtr<IDXGIFactory6> m_factory;

	ComPtr<IDXGIAdapter4> m_adapter;
	DXGI_ADAPTER_DESC3 m_adapterDesc;

	ComPtr<ID3D12Device7> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12CommandList> m_commandList;
	uint32_t m_rtvDescriptorSize;

	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];

	uint32_t m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValue;
};

void Demo3::Init()
{
	// create factory
	CHECK_HRESULT(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)));

	// get first adapter
	CHECK_HRESULT(m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)));

	// get adapter info
	m_adapter->GetDesc3(&m_adapterDesc);

	printf("got adapter %ls [%04x:%04x]", m_adapterDesc.Description, m_adapterDesc.VendorId, m_adapterDesc.DeviceId);

	// create device
	CHECK_HRESULT(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));

	// create command queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_HRESULT(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// set swapchain params
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = g_width;
	swapchainDesc.Height = g_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.BufferCount = FRAME_COUNT;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	swapchainDesc.SampleDesc.Count = 1;

	// create swapchain
	IDXGISwapChain1* swapchain;
	CHECK_HRESULT(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), g_wnd, &swapchainDesc, nullptr, nullptr, &swapchain));
	swapchain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
	swapchain->Release();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// disable alt-enter fullscreening
	CHECK_HRESULT(m_factory->MakeWindowAssociation(g_wnd, DXGI_MWA_NO_ALT_ENTER));

	// render target view heap (TODO: generic heap creator)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	CHECK_HRESULT(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);

	// frame resources
	auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (uint32_t i = 0; i < FRAME_COUNT; i++)
	{
		CHECK_HRESULT(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
}

void Demo3::LoadAssets()
{
	// root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	CHECK_HRESULT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	CHECK_HRESULT(m_device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	// vertex layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
#define X(sem, field)                                                                                                            \
	{sem, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, field), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		X("POSITION", position), X("COLOR", color)
#undef X
	};

	auto vertShader = GetFile("shaders.vs.spv");
	auto pixelShader = GetFile("shaders.ps.spv");

	// shader pipeline (TODO: split into class in demolib)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.InputLayout = {inputLayout, ArraySize(inputLayout)};
	pipelineDesc.pRootSignature = m_rootSignature.Get();
	pipelineDesc.VS = {vertShader.data(), vertShader.size()};
	pipelineDesc.PS = {pixelShader.data(), pixelShader.size()};
}

void Demo3::Draw()
{
}

void InitDemoPalette()
{
	g_useFramebuffer = false;
}

Demo3 demo;
void InitDemo()
{
	demo.Init();
}

void DrawDemo()
{
	demo.Draw();
}

void ShutdownDemo()
{
}
