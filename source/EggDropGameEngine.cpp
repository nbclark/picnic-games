#pragma once
#include "EggDropGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"
#include "s3e.h"

float rot, fallingRot;
float g_finalScore = 0;
bool g_bBroken = false;
int g_iCrackIter = 0;

EggDropGameEngine::EggDropGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("eggdrop", "CIwTexture");
	g_pEgg = (CIwTexture*)IwGetResManager()->GetResNamed("egg", "CIwTexture");
	g_pSpoon = (CIwTexture*)IwGetResManager()->GetResNamed("spoon", "CIwTexture");
	g_pCrackedEgg = (CIwTexture*)IwGetResManager()->GetResNamed("crackedegg", "CIwTexture");
	g_pGrass = (CIwTexture*)IwGetResManager()->GetResNamed("grass", "CIwTexture");

	g_animCount = Utils::FPS;
}

void EggDropGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());
	
	s3eAccelerometerStart();
	g_bBroken = false;
	g_finalScore = 0;
	g_iCrackIter = 0;
}

float EggDropGameEngine::End()
{
	s3eAccelerometerStop();

	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();

	if (g_finalScore)
	{
		// we finished -- our score is the # of seconds remaining
		// The max score is 10,000. If you complete, you get 1,000 minimum.
		return 1000 + (g_finalScore / GetGameLength()) * 9000;
	}
	else if (g_bBroken)
	{
		// If we broke, we get 0
		return 0;
	}
	else
	{
		// if we didn't complete, the final score is 1000 - the distance remaining

		double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);
		return 1000 - (float)distance;
	}
}

void EggDropGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	s3eLocation curLoc;
	Utils::GetLocation(&curLoc);

	int requiredDistance = 50;

	double distance = LiveMaps::CalculateDistance(g_startLocation, curLoc);

	// Get the accelerometer values here...
	int32 x = s3eAccelerometerGetX();
	int32 y = s3eAccelerometerGetY();
	int32 z = s3eAccelerometerGetZ();

	if (g_finalScore != 0)
	{
		*pfScore = 0;
	}
	else if (g_bBroken)
	{
		*pfScore = 0;
		g_finalScore = -1;
	}
	else if (ABS(x) > 450)
	{
		*pfScore = 0;

		// we break;
		g_bBroken = true;
		g_iCrackIter = 0;
		fallingRot = rot;

		g_finalScore = (distance - requiredDistance);
	}
	else if (distance > requiredDistance)
	{
		g_finalScore = (GetGameLength() - remainingTime);
		*pfScore = 0;
	}
	else
	{
		*pfScore = (requiredDistance - distance);
		rot = -(x / 1000.0) / 2;
	}

	strcpy(szUnits, "meters remaining");
}

void EggDropGameEngine::RenderInfo(CIwRect& bounds)
{
	IGameEngine::RenderInfo(bounds);
}

void EggDropGameEngine::Render(CIwRect& bounds)
{
	CIwRect drawGrass(bounds.x, bounds.y, bounds.w, bounds.h);
	Utils::AlphaRenderImage(g_pGrass, drawGrass, 0xff);

	float scale = Utils::GetImageScalingFactor();
	int spoonHeight = g_pSpoon->GetHeight() * scale;
	int spoonWidth = g_pSpoon->GetWidth() * scale;
	int crackedEggHeight = g_pCrackedEgg->GetHeight() * scale;
	int crackedEggWidth = g_pCrackedEgg->GetWidth() * scale;

	if (spoonHeight > bounds.h)
	{
		scale = bounds.h / (float)g_pSpoon->GetHeight();
		spoonHeight = g_pSpoon->GetHeight() * scale;
		spoonWidth = g_pSpoon->GetWidth() * scale;
	}

	int eggHeight = g_pEgg->GetHeight() * scale;
	int eggWidth = g_pEgg->GetWidth() * scale;

	if (g_bBroken)
	{
		if (g_iCrackIter < g_animCount)
		{
			int yOffset = (spoonHeight - eggHeight) * g_iCrackIter / (float)g_animCount;
			CIwRect bounds2(bounds.x + (bounds.w - eggWidth) / 2, bounds.y + bounds.h - spoonHeight + yOffset, eggWidth, eggHeight);

			double fallRot = fallingRot + (g_iCrackIter * 2 / (float)g_animCount);

			Utils::AlphaRenderAndRotateImage(g_pEgg, bounds2, 0xff, fallRot);

			g_iCrackIter++;
		}
		else
		{
			CIwRect draw(bounds.x + (bounds.w - crackedEggWidth) / 2, bounds.y + (bounds.h - crackedEggHeight), crackedEggWidth, crackedEggHeight);
			Utils::AlphaRenderImage(g_pCrackedEgg, draw, 0xff);
		}
	}
	else
	{
		CIwRect draw(bounds.x + (bounds.w - spoonWidth) / 2, bounds.y + bounds.h - spoonHeight, spoonWidth, spoonHeight);
		Utils::AlphaRenderImage(g_pSpoon, draw, 0xff);

		IwGxFlush();

		CIwRect bounds2(bounds.x + (bounds.w - eggWidth) / 2, bounds.y + bounds.h - spoonHeight, eggWidth, eggHeight);
		Utils::AlphaRenderAndRotateImage(g_pEgg, bounds2, 0xff, rot);
	}
}

bool EggDropGameEngine::ShouldRenderScore()
{
	return true;
}