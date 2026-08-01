#include "demo3.h"

void Demo3::Init()
{
	m_waveOut = (HWAVEOUT)WAVE_MAPPER; // waveOutOpen(&m_waveOut, WAVE_MAPPER, nullptr, 0, 0, 0);

	// set volume of default output
	WORD vol = 0.33f * 0xFFFF;
	waveOutSetVolume(m_waveOut, (vol << 16) | vol);
}

void Demo3::SetupAssets()
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

	// initialize particles
	m_particles.resize(MAX_PARTICLES);
	m_smokeSimToScreen = Vec2(g_aspect, -1.0f);
	for (auto& particle : m_particles)
	{
		auto jitter = Vec2(UniformRandom(-1.0f), UniformRandom(-1.0f)) * 0.1;
		particle.pos = m_smokeSim.CellCentre(4, 12) * m_smokeSimToScreen + jitter;
	}

	// generate textures
	auto textureBuf = std::vector<uint32_t>(GENERATED_TEXTURE_WIDTH * GENERATED_TEXTURE_HEIGHT * GENERATED_TEXTURE_COUNT);
	auto texArrayCoord = [](uint32_t x, uint32_t y, uint32_t z) {
		return z * GENERATED_TEXTURE_WIDTH * GENERATED_TEXTURE_HEIGHT + y * GENERATED_TEXTURE_WIDTH + x;
	};

	float flashRadius = 0.35;
	float thickness = 0.04;
	uint32_t z = 0;
	for (uint32_t y = 0; y < GENERATED_TEXTURE_HEIGHT; y++)
	{
		for (uint32_t x = 0; x < GENERATED_TEXTURE_WIDTH; x++)
		{
			auto ray = Vec2((float)x / GENERATED_TEXTURE_WIDTH, (float)y / GENERATED_TEXTURE_HEIGHT) * 2.0f - Vec2(1.0f);
			bool circle = abs(ray.Length() - flashRadius) < thickness;
			bool cross = abs(ray.x) < thickness * 0.25f || abs(ray.y) < thickness * 2.5f;
			if (circle || cross)
			{
				textureBuf[texArrayCoord(x, y, z)] = 0xFFFFFFFF;
			}
		}
	}

	// upload character textures
	constexpr auto characterTextureSize = CHARACTER_TEXTURE_WIDTH * CHARACTER_TEXTURE_HEIGHT * sizeof(uint32_t);
	auto characterBlob = std::vector<byte>(characterTextureSize * CHARACTER_TEXTURE_COUNT);
	for (int i = 0; i < CHARACTER_TEXTURE_COUNT; i++)
	{
		char name[16] = {};
		snprintf(name, ArraySize(name), "pose%d.img", i);
		auto data = GetFile(name);
		std::copy(data.begin(), data.end(), characterBlob.begin() + i * characterTextureSize);
	}

	BeginTransfers();
	UploadData(std::span((byte*)textureBuf.data(), textureBuf.size() * sizeof(uint32_t)), m_generatedTextures);
	UploadData(characterBlob, m_characterTextures);
	UploadData(std::span(CreateFontAtlas(), FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT), m_fontTexture);

	std::array<CD3DX12_RESOURCE_BARRIER, 3> barriers = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_generatedTextures.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_characterTextures.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_fontTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)};
	m_transferCommandList->ResourceBarrier(barriers.size(), barriers.data());
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

	Pose pose = Pose::Jump;
	Vec2 characterPos =
		Vec2(0.0f, 0.2f + (m_scene == Scene::FountainStab ? -1 / (exp(progress - 4) + exp(-(progress - 4))) : 0.0f));
	if (m_sceneProgress > 0.6f || m_scene > Scene::FountainStab)
	{
		pose = Pose::Stab;
	}

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

	if (m_scene == Scene::FountainSmoke)
	{
		m_smokeSim.SmokeMap(4, 12) = 0.8f;
		m_smokeSim.VelocitiesY(4, 12) = 0.85f;

		m_smokeSim.RunPressureSolver(10);
		m_smokeSim.UpdateVelocities();

		for (uint32_t x = 0; x < m_smokeSim.CellCountX; x++)
		{
			for (uint32_t y = 0; y < m_smokeSim.CellCountY; y++)
			{
				auto& particle = m_particles[y * m_smokeSim.CellCountX + x];
				auto fac = (1.0f - (particle.pos.y * 0.5f + 0.5f)) * 0.5f;
				particle.pos += Vec2(m_smokeSim.VelocitiesX(x, y) * fac / 2, m_smokeSim.VelocitiesY(x, y)) * m_smokeSim.TimeStep *
								m_smokeSimToScreen;
				particle.size = m_smokeSim.SmokeMap(x, y) * fac;
				// if (particle.size > 0.6f)
				//{
				//	particle.size = 100.0f;
				//	m_sceneProgress = FLT_MAX;
				// }
			}
		}

		m_smokeSim.AdvectDye();
		m_smokeSim.AdvectVelocities();
	}

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
		RootParam::Int(pose),
		RootParam::Float(characterPos.x),
		RootParam::Float(characterPos.y),
	};
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
	demo->CreateResources();
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
