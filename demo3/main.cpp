#include "demo3.h"

void Demo3::Update()
{
	m_sceneProgress += g_delta;
	if (m_sceneProgress > SCENE_LENGTHS[m_scene])
	{
		m_scene = (Scene)((m_scene + 1) % SceneCount);
		m_sceneProgress = 0.0f;
	}

	// set shader params
	float progress = std::max(m_sceneProgress / 2, 0.0f);
	float jitter = std::clamp(exp(progress * 6.0f), 0.0f, 40.0f) * sin(progress * 100.0f + 0.5f) * 0.0005f;
	float fountainSize = std::clamp(progress / 2, 0.0f, 0.8f) + jitter;

	auto foregroundBrightness = [](float x) { return std::clamp(exp(-7.0f * (x - 0.76f)), 0.0f, 1.0f); };
	auto backgroundBrightness = [](float x) {
		float y;
		if (x < 0.24f)
		{
			y = 3.5f * x - 1.75f;
		}
		else
		{
			y = 2.3f * x - 0.6f;
		}
		return std::clamp(-abs(y) + 1.9f, 0.0f, 1.0f);
	};

	float brightness = backgroundBrightness(progress);
	float fountainBrightness = foregroundBrightness(progress);
	int32_t smokeLayers = 4;

	m_rootParams = {
		RootParam::Int((int)m_scene),
		RootParam::Float(g_aspect),
		RootParam::Float(progress),
		RootParam::Float(1.0f),
		RootParam::Float(brightness),
		RootParam::Float(fountainBrightness),
		RootParam::Float(fountainSize),
		RootParam::Int(smokeLayers)};
}

void Demo3::Draw()
{
	// copy particles in and update the view
	auto particles = std::span(m_particles.begin(), std::min(m_particles.size(), m_particleBufferView.size()));
	if (!particles.empty())
	{
		std::copy(particles.begin(), particles.end(), m_particleBufferView.begin());
		auto particleView = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(particles.size(), sizeof(Particle));
		auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_shaderHeap->GetCPUDescriptorHandleForHeapStart());
		m_device->CreateShaderResourceView(m_particleBuffer.Get(), &particleView, cpuHandle);
	}

	// copy parameters
	m_commandList->SetGraphicsRoot32BitConstants(0, ArraySize(m_rootParams), &m_rootParams, 0);
	m_commandList->SetGraphicsRootDescriptorTable(1, m_shaderHeap->GetGPUDescriptorHandleForHeapStart());

	// draw the triangle
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->DrawInstanced(3, 1, 0, 0);
}

void InitDemoPalette()
{
	g_useFramebuffer = false;
	InitFileTable();
}

std::shared_ptr<Demo3> demo;
void InitDemo()
{
	demo = std::make_shared<Demo3>();
	demo->InitD3D12();
	demo->LoadAssets();
}

void DrawDemo()
{
	if (!g_paused)
	{
		demo->Update();
	}

	demo->PrepareFrame();
	demo->Draw();
	demo->FinishFrame();
}

void ShutdownDemo()
{
	demo.reset();
}
