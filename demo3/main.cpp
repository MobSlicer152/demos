#include "demo3.h"

void Demo3::Draw()
{
	// generate particles
	m_particles.resize(0);
	for (uint32_t i = 0; i < 64; i++)
	{
		m_particles.push_back(Particle(Vec2(UniformRandom(-1.0), UniformRandom(-1.0)), 0.1 * UniformRandom()));
	}

	// copy particles in and update the view
	auto particles = std::span(m_particles.begin(), std::min(m_particles.size(), m_particleBufferView.size()));
	if (particles.size() > 0)
	{
		std::copy(particles.begin(), particles.end(), m_particleBufferView.begin());
		auto particleView = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(particles.size(), sizeof(Particle));
		auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_shaderHeap->GetCPUDescriptorHandleForHeapStart());
		m_device->CreateShaderResourceView(m_particleBuffer.Get(), &particleView, cpuHandle);
	}

	// set shader params
	float rootConstants[] = { g_aspect, g_elapsed, 1.0 };
	m_commandList->SetGraphicsRoot32BitConstants(0, ArraySize(rootConstants), &rootConstants, 0);
	m_commandList->SetGraphicsRootDescriptorTable(1, m_shaderHeap->GetGPUDescriptorHandleForHeapStart());

	// draw the quad
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->DrawInstanced(3, 1, 0, 0);
}

void InitDemoPalette()
{
	g_useFramebuffer = false;
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
	demo->PrepareFrame();
	demo->Draw();
	demo->FinishFrame();
}

void ShutdownDemo()
{
	demo.reset();
}
