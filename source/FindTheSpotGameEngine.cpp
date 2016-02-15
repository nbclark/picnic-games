#pragma once
#include "FindTheSpotGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"
#include "IwNotificationHandler.h"
#include "IwMultiPlayerHandler.h"

FindTheSpotGameEngine::FindTheSpotGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("findthespot", "CIwTexture");
	g_pHotterTexture = (CIwTexture*)IwGetResManager()->GetResNamed("hotter", "CIwTexture");
	g_pColderTexture = (CIwTexture*)IwGetResManager()->GetResNamed("colder", "CIwTexture");
	g_pFoundTexture = (CIwTexture*)IwGetResManager()->GetResNamed("found", "CIwTexture");
	g_pCursorTexture = (CIwTexture*)IwGetResManager()->GetResNamed("cursor", "CIwTexture");
	g_pTile = NULL;
}

FindTheSpotGameEngine::~FindTheSpotGameEngine()
{
	if (g_pTile)
	{
		delete g_pTile;
		g_pTile = NULL;
	}
}

void FindTheSpotGameEngine::Prepare()
{
	if (g_pTile)
	{
		delete g_pTile;
		g_pTile = NULL;
	}

	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());

	IGameHandler* pHandler = (IGameHandler*)g_pGameHandler;
	Region* pRegion = pHandler->GetBoundingRegion();

	// Calculate a random location within a given radius
	int tryCount = 20;
	do
	{
		double angle = PI / 180.0 * IwRandMinMax(-180, 180); // 360 degree range
		int32 radius = IwRandMinMax(25, 50); // 50 meters

		LiveMaps::CalculateLatLongInDirection(&g_startLocation, radius, angle, &g_randLocation);

		if (pRegion->Contains(g_randLocation))
		{
			break;
		}
	} while (tryCount-- > 0);

	int acx = LiveMaps::LongitudeToXAtZoom(g_randLocation.m_Longitude, LiveMaps::MaxZoom);
	int acy = LiveMaps::LatitudeToYAtZoom(g_randLocation.m_Latitude, LiveMaps::MaxZoom);

	int tx = acx / 256;
	int ty = acy / 256;

	int x = tx * 256;
	int y = ty * 256;

	g_tileLoc.x = (acx - x);
	g_tileLoc.y = (acy - y);

	char szQuad[20];
    int server = 0;
	LiveMaps::TileToQuadKey(szQuad, tx, ty, LiveMaps::MaxZoom);

	char szImageUrl[256];
	sprintf(szImageUrl, "http://r%i.ortho.tiles.virtualearth.net/tiles/%s%s.%s?g=22", server, "h", szQuad, "jpg");

	g_topLeft.m_Longitude = LiveMaps::XToLongitudeAtZoom(x, LiveMaps::MaxZoom);
	g_topLeft.m_Latitude = LiveMaps::YToLatitudeAtZoom(y, LiveMaps::MaxZoom);

	g_botRight.m_Longitude = LiveMaps::XToLongitudeAtZoom(x+256, LiveMaps::MaxZoom);
	g_botRight.m_Latitude = LiveMaps::YToLatitudeAtZoom(y+256, LiveMaps::MaxZoom);

	Utils::DownloadMapTile(&g_pTile, szImageUrl, true);
}

void FindTheSpotGameEngine::Start()
{
	g_lastTimeCheck = 0;
	g_renderTemp = 0;
	g_distance = DBL_MAX;
	g_finalScore = 0;
}

float FindTheSpotGameEngine::End()
{
	if (g_pTile)
	{
		delete g_pTile;
		g_pTile = NULL;
	}

	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	double distance = LiveMaps::CalculateDistance(g_randLocation, g_currLoc);

	if (g_finalScore)
	{
		return 1000 + (g_finalScore / GetGameLength()) * 9000;
	}
	else
	{
		// 1000 points if we get close
		return MIN(1000, 1000-distance);
	}
}

void FindTheSpotGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	if (g_finalScore != 0)
	{
		g_renderTemp = 0;
		*pfScore = 0;
	}
	else
	{
		if (ABS(g_lastTimeCheck - remainingTime) > 500)
		{
			g_lastTimeCheck = remainingTime;
			Utils::GetLocation(&g_currLoc);

			g_renderTemp = 0;
		}

		double distance = ABS(LiveMaps::CalculateDistance(g_randLocation, g_currLoc));
		*pfScore = (float)distance;

		if (distance > g_distance)
		{
			// colder
			g_renderTemp = 1;
		}
		else if (distance < g_distance)
		{
			// hotter
			g_renderTemp = 2;
		}

		// Under 2 meters
		if (distance < 2)
		{
			g_finalScore = (GetGameLength() - (remainingTime));
		}

		g_distance = distance;
	}
	strcpy(szUnits, "meters");
}

void FindTheSpotGameEngine::Render(CIwRect& bounds)
{
	float tileSize = 160.0f;
	float scale = Utils::GetImageScalingFactor();

	if (g_finalScore == 0)
	{
		IwGxFlush();

		tileSize = bounds.w;
		CIwRect tileBounds(bounds.x + (bounds.w - tileSize) / 2, bounds.y + (bounds.h - tileSize) / 2, tileSize, tileSize);
		Utils::AlphaRenderImage(g_pTile, tileBounds, 255);

		CIwColour white;
		white.Set(0xff, 0xff, 0xff, 0x7f);

		float scale = tileSize / 256.0f;

		Iw2DSetColour(white);
		Iw2DDrawRect(CIwSVec2(tileBounds.x-1, tileBounds.y-1), CIwSVec2(tileBounds.w+2, tileBounds.h+2));

		IwGxFlush();

		int cursorWidth = g_pCursorTexture->GetWidth() * scale;

		CIwRect spotLoc(tileBounds.x + (g_tileLoc.x * scale) - (cursorWidth / 2), tileBounds.y + (g_tileLoc.y * scale) - (cursorWidth / 2), cursorWidth, cursorWidth);
		Utils::AlphaRenderImage(g_pCursorTexture, spotLoc, 255);
		
		if (g_renderTemp == 1)
		{
			int colderHeight = g_pColderTexture->GetHeight() * scale;
			int colderWidth = g_pColderTexture->GetWidth() * scale;

			CIwRect imgBounds(bounds.x + (bounds.w - colderWidth) / 2, bounds.y + bounds.h - colderHeight, colderWidth, colderHeight);
			Utils::AlphaRenderImage(g_pColderTexture, imgBounds, 255);
		}
		else if (g_renderTemp == 2)
		{
			int hotterHeight = g_pHotterTexture->GetHeight() * scale;
			int hotterWidth = g_pHotterTexture->GetWidth() * scale;

			CIwRect imgBounds(bounds.x + (bounds.w - hotterWidth) / 2, bounds.y + bounds.h - hotterHeight, hotterWidth, hotterHeight);
			Utils::AlphaRenderImage(g_pHotterTexture, imgBounds, 255);
		}
	}
	else
	{
		tileSize = g_pFoundTexture->GetWidth() * scale;
		CIwRect tileBounds(bounds.x + (bounds.w - tileSize) / 2, bounds.y + (bounds.h - tileSize) / 2, tileSize, tileSize);

		Utils::AlphaRenderImage(g_pFoundTexture, tileBounds, 255);
	}
}

bool FindTheSpotGameEngine::ShouldRenderScore()
{
	return true;
}