#pragma once
#include "CaptureGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"
#include "IwNotificationHandler.h"
#include "IwMultiPlayerHandler.h"

#define CAPTURE_GAME_COUNT	15

CaptureGameEngine::CaptureGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("Capture", "CIwTexture");
	g_pUserTexture = (CIwTexture*)IwGetResManager()->GetResNamed("user", "CIwTexture");
	g_pCursorTexture = (CIwTexture*)IwGetResManager()->GetResNamed("cursor", "CIwTexture");
	
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			g_pTiles[i][j] = NULL;
		}
	}
}

CaptureGameEngine::~CaptureGameEngine()
{
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			if (g_pTiles[i][j])
			{
				delete g_pTiles[i][j];
				g_pTiles[i][j] = NULL;
			}
		}
	}

	for (int i = 0; i < g_pCaptures.size(); ++i)
	{
		delete g_pCaptures[i];
	}
	g_pCaptures.clear();
}

void CaptureGameEngine::Prepare()
{
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());

	// get the 9 tiles around our location
	int acx = LiveMaps::LongitudeToXAtZoom(g_startLocation.m_Longitude, LiveMaps::MaxZoom);
	int acy = LiveMaps::LatitudeToYAtZoom(g_startLocation.m_Latitude, LiveMaps::MaxZoom);

	int tx = acx / 256;
	int ty = acy / 256;

	int x = tx * 256;
	int y = ty * 256;

	g_tileLoc.x = (acx - x);
	g_tileLoc.y = (acy - y);

	char szQuad[20];
	char szImageUrl[256];

	g_topLeft.x = x;// + 128;
	g_topLeft.y = y;// + 128;

	for (int ttx = tx - 1; ttx <= tx + 1; ++ttx)
	{
		for (int tty = ty - 1; tty <= ty + 1; ++tty)
		{
			int server = 0;
			LiveMaps::TileToQuadKey(szQuad, ttx, tty, LiveMaps::MaxZoom);

			sprintf(szImageUrl, "http://r%i.ortho.tiles.virtualearth.net/tiles/%s%s.%s?g=22", server, "h", szQuad, "jpg");

			g_pTiles[1 + (ttx-tx)][1 + (tty-ty)] = 0;
			if (!Utils::DownloadMapTile(&g_pTiles[1 + (ttx-tx)][1 + (tty-ty)], szImageUrl, true))
			{
				if (g_pTiles[1 + (ttx-tx)][1 + (tty-ty)])
				{
					delete g_pTiles[1 + (ttx-tx)][1 + (tty-ty)];
					g_pTiles[1 + (ttx-tx)][1 + (tty-ty)] = 0;
				}
				sprintf(szImageUrl, "http://r%i.ortho.tiles.virtualearth.net/tiles/%s%s.%s?g=22", server, "r", szQuad, "png");
				Utils::DownloadMapTile(&g_pTiles[1 + (ttx-tx)][1 + (tty-ty)], szImageUrl, false);
			}
		}
	}

	s3eLocation bottomRight, topLeft, topRight, bottomLeft, center;
	bottomRight.m_Latitude = LiveMaps::YToLatitudeAtZoom(y+512, LiveMaps::MaxZoom);
	bottomRight.m_Longitude = LiveMaps::XToLongitudeAtZoom(x+512, LiveMaps::MaxZoom);
	topLeft.m_Latitude = LiveMaps::YToLatitudeAtZoom(y-256, LiveMaps::MaxZoom);
	topLeft.m_Longitude = LiveMaps::XToLongitudeAtZoom(x-256, LiveMaps::MaxZoom);

	if (topLeft.m_Longitude > bottomRight.m_Longitude)
	{
		float longitude = bottomRight.m_Longitude;
		bottomRight.m_Longitude = topLeft.m_Longitude;
		topLeft.m_Longitude = longitude;
	}
	if (topLeft.m_Latitude > bottomRight.m_Latitude)
	{
		float latitude = bottomRight.m_Latitude;
		bottomRight.m_Latitude = topLeft.m_Latitude;
		topLeft.m_Latitude = latitude;
	}

	topRight.m_Longitude = topLeft.m_Longitude;
	topRight.m_Latitude = bottomRight.m_Latitude;

	center.m_Latitude = (bottomRight.m_Latitude + topLeft.m_Latitude) / 2;
	center.m_Longitude = (bottomRight.m_Longitude + topLeft.m_Longitude) / 2;

	float maxDir = LiveMaps::CalculateDistance(topLeft, topRight) / 2.1;
	
	IGameHandler* pHandler = (IGameHandler*)g_pGameHandler;
	Region* pRegion = pHandler->GetBoundingRegion();

	for (int i = 0; i < CAPTURE_GAME_COUNT; ++i)
	{
		s3eLocation* randLoc = new s3eLocation;
		// Calculate a random location within a given radius
		int tryCount = 20;
		do
		{
			double angle = PI / 180.0 * IwRandMinMax(-180, 180); // 360 degree range
			int32 radius = IwRandMinMax(5, maxDir); // 50 meters

			LiveMaps::CalculateLatLongInDirection(&center, radius, angle, randLoc);

			s3eLocation testLoc = *randLoc;

			if (pRegion->Contains(testLoc) && (testLoc.m_Latitude >= topLeft.m_Latitude) && (testLoc.m_Latitude <= bottomRight.m_Latitude) && (testLoc.m_Longitude >= topLeft.m_Longitude) && (testLoc.m_Longitude <= bottomRight.m_Longitude))
			{
				break;
			}
		} while (tryCount-- > 0);

		g_pCaptures.push_back(randLoc);
	}
}

void CaptureGameEngine::Start()
{
	g_caughtAllTime = 0;
}

float CaptureGameEngine::End()
{
	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	// The score is 9000 * (captured / total) + 1000 * (timeremaining / total)

	float score = 9000.0f * (float)(CAPTURE_GAME_COUNT - g_pCaptures.size()) / CAPTURE_GAME_COUNT;

	if (g_caughtAllTime)
	{
		score += (g_caughtAllTime / GetGameLength()) * 1000.0f;
	}

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			if (g_pTiles[i][j])
			{
				delete g_pTiles[i][j];
				g_pTiles[i][j] = NULL;
			}
		}
	}

	for (int i = 0; i < g_pCaptures.size(); ++i)
	{
		delete g_pCaptures[i];
	}
	g_pCaptures.clear();

	return score;
}

void CaptureGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	*pfScore = (float)(g_pCaptures.size());
	strcpy(szUnits, "items remaining");
	
	s3eLocation loc;
	Utils::GetLocation(&loc);

	for (int i = 0; i < g_pCaptures.size(); ++i)
	{
		s3eLocation testLoc = *g_pCaptures[i];
		float distance = LiveMaps::CalculateDistance(loc, testLoc);

		if (distance <= 4)
		{
			s3eLocation* pLoc = g_pCaptures[i];
			g_pCaptures.erase(i);
			delete pLoc;
			
			i--;
		}
	}
	if (g_pCaptures.size() == 0 && g_caughtAllTime == 0)
	{
		g_caughtAllTime = remainingTime;
	}
}

void CaptureGameEngine::Render(CIwRect& bounds)
{
	float tileSize = bounds.w / 3.0f;
	float scale = tileSize / 256;
	float factor = Utils::GetImageScalingFactor();
	int userWidth = g_pUserTexture->GetWidth() * factor;
	int capWidth = g_pCursorTexture->GetWidth() * factor;

	CIwVec2 centerTileTL(bounds.x + (bounds.w - tileSize) / 2 + 1, bounds.y + (bounds.h - tileSize) / 2 + 1);

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			CIwRect tileBounds(centerTileTL.x + (x * tileSize), centerTileTL.y + (y * tileSize), tileSize, tileSize);
			Utils::AlphaRenderImage(g_pTiles[x+1][y+1], tileBounds, 255);
		}
	}

	for (int i = 0; i < g_pCaptures.size(); ++i)
	{
		int xDiffC = LiveMaps::LongitudeToXAtZoom(g_pCaptures[i]->m_Longitude, LiveMaps::MaxZoom) - g_topLeft.x;
		int yDiffC = LiveMaps::LatitudeToYAtZoom(g_pCaptures[i]->m_Latitude, LiveMaps::MaxZoom) - g_topLeft.y;
		
		CIwRect capLoc(centerTileTL.x + (xDiffC * scale) - capWidth/2, centerTileTL.y + (yDiffC * scale) - capWidth/2, capWidth, capWidth);
		Utils::AlphaRenderImage(g_pCursorTexture, capLoc, 255);
	}

	s3eLocation loc;
	Utils::GetLocation(&loc);

	int xDiff = LiveMaps::LongitudeToXAtZoom(loc.m_Longitude, LiveMaps::MaxZoom) - g_topLeft.x;
	int yDiff = LiveMaps::LatitudeToYAtZoom(loc.m_Latitude, LiveMaps::MaxZoom) - g_topLeft.y;
	
	CIwRect meLoc(centerTileTL.x + (xDiff * scale) - userWidth/2, centerTileTL.y + (yDiff * scale) - userWidth/2, userWidth, userWidth);
	Utils::AlphaRenderImage(g_pUserTexture, meLoc, 255);
}

bool CaptureGameEngine::ShouldRenderScore()
{
	return true;
}