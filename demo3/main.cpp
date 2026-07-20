#include "demo3.h"

void Demo3::Draw()
{
	auto rtvHandle = GetRTVHandle(m_frameIndex);
	m_commandList->ClearRenderTargetView(rtvHandle, Vec4::BLUE, 0, nullptr);
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
