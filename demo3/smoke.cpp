#include "smoke.h"

static float SampleBilinear(
	const std::vector<float>& map, int width, int height, float u, float v, bool wrap = false, bool debug = false)
{
	u -= 0.5f / width;
	v -= 0.5f / height;

	if (wrap)
	{
		u = fmodf(u, 1.0f);
		v = fmodf(v, 1.0f);
	}

	float px = u * (width);
	float py = v * (height);

	int x = (int)px;
	int y = (int)py;

	float xFrac = std::clamp(px - x, 0.0f, 1.0f);
	float yFrac = std::clamp(py - y, 0.0f, 1.0f);

	if (debug)
	{
		// Message(x + ", " + y + "  : uv = " + xFrac + ", " + yFrac + " ----------- uv in: " + u + ", " + v);
	}

	auto HandleBoundary = [&](int value, int dim) -> float { return wrap ? fmod(value, dim) : std::clamp(value, 0, dim - 1); };
	auto Sample = [&](int x, int y) -> float { return map[HandleBoundary(x, width) + HandleBoundary(y, height) * width]; };

	float bottomLeft = Sample(x, y);
	float bottomRight = Sample(x + 1, y);
	float topLeft = Sample(x, y + 1);
	float topRight = Sample(x + 1, y + 1);

	float interpolatedTop = Lerp(topLeft, topRight, xFrac);
	float interpolatedBottom = Lerp(bottomLeft, bottomRight, xFrac);
	return Lerp(interpolatedBottom, interpolatedTop, yFrac);
}

SmokeSimulation::SmokeSimulation(int cellCountX, int cellCountY, float cellSize)
	: CellCountX(cellCountX), CellCountY(cellCountY), CellSize(cellSize), boundsSize(Vec2(cellCountX, cellCountY) * CellSize),
	  bottomLeft(-boundsSize / 2), halfCellSize(CellSize / 2)
{
	auto mapSize = cellCountX * cellCountY;
	auto mapSizeX = (cellCountX + 1) * cellCountY;
	auto mapSizeY = cellCountX * (cellCountY + 1);

	VelocitiesX.resize(mapSizeX);
	VelocitiesX_Temp.resize(mapSizeX);

	VelocitiesY.resize(mapSizeY);
	VelocitiesY_Temp.resize(mapSizeY);

	PressureMap.resize(mapSize);
	SmokeMap.resize(mapSize);
	SmokeMapTemp.resize(mapSize);

	SolidCellMap.resize(mapSize);
	PressureSolveDataMap.resize(mapSize);

	// make borders solid
	for (int x = 0; x < cellCountX; x++)
	{
		SolidCellMap[CellIndex(x, 0)] = true;
		SolidCellMap[CellIndex(x, cellCountY - 1)] = true;
	}

	for (int y = 0; y < cellCountY; y++)
	{
		SolidCellMap[CellIndex(0, y)] = true;
		SolidCellMap[CellIndex(cellCountX - 1, y)] = true;
	}
}

void SmokeSimulation::RunPressureSolver(int iterations)
{
	PreparePressureSolver();

	for (int i = 0; i < iterations; i++)
	{
		PressureSolve();
	}
}

void SmokeSimulation::UpdateVelocities()
{

	float K = TimeStep / (Density * CellSize);

	// ---- Horizontal ----
	for (int x = 0; x < CellCountX + 1; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			if (IsSolid(x, y) || IsSolid(x - 1, y))
			{
				continue;
			}

			float pressureRight = GetPressure(x, y);
			float pressureLeft = GetPressure(x - 1, y);
			VelocitiesX[CellIndex(x, y)] -= K * (pressureRight - pressureLeft);
		}
	}

	// ---- Vertical ----

	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY + 1; y++)
		{
			if (IsSolid(x, y) || IsSolid(x, y - 1))
			{
				continue;
			}

			float pressureTop = GetPressure(x, y);
			float pressureBottom = GetPressure(x, y - 1);
			VelocitiesY[CellIndex(x, y)] -= K * (pressureTop - pressureBottom);
		}
	}
}

void SmokeSimulation::AdvectVelocities()
{
	// Horizontal
	for (int x = 0; x < CellCountX + 1; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			if (IsSolid(x - 1, y) || IsSolid(x, y))
			{
				VelocitiesX_Temp[CellIndex(x, y)] = VelocitiesX[CellIndex(x, y)];
				continue;
			}

			Vec2 pos = LeftEdgeCentre(x, y);
			Vec2 vel = GetVelocityAtWorldPos(pos);
			Vec2 posPrev = pos - vel * TimeStep;
			VelocitiesX_Temp[CellIndex(x, y)] = GetVelocityAtWorldPos(posPrev).x;
		}
	}

	// Vertical
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY + 1; y++)
		{
			if (IsSolid(x, y - 1) || IsSolid(x, y))
			{
				VelocitiesY_Temp[CellIndex(x, y)] = VelocitiesY[CellIndex(x, y)];
				continue;
			}

			Vec2 pos = BottomEdgeCentre(x, y);
			Vec2 vel = GetVelocityAtWorldPos(pos);
			Vec2 posPrev = pos - vel * TimeStep;
			VelocitiesY_Temp[CellIndex(x, y)] = GetVelocityAtWorldPos(posPrev).y;
		}
	}

	UpdateVelocitiesFromTemporary();
}

void SmokeSimulation::AdvectDye()
{
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			if (IsSolid(x - 1, y) || IsSolid(x, y))
				continue;
			Vec2 pos = CellCentre(x, y);
			Vec2 vel = GetVelocityAtWorldPos(pos);
			Vec2 posPrev = pos - vel * TimeStep;

			float tx = (posPrev.x - bottomLeft.x) / (boundsSize.x);
			float ty = (posPrev.y - bottomLeft.y) / (boundsSize.y);
			float amount = ::SampleBilinear(SmokeMap, CellCountX, CellCountY, tx, ty, false);

			SmokeMapTemp[CellIndex(x, y)] = amount;
		}
	}

	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			SmokeMap[CellIndex(x, y)] = SmokeMapTemp[CellIndex(x, y)];
		}
	}
}

float SmokeSimulation::SampleBilinear(const std::vector<float>& edgeValues, int edgeCountX, int edgeCountY, float cellSize, Vec2 worldPos)
{
	float width = (edgeCountX - 1) * cellSize;
	float height = (edgeCountY - 1) * cellSize;

	// Calculate indices of each edge for the current cell
	float px = (worldPos.x + width / 2) / cellSize;	 // [0, countX]
	float py = (worldPos.y + height / 2) / cellSize; // [0, countY]

	int left = std::clamp((int)px, 0, edgeCountX - 2);
	int bottom = std::clamp((int)py, 0, edgeCountY - 2);
	int right = left + 1;
	int top = bottom + 1;

	// Calculate how far [0, 1] the input point is along the current cell
	float xFrac = std::clamp(px - left, 0.0f, 1.0f);
	float yFrac = std::clamp(py - bottom, 0.0f, 1.0f);

	auto HandleBoundary = [&](int value, int dim) -> float { return std::clamp(value, 0, dim - 1); };
	auto Sample = [&](int x, int y) -> float {
		return edgeValues[HandleBoundary(x, width) + HandleBoundary(y, height) * width];
	};

	// Bilinear interpolation
	float valueTop = Lerp(Sample(left, top), Sample(right, top), xFrac);
	float valueBottom = Lerp(Sample(left, bottom), Sample(right, bottom), xFrac);
	return Lerp(valueBottom, valueTop, yFrac);
}

Vec2 SmokeSimulation::GetVelocityAtWorldPos(Vec2 worldPos) const
{
	float velX = SampleBilinear(VelocitiesX, CellCountX + 1, CellCountY, CellSize, worldPos);
	float velY = SampleBilinear(VelocitiesY, CellCountX, CellCountY + 1, CellSize, worldPos);
	return Vec2(velX, velY);
}

float SmokeSimulation::CalculateVelocityDivergenceAtCell(int cellX, int cellY)
{
	// Get velocities at each edge of cell
	float velocityTop = VelocitiesY[CellIndex(cellX + 0, cellY + 1)];
	float velocityLeft = VelocitiesX[CellIndex(cellX + 0, cellY + 0)];
	float velocityRight = VelocitiesX[CellIndex(cellX + 1, cellY + 0)];
	float velocityBottom = VelocitiesY[CellIndex(cellX + 0, cellY + 0)];

	// Calculate how fast the fluid's velocity is changing across this cell on either axis
	float gradientX = (velocityRight - velocityLeft) / CellSize; // finite-difference approximation of du/dx
	float gradientY = (velocityTop - velocityBottom) / CellSize; // finite-difference approximation of du/dy
	// Sum to calculate if more fluid is entering (divergence < 0) or exiting the cell (divergence > 0)
	float divergence = gradientX + gradientY;
	return divergence;
}

void SmokeSimulation::ClearDye()
{
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			SmokeMap[CellIndex(x, y)] = 0;
		}
	}
}

void SmokeSimulation::ClearVelocities()
{
	for (int x = 0; x < CellCountX + 1; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			VelocitiesX[CellIndex(x, y)] = 0;
		}
	}

	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY + 1; y++)
		{
			VelocitiesY[CellIndex(x, y)] = 0;
		}
	}

	// pressure
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			PressureMap[CellIndex(x, y)] = 0;
		}
	}
}

void SmokeSimulation::PressureSolve()
{
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			float newPressure;

			PressureSolveData info = PressureSolveDataMap[CellIndex(x, y)];

			if (info.solid || info.flowEdgeCount == 0)
				newPressure = 0;
			else
			{
				float pressureTop = PressureMap[CellIndex(x, std::min(y + 1, CellCountY - 1))] * info.flowTop;
				float pressureLeft = PressureMap[CellIndex(std::max(x - 1, 0), y)] * info.flowLeft;
				float pressureRight = PressureMap[CellIndex(std::min(x + 1, CellCountX - 1), y)] * info.flowRight;
				float pressureBottom = PressureMap[CellIndex(x, std::max(y - 1, 0))] * info.flowBottom;

				float pressureSum = pressureRight + pressureLeft + pressureTop + pressureBottom;
				newPressure = (pressureSum - Density * CellSize * info.velocityTerm) / info.flowEdgeCount;
			}

			// SOR update
			float oldPressure = PressureMap[CellIndex(x, y)];
			PressureMap[CellIndex(x, y)] = oldPressure + (newPressure - oldPressure) * SOR;
		}
	}
}

void SmokeSimulation::UpdateVelocitiesFromTemporary()
{
	for (int x = 0; x < CellCountX + 1; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			VelocitiesX[CellIndex(x, y)] = VelocitiesX_Temp[CellIndex(x, y)];
		}
	}

	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY + 1; y++)
		{
			VelocitiesY[CellIndex(x, y)] = VelocitiesY_Temp[CellIndex(x, y)];
		}
	}
}

void SmokeSimulation::PreparePressureSolver()
{
	for (int x = 0; x < CellCountX; x++)
	{
		for (int y = 0; y < CellCountY; y++)
		{
			float flowTop = IsSolid(x + 0, y + 1) ? 0.0f : 1.0f;
			float flowLeft = IsSolid(x - 1, y + 0) ? 0.0f : 1.0f;
			float flowRight = IsSolid(x + 1, y + 0) ? 0.0f : 1.0f;
			float flowBottom = IsSolid(x + 0, y - 1) ? 0.0f : 1.0f;
			int fluidEdgeCount = flowLeft + flowRight + flowTop + flowBottom;
			bool isSolid = IsSolid(x, y);

			float velocityTop = VelocitiesY[CellIndex(x + 0, y + 1)];
			float velocityLeft = VelocitiesX[CellIndex(x + 0, y + 0)];
			float velocityRight = VelocitiesX[CellIndex(x + 1, y + 0)];
			float velocityBottom = VelocitiesY[CellIndex(x + 0, y + 0)];

			float velTerm = (velocityRight - velocityLeft + velocityTop - velocityBottom) / TimeStep;

			PressureSolveDataMap[CellIndex(x, y)] = PressureSolveData {
				flowLeft,
				flowRight,
				flowTop,
				flowBottom,
				fluidEdgeCount,
				isSolid,
				velTerm};
		}
	}
}
