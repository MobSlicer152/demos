#pragma once

#include "../demolib/demolib.h"
#include "vector2d.h"
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

	Vector2D<float> VelocitiesX;
	Vector2D<float> VelocitiesY;
	Vector2D<float> VelocitiesX_Temp;
	Vector2D<float> VelocitiesY_Temp;

	Vector2D<byte> SolidCellMap;
	Vector2D<PressureSolveData> PressureSolveDataMap;

	Vector2D<float> PressureMap;
	Vector2D<float> SmokeMap;
	Vector2D<float> SmokeMapTemp;

	float TimeStep = 1 / 60.0f;
	float Density = 2.0f;
	float SOR = 1.7f;

	SmokeSimulation(int cellCountX, int cellCountY, float cellSize);

	Vec2 CellCentre(int x, int y) const
	{
		return bottomLeft + Vec2(x + 0.5f, y + 0.5f) * CellSize;
	}

	void RunPressureSolver(int iterations);
	void UpdateVelocities();
	void AdvectVelocities();
	void AdvectDye();
	static float SampleBilinear(
		const Vector2D<float>& edgeValues, float cellSize, Vec2 worldPos);
	Vec2 GetVelocityAtWorldPos(Vec2 worldPos) const;
	float CalculateVelocityDivergenceAtCell(int cellX, int cellY);
	void ClearDye();
	void ClearVelocities();

	bool IsSolid(int x, int y) const
	{
		return SolidCellMap.AtClamped(x, y);
	}

	float GetPressure(int x, int y) const
	{
		return PressureMap.AtClamped(x, y);
	}

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

	void PreparePressureSolver();
};
