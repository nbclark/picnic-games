#pragma once
#include "Utils.h"
#include "IwGx.h"
#include "IwGxFont.h"
#include "Iw2D.h"

class IGameObject
{
public:
	virtual CIwRect GetBounds() { return g_bounds; }
	virtual void SetBounds(CIwRect& bounds) { g_bounds = bounds; }
	virtual void Update() {}
	virtual void Render() {}
	virtual void Render2D() {}
	virtual void Reset() {}
protected:
	CIwRect g_bounds;
};

class CCountdownTimer : public IGameObject
{
public:
	CCountdownTimer()
	{
		char szNumber[3];
		szNumber[2] = 0;

		for (int i = 0; i < 10; ++i)
		{
			szNumber[0] = '0' + i;
			szNumber[1] = 'b';
			g_pNumberTextures[i] = (CIwTexture*)IwGetResManager()->GetResNamed(szNumber, "CIwTexture");
		}

		g_pLogo = (CIwTexture*)IwGetResManager()->GetResNamed("picnicgames", "CIwTexture");
		g_backCircle = (CIwTexture*)IwGetResManager()->GetResNamed("darkbluecircle", "CIwTexture");
		g_white.r = 0xff;
		g_white.g = 0xff;
		g_white.b = 0xff;
		g_white.a = 0xbf;

		g_bounds.x = g_bounds.y = 0;
		g_bounds.w = g_backCircle->GetWidth();
		g_bounds.h = g_backCircle->GetHeight();

		SetBounds(g_bounds);

		g_currentValue = 0;
		g_totalSize = 1;
		g_endArc = 0;
	}
	~CCountdownTimer()
	{
		delete g_backCircle;
	}
	void SetLocation(CIwVec2 location)
	{
		CIwRect bounds(location.x, location.y, g_backCircle->GetWidth(), g_backCircle->GetHeight());
		SetBounds(bounds);
	}
	void SetBounds(CIwRect& bounds)
	{
		g_bounds = bounds;

		g_center.x = g_bounds.x + (g_bounds.w) / 2;
		g_center.y = g_bounds.y + (g_bounds.h) / 2;

		g_arcSize.x = (g_bounds.w / 2) - 1;
		g_arcSize.y = (g_bounds.h / 2) - 1;
	}
	void SetTotal(uint totalSize)
	{
		g_totalSize = totalSize;
	}
	void SetCurrent(uint currentValue)
	{
		g_currentValue = currentValue;
	}
	void Update()
	{
		double degAng = ((double)g_currentValue / g_totalSize);
		while (degAng > 1)
		{
			degAng -= 1;
		}
		g_endArc = degAng * IW_ANGLE_PI * 2;

		float fSeconds = (g_totalSize-g_currentValue) / 1000.0;

		int secondsLeft = fSeconds;

		if (fSeconds > secondsLeft)
		{
			secondsLeft++;
		}

		if (secondsLeft != g_oldSecondsLeft)
		{
			g_timeLeft.clear();
			g_oldSecondsLeft = secondsLeft;

			if (secondsLeft >= 100)
			{
				g_timeLeft.append(secondsLeft / 100);
			}
			if (secondsLeft >= 10)
			{
				g_timeLeft.append((secondsLeft % 100) / 10);
			}
			g_timeLeft.append(secondsLeft % 10);
		}
	}
	static void DrawPrimsQuadList(CIwSVec2* pCoords)
	{
		if (g_bLeft)
		{
			static CIwSVec2 uvs[4] =
			{
				CIwSVec2((0 << 12) + 0, (0 << 12) + 0),
				CIwSVec2((0 << 12) + 0, (1 << 12) - 0),
				CIwSVec2((1 << 11) - 0, (1 << 12) - 0),
				CIwSVec2((1 << 11) - 0, (0 << 12) + 0),
			};
			IwGxSetUVStream(uvs);
		}
		else
		{
			static CIwSVec2 uvs[4] =
			{
				CIwSVec2((1 << 11) + 0, (0 << 11) + 0),
				CIwSVec2((1 << 11) + 0, (1 << 12) - 0),
				CIwSVec2((1 << 12) - 0, (1 << 12) - 0),
				CIwSVec2((1 << 12) - 0, (0 << 11) + 0),
			};
			IwGxSetUVStream(uvs);
		}
		IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);
	}
	void Render()
	{
		Utils::AlphaRenderImage(g_backCircle, g_bounds, 255);
		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);

		Iw2DSetColour(g_white);
		Iw2DFillArc(g_center, g_arcSize, 0, g_endArc, 0);

		IwGxFlush();

		int width = g_pNumberTextures[0]->GetWidth() * Utils::GetImageScalingFactor();
		int height = g_pNumberTextures[0]->GetHeight() * Utils::GetImageScalingFactor();
		int totalWidth = width * g_timeLeft.size();

		CIwRect location(g_bounds.x + (g_bounds.w - totalWidth) / 2, g_bounds.y + (g_bounds.h - height) / 2, width, height);

		for (int i = 0; i < g_timeLeft.size(); ++i)
		{
			CIwTexture* pTexture = g_pNumberTextures[g_timeLeft[i]];
			Utils::AlphaRenderImage(pTexture, location, 175);
			location.x += width;
		}

		int logoWidth = g_pLogo->GetWidth() * Utils::GetImageScalingFactor() / 2;
		int logoHeight = g_pLogo->GetHeight() * Utils::GetImageScalingFactor();

		CIwRect logoLeft(g_bounds.x - logoWidth - 10, g_bounds.y + (g_bounds.h - logoHeight) / 2, logoWidth, logoHeight);
		CIwRect logoRight(g_bounds.x + g_bounds.w + 10, g_bounds.y + (g_bounds.h - logoHeight) / 2, logoWidth, logoHeight);

		g_bLeft = true;
		Utils::AlphaRenderImage(g_pLogo, logoLeft, 100, DrawPrimsQuadList);
		g_bLeft = false;
		Utils::AlphaRenderImage(g_pLogo, logoRight, 100, DrawPrimsQuadList);
	}
	void Render2D()
	{
	}
	void Reset()
	{
		g_oldSecondsLeft = 0;
		g_currentValue = 0;
		g_totalSize = 360;
	}
private :
	static bool g_bLeft;
	int g_oldSecondsLeft;
	CIwArray<int> g_timeLeft;
	CIwTexture* g_pLogo;
	CIwTexture* g_pNumberTextures[10];
	uint g_totalSize;
	uint g_currentValue;
	CIwTexture* g_backCircle;
	CIwColour g_white;
	CIwSVec2 g_center;
	CIwSVec2 g_arcSize;
	iwangle g_endArc;
};



class CScoreKeeper : public IGameObject
{
public:
	CScoreKeeper()
	{
		char szNumber[2];
		szNumber[1] = 0;

		for (int i = 0; i < 10; ++i)
		{
			szNumber[0] = '0' + i;
			g_pNumberTextures[i] = (CIwTexture*)IwGetResManager()->GetResNamed(szNumber, "CIwTexture");
		}
		g_pDotTexture = (CIwTexture*)IwGetResManager()->GetResNamed("dot", "CIwTexture");

		float scale = Utils::GetImageScalingFactor();
		float tscale = Utils::GetTextScalingFactor();
		g_iFontHeight = 60 * tscale;

		g_sizeDot.x = g_pDotTexture->GetWidth() * scale;
		g_sizeDot.y = g_pDotTexture->GetHeight() * scale;

		g_sizeNum.x = g_pNumberTextures[0]->GetWidth() * scale;
		g_sizeNum.y = g_pNumberTextures[0]->GetHeight() * scale;

		g_bounds.h = g_sizeNum.y + g_iFontHeight;
		g_bounds.x = g_bounds.y = 0;
		g_bounds.w = (g_sizeNum.x * 7) + g_sizeDot.x;

		SetBounds(g_bounds);

		g_pFont = Utils::GetFont(true);
		strcpy(g_szUnits, "feet");
		strcpy(g_szString, "00000.00");
	}
	~CScoreKeeper()
	{
	}
	void SetLocation(CIwVec2 location)
	{
		CIwRect bounds(location.x, location.y, g_bounds.w, g_bounds.h);
		SetBounds(bounds);
	}
	void SetBounds(CIwRect& bounds)
	{
		g_bounds = bounds;
	}
	void SetUnits(char* szUnits)
	{
		strcpy(g_szUnits, szUnits);
	}
	void SetValue(double currentValue)
	{
		g_currentValue = currentValue;
	}
	void Update()
	{
		while (g_currentValue >= 100000)
		{
			g_currentValue-= 100000;
		}

		sprintf(g_szString, "%05d.%02d", (int)ABS(g_currentValue), (int)(100 * ABS(g_currentValue - (int)g_currentValue)));
	}
	void Render()
	{
		CIwRect loc(g_bounds.x, g_bounds.y, 0, g_sizeNum.y);

		for (int i = 0; i < 8; ++i)
		{
			if (i == 5)
			{
				CIwTexture* pTexture = g_pDotTexture;
				loc.w = g_sizeDot.x;
				Utils::AlphaRenderImage(pTexture, loc, 255);
				loc.x += loc.w;
			}
			else
			{
				CIwTexture* pTexture = g_pNumberTextures[g_szString[i] - '0'];
				loc.w = g_sizeNum.x;
				Utils::AlphaRenderImage(pTexture, loc, 255);
				loc.x += loc.w;
			}
		}
		IwGxLightingOn();
		IwGxFontSetFont(g_pFont);
		IwGxFontSetCol(0xff6BFF8D);
		IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
		IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);

		CIwRect rect(g_bounds.x, g_bounds.y + g_bounds.h - g_iFontHeight, g_bounds.w, g_iFontHeight);
		IwGxFontSetRect(rect);
		IwGxFontDrawText(g_szUnits);
		
		IwGxLightingOff();
	}
	void Render2D()
	{
	}
	void Reset()
	{
		g_currentValue = 0;
	}
private :
	double g_currentValue;
	CIwTexture* g_pNumberTextures[10];
	CIwTexture* g_pDotTexture;
	char g_szUnits[50];
	char g_szString[20];
	CIwGxFont* g_pFont;
	int g_iFontHeight;
	CIwVec2 g_sizeDot;
	CIwVec2 g_sizeNum;
};


class CContentBlock : public IGameObject
{
public:
	CContentBlock()
	{
	}
	~CContentBlock()
	{
	}
	void SetLocation(CIwVec2 location)
	{
		CIwRect bounds(location.x, location.y, g_bounds.w, g_bounds.h);
		SetBounds(bounds);
	}
	void SetBounds(CIwRect& bounds)
	{
		g_bounds = bounds;
	}
	CIwRect GetInnerBounds()
	{
		CIwRect innerBounds(g_bounds.x+2, g_bounds.y+2, g_bounds.w-4, g_bounds.h-4);
		return innerBounds;
	}
	void Update()
	{
	}
	void Render()
	{
		CIwColour white;
		white.Set(0xff, 0xff, 0xff, 0x7f);
		CIwColour black;
		black.Set(0, 0, 0, 0x7f);

		Iw2DSetColour(white);
		Iw2DFillRect(CIwSVec2(g_bounds.x, g_bounds.y), CIwSVec2(g_bounds.w, g_bounds.h));
		Iw2DSetColour(black);
		Iw2DFillRect(CIwSVec2(g_bounds.x + 2, g_bounds.y + 2), CIwSVec2(g_bounds.w - 4, g_bounds.h - 4));
	}
	void Render2D()
	{
	}
	void Reset()
	{
	}
};

class CLineGraph : public IGameObject
{
public:
	CLineGraph()
	{
		g_iPointCount = 20;
	}
	~CLineGraph()
	{
	}
	void SetLocation(CIwVec2 location)
	{
		CIwRect bounds(location.x, location.y, g_bounds.w, g_bounds.h);
		SetBounds(bounds);
	}
	void SetBounds(CIwRect& bounds)
	{
		g_bounds = bounds;
	}
	void Clear()
	{
		g_distancesOverTime.clear();
	}
	void SetTotalPoints(int iPointCount)
	{
		g_iPointCount = iPointCount;
	}
	void AddPoint(float value)
	{
		g_distancesOverTime.push_back(value);
	}
	void Update()
	{
	}
	void Render()
	{
		if (g_distancesOverTime.size() > 1)
		{
			float maxValue = 0;
			float minValue = 0;

			for (int i = 0; i < g_distancesOverTime.size() && i < g_iPointCount; ++i)
			{
				maxValue = MAX(maxValue, g_distancesOverTime[i]);
				minValue = MIN(minValue, g_distancesOverTime[i]);
			}

			maxValue = MAX(maxValue, 250);
			minValue = MIN(minValue, -10);

			float scaleY = g_bounds.h / (maxValue - minValue);
			float zeroY = g_bounds.y + (g_bounds.h * (maxValue / (maxValue - minValue)));
			float tickSize = (g_bounds.w / (float)(g_iPointCount-1));

			CIwFVec2 startPoint(g_bounds.x, zeroY);

			float xPos = g_bounds.x;
			float previousValueX = xPos;
			float previousValueY = zeroY;

			CIwColour whiteAlpha;
			whiteAlpha.Set(0xff, 0xff, 0xff, 0x7f);
			Iw2DSetColour(whiteAlpha);
			Iw2DDrawLine(CIwSVec2(g_bounds.x, zeroY), CIwSVec2(g_bounds.x + g_bounds.w, zeroY));
			Iw2DDrawLine(CIwSVec2(g_bounds.x, zeroY-1), CIwSVec2(g_bounds.x + g_bounds.w, zeroY-1));

			CIwColour white;
			white.Set(0xff, 0xff, 0xff);
			CIwColour black;
			black.Set(0xff, 0xff, 0xff);
			
			Iw2DSetColour(black);
			Iw2DFillRect(CIwSVec2(xPos-3, zeroY-3), CIwSVec2(6,6));

			for (int i = 1; i < g_distancesOverTime.size() && i < g_iPointCount; ++i)
			{
				xPos += tickSize;
				float currentY = zeroY - (scaleY * g_distancesOverTime[i]);
				// draw from previous value to current value
				Iw2DSetColour(white);
				Iw2DDrawLine(CIwSVec2(previousValueX, previousValueY), CIwSVec2(xPos, currentY));

				Iw2DSetColour(whiteAlpha);
				Iw2DDrawLine(CIwSVec2(previousValueX, previousValueY-1), CIwSVec2(xPos, currentY-1));
				Iw2DDrawLine(CIwSVec2(previousValueX, previousValueY+1), CIwSVec2(xPos, currentY+1));
				
				Iw2DSetColour(black);
				Iw2DFillRect(CIwSVec2(xPos-3, currentY-3), CIwSVec2(6,6));

				previousValueX = xPos;
				previousValueY = currentY;
			}
		}
	}
	void Reset()
	{
	}
private:
	CIwArray<float> g_distancesOverTime;
	uint64 g_lastTimeAdd;
	int g_iPointCount;
};

class CBarGraph : public IGameObject
{
public:
	CBarGraph()
	{
		g_pFont = Utils::GetFont(false);
		g_animCount = Utils::FPS;
	}
	~CBarGraph()
	{
		Reset();
	}
	void SetLocation(CIwVec2 location)
	{
		CIwRect bounds(location.x, location.y, g_bounds.w, g_bounds.h);
		SetBounds(bounds);
	}
	void SetBounds(CIwRect& bounds)
	{
		g_bounds = bounds;
	}
	void AddBar(char* szLabel, float value)
	{
		// Todo, animate updates here
		for (int i = 0; i < g_barPoints.size(); ++i)
		{
			if (0 == strcmp(szLabel, g_barPoints[i]->szLabel))
			{
				g_barPoints[i]->fOldValue = 0;//g_barPoints[i]->fNewValue;
				g_barPoints[i]->fNewValue = value;
				g_barPoints[i]->iIter = 0;
				return;
			}
		}
		BarPoint* bp = new BarPoint;
		strcpy(bp->szLabel, szLabel);
		bp->fNewValue = value;
		bp->fOldValue = 0;
		bp->iIter = 0;

		g_barPoints.push_back(bp);
	}
	void Update()
	{
		for (int i = 0; i < g_barPoints.size(); ++i)
		{
			if (g_barPoints[i]->iIter < g_animCount)
			{
				g_barPoints[i]->iIter++;
			}
		}
	}
	void Render()
	{
		if (g_barPoints.size() > 0)
		{
			CIwSVec2 startPoint(g_bounds.x, g_bounds.y);

			CIwColour whiteAlpha;
			whiteAlpha.Set(0xff, 0xff, 0xff, 0x7f);
			Iw2DSetColour(whiteAlpha);

			CIwColour white;
			white.Set(0xff, 0xff, 0xff);
			CIwColour black;
			black.Set(0xff, 0xff, 0xff);
			
			IwGxLightingOn();
			IwGxFontSetFont(g_pFont);
			IwGxFontSetCol(0xffffffff);
			IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
			IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_LEFT);
			
			Iw2DSetColour(black);
			//Iw2DFillRect(CIwSVec2(xPos-3, zeroY-3), CIwSVec2(6,6));

			float maxValue = g_barPoints[0]->fNewValue;
			
			for (int i = 1; i < g_barPoints.size(); ++i)
			{
				maxValue = MAX(g_barPoints[i]->fNewValue, maxValue);
			}

			if (!maxValue)
			{
				maxValue = 1;
			}

			static int colorSize = IW_GX_COLOUR_MAX;
			static int barHeight = Utils::GetTextScalingFactor() * 60;
			char szScore[10];
			Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);
			for (int i = 0; i < g_barPoints.size(); ++i)
			{
				startPoint.y += 5;

				CIwColour barColor = g_IwGxColours[(i % (colorSize - 3)) + 3];
				
				int maxBar = g_bounds.w;
				int remaining = g_bounds.w - maxBar;
				float currValue = g_barPoints[i]->fOldValue + (g_barPoints[i]->fNewValue - g_barPoints[i]->fOldValue) * (g_barPoints[i]->iIter / (float)g_animCount);
				float perc = (currValue / maxValue);
				float width = MAX(15, perc * maxBar);

				CIwRect rectBar(startPoint.x, startPoint.y, width, barHeight);
				CIwRect rectBarInside = rectBar;
				rectBarInside.x += 2;
				rectBarInside.y += 2;
				rectBarInside.w -= 4;
				rectBarInside.h -= 4;

				Iw2DSetColour(g_IwGxColours[IW_GX_COLOUR_WHITE]);
				Iw2DFillRect(CIwSVec2(rectBar.x, rectBar.y), CIwSVec2(rectBar.w, rectBar.h));

				Iw2DSetColour(barColor);
				Iw2DFillRect(CIwSVec2(rectBarInside.x, rectBarInside.y), CIwSVec2(rectBarInside.w, rectBarInside.h));

				IwGxFlush();
				
				CIwRect rectScore(rectBar.x + 5, startPoint.y, 80, barHeight);
				IwGxFontSetRect(rectScore);

				sprintf(szScore, "%d", (int)g_barPoints[i]->fNewValue);
				IwGxFontDrawText(szScore);

				startPoint.y += barHeight;
				startPoint.y += 5;
				
				CIwRect rectName(startPoint.x, startPoint.y, g_bounds.w, barHeight);
				IwGxFontSetRect(rectName);
				IwGxFontDrawText(g_barPoints[i]->szLabel);

				startPoint.y += barHeight;
				startPoint.y += 5;
			}
			IwGxLightingOff();
		}
	}
	void Reset()
	{
		for (int i =0; i < g_barPoints.size(); ++i)
		{
			delete g_barPoints[i];
		}
		g_barPoints.clear();
	}
private:
	struct BarPoint
	{
		char szLabel[50];
		float fOldValue;
		float fNewValue;
		int iIter;
	};
	int g_animCount;
	CIwArray<BarPoint*> g_barPoints;
	uint64 g_lastTimeAdd;
	CIwGxFont* g_pFont;
};