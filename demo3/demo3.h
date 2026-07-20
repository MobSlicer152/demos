#pragma once

#include "../demolib/demolib.h"
#include "../libs/utf8.h"
#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

#define CHECK_HRESULT(x)                                                                                                         \
	do                                                                                                                           \
	{                                                                                                                            \
		auto result = (x);                                                                                                       \
		if (FAILED(result))                                                                                                      \
		{                                                                                                                        \
			ErrorMessage(result, "Bad result from\n" #x "\n\nHRESULT 0x%08X", result);                                           \
		}                                                                                                                        \
	} while (false)

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
	void InitD3D12();

	// load assets
	void LoadAssets();

	// prepare to record draw commands
	void PrepareFrame();

	// draw
	void Draw();

	// submit and present
	void FinishFrame();

  private:
	ComPtr<IDXGIFactory6> m_factory;

	// adapter
	ComPtr<IDXGIAdapter4> m_adapter;
	DXGI_ADAPTER_DESC3 m_adapterDesc = {};

	// device and graphics stuff
	ComPtr<ID3D12Device7> m_device;
	ComPtr<ID3D12CommandQueue> m_directQueue;
	ComPtr<ID3D12CommandAllocator> m_directAllocator;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	uint32_t m_rtvDescriptorSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	uint32_t m_dsvDescriptorSize = 0;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	// transfer stuff
	ComPtr<ID3D12CommandQueue> m_transferQueue;
	ComPtr<ID3D12CommandAllocator> m_transferAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_transferCommandList;
	ComPtr<ID3D12Resource> m_transferBuffer;
	std::span<byte> m_transferBufferPtr;
	size_t m_transferBufferOffset = 0;
	HANDLE m_transferFenceEvent = nullptr;
	ComPtr<ID3D12Fence> m_transferFence;
	uint64_t m_transferFenceValue = 0;

	static constexpr size_t TRANSFER_BUFFER_SIZE = 64 * 1024;

	// swapchain and render targets
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];

	// sync stuff
	uint32_t m_frameIndex = 0;
	HANDLE m_fenceEvent = nullptr;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValue = 0;

	// create command stuff
	HRESULT CreateCommandStuff(
		ID3D12CommandQueue** queue,
		ID3D12CommandAllocator** allocator,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_QUEUE_PRIORITY priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE);

	// create a descriptor heap
	HRESULT CreateDescriptorHeap(
		ID3D12DescriptorHeap** heap,
		uint32_t& descriptorSize,
		uint32_t descriptorCount,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// create a resource
	HRESULT CreateResource(
		ID3D12Resource** resource,
		D3D12_RESOURCE_DESC& desc,
		D3D12_RESOURCE_STATES initialState,
		D3D12_HEAP_TYPE heap = D3D12_HEAP_TYPE_DEFAULT);

	// map a resource
	template <typename T>
	std::span<T> MapResource(ComPtr<ID3D12Resource> resource, size_t size, size_t subResource = 0, size_t readStart = 0, size_t readEnd = 0)
	{
		auto readRange = CD3DX12_RANGE(readStart, readEnd);
		void* ptr = nullptr;
		CHECK_HRESULT(resource->Map(0, &readRange, &ptr));
		return std::span((T*)ptr, size);
	}

	// wait for the last frame
	void WaitForPreviousFrame();

	// get current render target view handle
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t index) const;

	// start a set of transfers
	void BeginTransfers();

	// submit transfers
	void SubmitTransfers(bool wait = true);

	// wait for a set of transfers to finish
	void WaitForTransfers();

	// upload data to a buffer
	void UploadData(std::span<byte> src, ComPtr<ID3D12Resource> dest, uint64_t destOffset = 0);

	// create a vertex buffer
	ComPtr<ID3D12Resource> CreateVertexBuffer(std::span<Vertex> vertices, bool grouped = false);

	// create an index buffer
	ComPtr<ID3D12Resource> CreateIndexBuffer(std::span<Vec3i> indices, bool grouped = false);
};
