#include "demo3.h"

// dont wanna depend on dlls in release
#ifdef _DEBUG
extern "C" __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
extern "C" __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
#endif

void Demo3::InitD3D12()
{
	// create factory
#if 0 // def _DEBUG
	uint32_t factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
	uint32_t factoryFlags = 0;
#endif
	CHECK_HRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory)));

	// get first adapter
	CHECK_HRESULT(m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)));

	// get adapter info
	m_adapter->GetDesc3(&m_adapterDesc);

	Message("got adapter %ls [%04x:%04x]\n", m_adapterDesc.Description, m_adapterDesc.VendorId, m_adapterDesc.DeviceId);

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
	{
		Message("enabling debug layer\n");
		debug->EnableDebugLayer();
		ComPtr<ID3D12Debug6> debug6;
		if (SUCCEEDED(debug.As(&debug6)))
		{
			debug6->SetEnableGPUBasedValidation(true);
			debug6->SetEnableAutoName(true);
		}
	}
#endif

	// create device
	CHECK_HRESULT(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));

	// create command queues and allocators
	CHECK_HRESULT(CreateCommandStuff(&m_directQueue, &m_directAllocator));
	CHECK_HRESULT(CreateCommandStuff(&m_transferQueue, &m_transferAllocator));
	CHECK_HRESULT(m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_transferAllocator.Get(), nullptr, IID_PPV_ARGS(&m_transferCommandList)));
	CHECK_HRESULT(m_transferCommandList->Close());

	// transfer buffer
	auto transferBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(TRANSFER_BUFFER_SIZE);
	CHECK_HRESULT(
		CreateResource(&m_transferBuffer, transferBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD));
	m_transferBufferView = MapResource<byte>(m_transferBuffer, TRANSFER_BUFFER_SIZE);

	// transfer fence
	CHECK_HRESULT(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_transferFence)));
	CHECK_HRESULT(
		(m_transferFenceEvent = CreateEventA(nullptr, false, false, nullptr)) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

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
	CHECK_HRESULT(m_factory->CreateSwapChainForHwnd(m_directQueue.Get(), g_wnd, &swapchainDesc, nullptr, nullptr, &swapchain));
	swapchain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
	swapchain->Release();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// disable alt-enter fullscreening
	CHECK_HRESULT(m_factory->MakeWindowAssociation(g_wnd, DXGI_MWA_NO_ALT_ENTER));

	CHECK_HRESULT(CreateDescriptorHeap(&m_rtvHeap, m_rtvDescriptorSize, FRAME_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	CHECK_HRESULT(CreateDescriptorHeap(&m_dsvHeap, m_dsvDescriptorSize, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	CHECK_HRESULT(CreateDescriptorHeap(
		&m_shaderHeap,
		m_shaderDescriptorSize,
		SHADER_RESOURCE_COUNT,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));

	// frame resources
	auto rtvHandle = GetRTVHandle(0);
	for (uint32_t i = 0; i < FRAME_COUNT; i++)
	{
		CHECK_HRESULT(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// TODO: depth buffer
}

void Demo3::CreateResources()
{
	// root signature
	// aspect, time, etc
	CD3DX12_ROOT_PARAMETER constants = {};
	constants.InitAsConstants(ArraySize(m_rootParams), 0);

	// particle buffer and generated textures
	CD3DX12_DESCRIPTOR_RANGE srvDescriptorRange;
	srvDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SHADER_RESOURCE_COUNT, 1);

	CD3DX12_ROOT_PARAMETER descriptorTable = {};
	std::array<D3D12_DESCRIPTOR_RANGE, 1> descriptorRanges = {srvDescriptorRange};
	descriptorTable.InitAsDescriptorTable(descriptorRanges.size(), descriptorRanges.data());

	D3D12_ROOT_PARAMETER rootParams[] = {constants, descriptorTable};

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 2;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
		ArraySize(rootParams), rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

	auto vertShader = GetFile("shaders.vs.bin");
	auto pixelShader = GetFile("shaders.ps.bin");

	// shader pipeline (TODO: split into class in demolib)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.InputLayout = {inputLayout, ArraySize(inputLayout)};
	pipelineDesc.pRootSignature = m_rootSignature.Get();
	pipelineDesc.VS = {vertShader.data(), vertShader.size()};
	pipelineDesc.PS = {pixelShader.data(), pixelShader.size()};
	pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineDesc.DepthStencilState.DepthEnable = false; // TODO: depth buffer
	pipelineDesc.DepthStencilState.StencilEnable = false;
	pipelineDesc.SampleMask = UINT32_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.SampleDesc.Count = 1;

	CHECK_HRESULT(m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&m_pipelineState)));

	// command list
	CHECK_HRESULT(m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_directAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
	CHECK_HRESULT(m_commandList->Close()); // take out of recording

	// particle buffer
	auto particleBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(MAX_PARTICLES * sizeof(Particle));
	CHECK_HRESULT(
		CreateResource(&m_particleBuffer, particleBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD));
	m_particleBufferView = MapResource<Particle>(m_particleBuffer, MAX_PARTICLES);
	m_particleBufferHandle.InitOffsetted(m_shaderHeap->GetCPUDescriptorHandleForHeapStart(), 0);

	// generated textures
	auto generatedTexturesDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, GENERATED_TEXTURE_WIDTH, GENERATED_TEXTURE_HEIGHT, GENERATED_TEXTURE_COUNT, 1);
	CHECK_HRESULT(CreateResource(&m_generatedTextures, generatedTexturesDesc, D3D12_RESOURCE_STATE_COPY_DEST));
	auto generatedTexturesView =
		CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(generatedTexturesDesc.Format, generatedTexturesDesc.DepthOrArraySize);
	m_generatedTexturesHandle.InitOffsetted(m_particleBufferHandle, 1, m_shaderDescriptorSize);
	m_device->CreateShaderResourceView(m_generatedTextures.Get(), &generatedTexturesView, m_generatedTexturesHandle);

	// character textures
	auto characterTexturesDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, CHARACTER_TEXTURE_WIDTH, CHARACTER_TEXTURE_HEIGHT, CHARACTER_TEXTURE_COUNT, 1);
	CHECK_HRESULT(CreateResource(&m_characterTextures, characterTexturesDesc, D3D12_RESOURCE_STATE_COPY_DEST));
	auto characterTexturesView =
		CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(characterTexturesDesc.Format, characterTexturesDesc.DepthOrArraySize);
	m_characterTexturesHandle.InitOffsetted(m_particleBufferHandle, 2, m_shaderDescriptorSize);
	m_device->CreateShaderResourceView(m_characterTextures.Get(), &characterTexturesView, m_characterTexturesHandle);

	SetupAssets();

	// sync stuff
	CHECK_HRESULT(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	CHECK_HRESULT((m_fenceEvent = CreateEventA(nullptr, false, false, nullptr)) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
	WaitForPreviousFrame();
}

void Demo3::PrepareFrame()
{
	// reset command stuff
	CHECK_HRESULT(m_directAllocator->Reset());
	CHECK_HRESULT(m_commandList->Reset(m_directAllocator.Get(), m_pipelineState.Get()));

	// set root signature
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// set viewport and scissor
	D3D12_VIEWPORT viewport = {0, 0, g_width, g_height, 0.0, 1.0};
	m_commandList->RSSetViewports(1, &viewport);
	D3D12_RECT scissor = {0, 0, g_width, g_height};
	m_commandList->RSSetScissorRects(1, &scissor);

	// transition the current frame from present to render target
	auto renderTarget = m_renderTargets[m_frameIndex];
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	// set the render target
	auto rtvHandle = GetRTVHandle(m_frameIndex);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr); // TODO: depth buffer

	// set descriptor heaps
	m_commandList->SetDescriptorHeaps(1, m_shaderHeap.GetAddressOf());
}

void Demo3::FinishFrame()
{
	// transition the current frame from render target back to present
	auto renderTarget = m_renderTargets[m_frameIndex];
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	// close and execute the lists
	ID3D12CommandList* commandLists[] = {m_commandList.Get()};
	for (auto list : std::span((ID3D12GraphicsCommandList**)commandLists, ArraySize(commandLists)))
	{
		list->Close();
	}
	m_directQueue->ExecuteCommandLists(ArraySize(commandLists), commandLists);

	// present and wait for the last frame to finish
	CHECK_HRESULT(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	WaitForPreviousFrame();
}

HRESULT Demo3::CreateCommandStuff(
	ID3D12CommandQueue** queue,
	ID3D12CommandAllocator** allocator,
	D3D12_COMMAND_LIST_TYPE type /*= D3D12_COMMAND_LIST_TYPE_DIRECT*/,
	D3D12_COMMAND_QUEUE_PRIORITY priority /*= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL*/,
	D3D12_COMMAND_QUEUE_FLAGS flags /*= D3D12_COMMAND_QUEUE_FLAG_NONE*/)
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = type;
	commandQueueDesc.Priority = priority;
	commandQueueDesc.Flags = flags;
	auto result = m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(queue));
	if (FAILED(result))
	{
		Message("failed to create command queue: HRESULT 0x%08X\n", result);
		return result;
	}

	result = m_device->CreateCommandAllocator(type, IID_PPV_ARGS(allocator));
	if (FAILED(result))
	{
		Message("failed to create command allocator: HRESULT 0x%08X\n", result);
		return result;
	}

	return S_OK;
}

HRESULT Demo3::CreateDescriptorHeap(
	ID3D12DescriptorHeap** heap,
	uint32_t& descriptorSize,
	uint32_t descriptorCount,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = descriptorCount;
	desc.Type = type;
	desc.Flags = flags;
	auto result = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap));
	if (SUCCEEDED(result))
	{
		descriptorSize = m_device->GetDescriptorHandleIncrementSize(desc.Type);
	}

	return result;
}

HRESULT Demo3::CreateResource(
	ID3D12Resource** resource,
	D3D12_RESOURCE_DESC& desc,
	D3D12_RESOURCE_STATES initialState,
	D3D12_HEAP_TYPE heap /*= D3D12_HEAP_TYPE_DEFAULT*/)
{
	auto heapProps = CD3DX12_HEAP_PROPERTIES(heap);
	return m_device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr, IID_PPV_ARGS(resource));
}

void Demo3::WaitForPreviousFrame()
{
	const auto fence = m_fenceValue;
	CHECK_HRESULT(m_directQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		CHECK_HRESULT(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Demo3::GetRTVHandle(uint32_t index) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), index, m_rtvDescriptorSize);
}

void Demo3::BeginTransfers()
{
	// wait for anything unfinished to be done
	WaitForTransfers();

	// reset the command allocator, command list, and offset
	CHECK_HRESULT(m_transferAllocator->Reset());
	CHECK_HRESULT(m_transferCommandList->Reset(m_transferAllocator.Get(), nullptr));
}

void Demo3::SubmitTransfers(bool wait /*= true*/)
{
	m_transferCommandList->Close();
	m_transferQueue->ExecuteCommandLists(1, (ID3D12CommandList**)m_transferCommandList.GetAddressOf());
	if (wait)
	{
		WaitForTransfers();
	}
}

void Demo3::WaitForTransfers()
{
	const auto fence = m_transferFenceValue;
	CHECK_HRESULT(m_transferQueue->Signal(m_transferFence.Get(), fence));
	m_transferFenceValue++;

	if (m_transferFence->GetCompletedValue() < fence)
	{
		CHECK_HRESULT(m_transferFence->SetEventOnCompletion(fence, m_transferFenceEvent));
		WaitForSingleObject(m_transferFenceEvent, INFINITE);
	}

	m_transferBufferOffset = 0;
}

void Demo3::UploadData(std::span<const byte> src, ComPtr<ID3D12Resource> dest, uint64_t destOffset /*= 0*/)
{
	if (src.empty()) {}

	// check if it fits in the transfer buffer
	size_t newOffset = m_transferBufferOffset + src.size();
	if (newOffset > TRANSFER_BUFFER_SIZE)
	{
		Message(
			"tried to write %zu bytes (%zu byte(s) past the limit) to transfer buffer, go increase its size",
			newOffset,
			newOffset - TRANSFER_BUFFER_SIZE);
		return;
	}

	// copy to the cpu mapped address
	std::copy(src.begin(), src.end(), m_transferBufferView.begin() + m_transferBufferOffset);

	// copy the gpu buffer to the target resource
	auto desc = dest->GetDesc();
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_transferCommandList->CopyBufferRegion(
			dest.Get(), destOffset, m_transferBuffer.Get(), m_transferBufferOffset, src.size());
	}
	else
	{
		// get data regions to copy
		auto subresourceCount = desc.MipLevels * desc.DepthOrArraySize;
		auto footprints = std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(subresourceCount);
		auto sizes = std::vector<uint64_t>(subresourceCount);
		m_device->GetCopyableFootprints(
			&desc, 0, subresourceCount, m_transferBufferOffset, footprints.data(), nullptr, nullptr, sizes.data());
		for (uint32_t i = 0; i < subresourceCount; i++)
		{
			auto srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(m_transferBuffer.Get(), footprints[i]);
			auto destLocation = CD3DX12_TEXTURE_COPY_LOCATION(dest.Get(), i);
			m_transferCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
		}
	}

	// set the new offset
	m_transferBufferOffset = newOffset;
}

ComPtr<ID3D12Resource> Demo3::CreateVertexBuffer(std::span<Vertex> vertices, bool grouped /*= false*/)
{
	if (!grouped)
	{
		BeginTransfers();
	}

	ComPtr<ID3D12Resource> buffer;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size_bytes());
	CHECK_HRESULT(CreateResource(&buffer, desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	UploadData(std::span((byte*)vertices.data(), vertices.size_bytes()), buffer);

	if (!grouped)
	{
		SubmitTransfers();
	}

	return buffer;
}

ComPtr<ID3D12Resource> Demo3::CreateIndexBuffer(std::span<Vec3i> indices, bool grouped /*= false*/)
{
	if (!grouped)
	{
		BeginTransfers();
	}

	ComPtr<ID3D12Resource> buffer;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(indices.size_bytes());
	CHECK_HRESULT(CreateResource(&buffer, desc, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	UploadData(std::span((byte*)indices.data(), indices.size_bytes()), buffer);

	if (!grouped)
	{
		SubmitTransfers();
	}

	return buffer;
}
