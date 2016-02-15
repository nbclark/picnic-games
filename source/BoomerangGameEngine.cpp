#pragma once
#include "BoomerangGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"

BoomerangGameEngine::BoomerangGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("boomerang", "CIwTexture");
}

void BoomerangGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());

	g_lineGraph.SetTotalPoints(this->GetGameLength() / 2000);

	g_maxDistance = 0;
}

float BoomerangGameEngine::End()
{
	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();
	
	// If we got back to our starting spot, you automatically do the best
	// Otherwise, your score is how far out you get, minus how far you get back
	double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);

	if (distance < 5)
	{
		return (9000 + MIN(1000, g_maxDistance));
	}
	else
	{
		return 9 * MIN(1000, ABS(g_maxDistance - distance));
	}
}

void BoomerangGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	s3eLocation curLoc;
	Utils::GetLocation(&curLoc);

	double distance = LiveMaps::CalculateDistance(g_startLocation, curLoc);
	g_maxDistance = MAX(g_maxDistance, distance);

	*pfScore = (float)g_maxDistance;

	if ((g_lastTimeAdd - remainingTime) > 2000)
	{
		g_lastTimeAdd = remainingTime;
		g_lineGraph.AddPoint(*pfScore);
	}

	strcpy(szUnits, "meters");
}

void BoomerangGameEngine::Render(CIwRect& bounds)
{
	CIwRect setBounds(bounds.x, bounds.y + 5, bounds.w, bounds.h - 10);
	g_lineGraph.SetBounds(setBounds);
	g_lineGraph.Render();
}

bool BoomerangGameEngine::ShouldRenderScore()
{
	return true;
}