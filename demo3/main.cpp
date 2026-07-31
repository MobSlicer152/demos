#include "demo3.h"

void Demo3::Init()
{
	m_waveOut = (HWAVEOUT)WAVE_MAPPER; // waveOutOpen(&m_waveOut, WAVE_MAPPER, nullptr, 0, 0, 0);

	// set volume of default output
	WORD vol = 0.33f * 0xFFFF;
	waveOutSetVolume(m_waveOut, (vol << 16) | vol);
}

void Demo3::GenerateAssets()
{
	// load sounds
	for (size_t i = 0; i < m_sceneSounds.size(); i++)
	{
		auto name = SCENE_SOUND_NAMES[i];
		if (name)
		{
			m_sceneSounds[i] = GetFile(name);
		}
	}

	// generate textures
	auto textureBuf = std::vector<uint32_t>(TEXTURE_ARRAY_WIDTH * TEXTURE_ARRAY_HEIGHT * TEXTURE_ARRAY_SIZE);
	auto texArrayCoord = [](uint32_t x, uint32_t y, uint32_t z) {
		return z * TEXTURE_ARRAY_WIDTH * TEXTURE_ARRAY_HEIGHT + y * TEXTURE_ARRAY_WIDTH + x;
	};

	auto rhombus = [](Vec2 p, Vec2 b) {
		b.y = -b.y;
		p = Vec2(abs(p.x), abs(p.y));
		float h = std::clamp(b.Dot(p) + b.y * b.y / b.Dot(b), 0.0f, 1.0f);
		p -= b * Vec2(h, h - 1.0f);
		return std::copysign(p.Length(), p.x);
	};

	// replication of spr_kris_make_fountain_flash of dubious quality
	float flashRadius = 0.35;
	float thickness = 0.065;
	uint32_t z = 0;
	for (uint32_t y = 0; y < TEXTURE_ARRAY_HEIGHT; y++)
	{
		for (uint32_t x = 0; x < TEXTURE_ARRAY_WIDTH; x++)
		{
			auto ray = Vec2((float)x / TEXTURE_ARRAY_WIDTH, (float)y / TEXTURE_ARRAY_HEIGHT) * 2.0f - Vec2(1.0f);
			bool circle = abs(ray.Length() - flashRadius) < thickness;
			bool cross = abs(ray.x) < thickness * 0.25f || abs(ray.y) < thickness * 2.5f;
			if (circle || cross)
			{
				textureBuf[texArrayCoord(x, y, z)] = 0xFFFFFFFF;
			}
		}
	}

	BeginTransfers();
	UploadData(std::span((byte*)textureBuf.data(), textureBuf.size() * sizeof(uint32_t)), m_generatedTextures);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_generatedTextures.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_transferCommandList->ResourceBarrier(1, &barrier);
	SubmitTransfers();
}

void Demo3::Update()
{
	if (g_paused)
	{
		return;
	}

	// progress scene
	m_sceneProgress += g_delta;
	if (m_sceneProgress > SCENE_LENGTHS[m_scene])
	{
		m_scene = (Scene)(m_scene + 1);
		if (m_scene >= Scene::SceneCount)
		{
			g_running = false;
			return;
		}
		m_sceneProgress = 0.0f;

		auto sound = m_sceneSounds[m_scene];
		if (!sound.empty() && SCENE_LENGTHS[m_scene] > 0.0f)
		{
			sndPlaySoundA((LPCSTR)sound.data(), SND_ASYNC | SND_MEMORY | SND_NODEFAULT | SND_NOWAIT);
		}
	}

	// set shader params
	float progress = m_sceneProgress * SCENE_SPEEDS[m_scene];

	float ellipseProgress = std::clamp(progress, 0.0f, 4.0f);
	Vec2 ellipsePos = Vec2(0.0f, -ellipseProgress + 0.5f);
	Vec2 ellipseSize = Vec2(std::clamp(-0.3f * ellipseProgress + 0.5f, 0.4f, 1.0f), ellipseProgress);

	// only do brightness if fountain is growing
	float brightnessProgress = 0.0f;
	if (m_scene == Scene::FountainGrow)
	{
		// stall progress around the middle
		brightnessProgress = progress;
		if (m_sceneProgress >= 2.7f)
		{
			brightnessProgress -= 1.5f;
		}
		else if (m_sceneProgress >= 0.7f)
		{
			brightnessProgress = 0.525f;
		}
	}

	float jitter = std::clamp(exp(progress * 6.0f), 0.0f, 40.0f) * sin(progress * 100.0f + 0.5f) * 0.0005f;
	float fountainSize = std::clamp(progress / 2 + 0.2f, 0.0f, 0.8f) + jitter;
	if (progress >= 4.6f) // close the fountain after it peaks
	{
		fountainSize = std::clamp(exp(-10 * (progress - 4.56f)) - 0.1f, 0.0f, 0.8f);
	}

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

	float brightness = 0.0f;
	float fountainBrightness = 1.0f;
	if (m_scene == Scene::FountainGrow)
	{
		brightness = backgroundBrightness(brightnessProgress);
		fountainBrightness = foregroundBrightness(brightnessProgress);
	}
	int32_t smokeLayers = 4;

	m_rootParams = {
		RootParam::Int(m_scene),
		RootParam::Float(g_aspect),
		RootParam::Float(g_elapsed),
		RootParam::Float(progress),
		RootParam::Float(ellipsePos.x),
		RootParam::Float(ellipsePos.y),
		RootParam::Float(ellipseSize.x),
		RootParam::Float(ellipseSize.y),
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
		m_device->CreateShaderResourceView(m_particleBuffer.Get(), &particleView, m_particleBufferHandle);
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
	InitStandardPalette();
}

std::shared_ptr<Demo3> demo;
void InitDemo()
{
	demo = std::make_shared<Demo3>();
	demo->Init();
	demo->InitD3D12();
	demo->LoadAssets();
}

void DrawDemo()
{
	demo->Update();
	demo->PrepareFrame();
	demo->Draw();
	demo->FinishFrame();
}

void ShutdownDemo()
{
	demo.reset();
}
