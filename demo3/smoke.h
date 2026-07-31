#pragma once

#include "../demolib/demolib.h"
#include <algorithm>
#include <array>
#include <vector>



// yoinked from sebastian lague cause he had time to read a paper
// https://github.com/SebLague/Smoke-Simulation/blob/main/Assets/SmokeCPU/FluidGrid.cs
class SmokeSimulation
{
  public:
	struct PressureSolveData
	{
		float flowLeft;
		float flowRight;
		float flowTop;
		float flowBottom;
		int flowEdgeCount;
		bool solid;
		float velocityTerm;
	};

	int CellCountX;
	int CellCountY;
	float CellSize;

	std::vector<float> VelocitiesX;
	std::vector<float> VelocitiesY;
	std::vector<float> VelocitiesX_Temp;
	std::vector<float> VelocitiesY_Temp;

	std::vector<bool> SolidCellMap;
	std::vector<PressureSolveData> PressureSolveDataMap;

	std::vector<float> PressureMap;
	std::vector<float> SmokeMap;
	std::vector<float> SmokeMapTemp;

	float TimeStep = 1 / 60.0f;
	float Density = 1.0f;
	float SOR = 1.7f;

	SmokeSimulation(int cellCountX, int cellCountY, float cellSize);

	Vec2 CellCentre(int x, int y) const
	{
		return bottomLeft + Vec2(x + 0.5f, y + 0.5f) * CellSize;
	}

	int CellIndex(int x, int y) const
	{
		return std::clamp(y, 0, CellCountY - 1) * CellCountX + std::clamp(x, 0, CellCountX - 1);
	}

	void RunPressureSolver(int iterations);
	void UpdateVelocities();
	void AdvectVelocities();
	void AdvectDye();
	static float SampleBilinear(
		const std::vector<float>& edgeValues, int edgeCountX, int edgeCountY, float cellSize, Vec2 worldPos);
	Vec2 GetVelocityAtWorldPos(Vec2 worldPos) const;
	float CalculateVelocityDivergenceAtCell(int cellX, int cellY);
	void ClearDye();
	void ClearVelocities();

  private:
	Vec2 boundsSize;
	Vec2 bottomLeft;
	float halfCellSize;

	Vec2 LeftEdgeCentre(int x, int y) const
	{
		return CellCentre(x, y) - Vec2(halfCellSize, 0.0f);
	}

	Vec2 BottomEdgeCentre(int x, int y) const
	{
		return CellCentre(x, y) - Vec2(0.0f, halfCellSize);
	}

	void PressureSolve();
	void UpdateVelocitiesFromTemporary();

	bool IsSolid(int x, int y) const
	{
		return SolidCellMap[CellIndex(x, y)];
	}

	float GetPressure(int x, int y) const
	{
		return PressureMap[CellIndex(x, y)];
	}

	void PreparePressureSolver();
};
