#pragma once
#include "GoTheDistanceGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"

GoTheDistanceGameEngine::GoTheDistanceGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("gothedistance", "CIwTexture");
}

void GoTheDistanceGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());
	
	g_lineGraph.SetTotalPoints(this->GetGameLength() / 2000);
}

float GoTheDistanceGameEngine::End()
{
	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();
	
	double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);
	return 10.0f * MIN(1000, (float)distance);
}

void GoTheDistanceGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	if (ABS(g_lastTimeCheck - remainingTime) > 500)
	{
		g_lastTimeCheck = remainingTime;
		Utils::GetLocation(&g_currLoc);
	}

	double distance = LiveMaps::CalculateDistance(g_startLocation, g_currLoc);
	*pfScore = (float)distance;

	if ((g_lastTimeAdd - remainingTime) > 2000)
	{
		g_lastTimeAdd = remainingTime;
		g_lineGraph.AddPoint(*pfScore);
	}

	strcpy(szUnits, "meters");
}

void GoTheDistanceGameEngine::Render(CIwRect& bounds)
{
	CIwRect setBounds(bounds.x, bounds.y + 5, bounds.w, bounds.h - 10);
	g_lineGraph.SetBounds(setBounds);
	g_lineGraph.Render();
}

bool GoTheDistanceGameEngine::ShouldRenderScore()
{
	return true;
}