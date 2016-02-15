#include "ActiveGameState.h"
#include "MessageBox.h"
#include "Constants.h"
#include "GoTheDistanceGameEngine.h"
#include "FindTheSpotGameEngine.h"
#include "BoomerangGameEngine.h"
#include "EggDropGameEngine.h"
#include "ShakeItGameEngine.h"
#include "CaptureGameEngine.h"

ActiveGameState::ActiveGameState(void)
{
	g_iAnimationCount = 0;
	g_cursorIter = 0;
	g_iStartGameCount = 0;

	IW_UI_CREATE_VIEW_SLOT1(this, "ActiveGameState", ActiveGameState, OnClickResume, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("active\\panel", "CIwUIElement");
	g_pDialogMain->SetVisible(false);
	g_pTitleBar = g_pDialogMain->GetChildNamed("TitleLabel", true);

	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	g_pFont = Utils::GetFont(false);
	g_pFontLarge = Utils::GetFont(true);

	g_bIsScore = false;
	g_dAlpha = 100;
	g_bGameOver = false;
	g_pGameEngine = NULL;

	IwGetMultiplayerHandler()->RegisterModeChangedCallback(MultiplayerModeChanged, this);
	IwGetMultiplayerHandler()->RegisterDisconnectCallback(MultiplayerDisconnect, this);

	g_pScoreKeeper = new CScoreKeeper;
	g_pContentBlock = new CContentBlock;
	g_pTimer = new CCountdownTimer;
	g_barGraph = new CBarGraph;
	
	CIwRect bounds = g_pScoreKeeper->GetBounds();
	bounds.x = (Iw2DGetSurfaceWidth() - bounds.w) / 2;
	bounds.y = (Iw2DGetSurfaceHeight() - bounds.h - 10);
	g_pScoreKeeper->SetBounds(bounds);
	g_pScoreKeeper->SetValue(0);

	CIwRect tbounds = g_pTimer->GetBounds();
	tbounds.w = (Iw2DGetSurfaceWidth() / 2) - 40;
	tbounds.h = tbounds.w;
	tbounds.x = (Iw2DGetSurfaceWidth() - tbounds.w) / 2;
	tbounds.y = 10;
	g_pTimer->SetBounds(tbounds);
	g_pTimer->SetTotal(60000);

	int barHeight = 100 * Utils::GetTextScalingFactor();
	CIwRect scoreBounds(10, barHeight, Iw2DGetSurfaceWidth() - 20, Iw2DGetSurfaceHeight() - Utils::BottomPadding);
	g_barGraph->SetBounds(scoreBounds);

	CIwRect cbounds = g_pContentBlock->GetBounds();
	cbounds.h = 260;
	cbounds.w = 260;//(Iw2DGetSurfaceWidth() - 64);

	int blockSize = MIN((Iw2DGetSurfaceWidth() - Utils::SidePadding), bounds.y - (tbounds.y + tbounds.h + 20));

	cbounds.w = blockSize;
	cbounds.h = cbounds.w;
	cbounds.x = (Iw2DGetSurfaceWidth() - cbounds.w) / 2;

	cbounds.y = tbounds.y + tbounds.h + 10;
	g_pContentBlock->SetBounds(cbounds);

	g_pBackground = (CIwTexture*)IwGetResManager()->GetResNamed("fullback", "CIwTexture");
	g_pReady = (CIwTexture*)IwGetResManager()->GetResNamed("ready", "CIwTexture");
	g_pSet = (CIwTexture*)IwGetResManager()->GetResNamed("set", "CIwTexture");
	g_pGo = (CIwTexture*)IwGetResManager()->GetResNamed("go", "CIwTexture");

	g_games.push_back(new FindTheSpotGameEngine);
	g_games.push_back(new GoTheDistanceGameEngine);

#ifndef GAME_TRIALMODE
	g_games.push_back(new ShakeItGameEngine);
	g_games.push_back(new CaptureGameEngine);
	g_games.push_back(new BoomerangGameEngine);
	g_games.push_back(new EggDropGameEngine);
#endif

	g_gameState = PGS_INIT;

	//g_iIntroCount = 300;
	//g_iIntroReadyCount = 200;
	//g_iIntroSetCount = 280;

	g_iIntroCount = 10 * Utils::FPS;
	g_iIntroReadyCount = 7 * Utils::FPS;
	g_iIntroSetCount = (int)(8.5 * Utils::FPS);
}

ActiveGameState::~ActiveGameState(void)
{
	delete g_pFont;
	delete g_pBackground;
	delete g_pReady;
	delete g_pSet;

	delete g_barGraph;
	delete g_pContentBlock;
	delete g_pTimer;
	delete g_pScoreKeeper;

	for (int i = 0; i < g_games.size(); ++i)
	{
		g_games[i]->End();
		delete g_games[i];
	}

	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void ActiveGameState::MultiplayerDisconnect(bool success, const char* szStatus, void* userData)
{
	ActiveGameState* pThis = (ActiveGameState*)userData;

	MessageBox::Show("Disconnected", (char*)szStatus, "OK", "Cancel", NULL, userData);
	pThis->g_pMapGame->SetGameState(GPS_GameState_SelectGame, AnimDir_Right);
}

void ActiveGameState::MultiplayerModeChanged(MultiplayerMode mode, void* userData)
{
	if (mode == MPM_IN_GAME)
	{
		// We have started our game
		// Send our state & transition
		ActiveGameState* pThis = (ActiveGameState*)userData;
	}
}


void ActiveGameState::OnClickResume(CIwUIElement* Clicked)
{
	HideScore();
	StartNextGame();
}

void ActiveGameState::PerformActivate()
{
	// Create an order of the games
	g_gameIndices.clear();
	for (int i = 0; i < g_games.size(); ++i)
	{
		int value = 0;
		bool hasMatch = false;
		do
		{
			value = IwRandMinMax(0, g_games.size());

			hasMatch = false;
			for (int x = 0; x < g_gameIndices.size(); ++x)
			{
				if (g_gameIndices[x] == value)
				{
					hasMatch = true;
					break;
				}
			}
		} while (hasMatch);
		g_gameIndices.push_back(value);
	}
	for (int i = 0; i < g_games.size(); ++i)
	{
		g_games[i]->SetGameHandler(g_pMapGame);
	}
	g_barGraph->Reset();
	g_bGameOver = false;

	g_bRenderGame = false;

	g_bMouseDown = false;
	g_iStartGameCount = 0;
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);

	g_bIsScore = false;
	g_pMapGame->StartTrackingDistance();

	g_mpLastFlush = 0;

	IwGetMultiplayerHandler()->Flush();
	g_mpLastFlush = s3eTimerGetMs();

	g_iGameIndex = -1;
	
	IwGetMultiplayerHandler()->RegisterCallback(0, ReceiveStatusUpdate, this);

	// We have our users & a game. If we are the master, we want to send out the game start

	g_iTotalScore = 0;
	StartNextGame();

	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	for (int i = 0; i < users.size(); ++i)
	{
		g_barGraph->AddBar(users[i]->szName, 0);
	}
}

void ActiveGameState::ReceiveStatusUpdate(const char * Result, uint32 ResultLen, void* userData)
{
	ActiveGameState* pThis = (ActiveGameState*)userData;
	PicnicGamesMessage* message = (PicnicGamesMessage*)Result;

	IwAssertMsg("FF", sizeof(PicnicGamesMessage) == ResultLen, ("sizeof(PicnicGamesMessage) != ResultLen (%d - %d)", sizeof(PicnicGamesMessage), ResultLen));

	PicnicGamesMessage networkMessage;
	memcpy(&networkMessage, Result, sizeof(PicnicGamesMessage));
	PrepareReceive_PicnicGamesMessage(&networkMessage, &networkMessage);

	switch (networkMessage.ev)
	{
		case PGE_GAME :
		{
			// TODO - we need to load the new game here & hide the score
			int gameIndex = networkMessage.dataA;
			pThis->g_iGameIndex = networkMessage.dataB;
			
			if (gameIndex < 0)
			{
				pThis->g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
				pThis->HideScore();
			}
			else
			{
				pThis->g_pGameEngine = pThis->g_games[gameIndex];
				pThis->HideScore();

				pThis->g_gameState = PGS_INGAME;
			}
		}
		break;
		case PGE_SCORESINGLE :
		{
			if (IwGetMultiplayerHandler()->IsMaster())
			{
				pThis->SetUserScore(networkMessage.dataA, networkMessage.dataB << 16, false);
			}
		}
		break;
		case PGE_SCOREMULTI :
		{
			if (!IwGetMultiplayerHandler()->IsMaster())
			{
				pThis->g_allUsersReady = true;
				CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();
				for (int i = 0; i < users.size(); ++i)
				{
					int32* pScoreFirst = &networkMessage.score1;
					int32* pScore = pScoreFirst + (sizeof(networkMessage.score1) * users[i]->uiIndex);

					users[i]->Score = *pScore;
					pThis->g_barGraph->AddBar(users[i]->szName, (users[i]->Score >> 16));
				}
			}
		}
		break;
	}
}

void ActiveGameState::SetUserScore(int32 userId, int32 score, bool setMe)
{
	int16 totalScore = (score & 0xFFFF);
	int16 levelScore = (score >> 16);

	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	bool allUsersReady = true;
	for (int i = 0; i < users.size(); ++i)
	{
		if (users[i]->uiIndex == userId && (setMe || !users[i]->IsMe))
		{
			users[i]->Ready = true;
			users[i]->UserData = score;
		}
		if (!users[i]->Ready)
		{
			allUsersReady = false;
		}
	}

	// We need to force an update of the UI here to update the scores / graphs
	g_allUsersReady = allUsersReady;

	if (allUsersReady)
	{
		CIwArray<CIwMultiplayerHandler::User*> userList;

		if (users.size() == 1)
		{
			users[0]->Score += levelScore;
		}
		else
		{
			for (uint i = 0; i < users.size(); ++i)
			{
				uint insertPoint = userList.size();
				for (uint j = 0; j < userList.size(); ++j)
				{
					if (users[i]->UserData < userList[j]->UserData)
					{
						insertPoint = j;
						break;
					}
				}
				userList.insert_slow(users[i], insertPoint);
			}

			// Sort the scores and sum
			int maxScore = users.size();
			int lastScore = INT16_MIN;
			for (uint i = 0; i < userList.size(); ++i)
			{
				int userScore = (userList[i]->UserData & 0xFFFF);
				if (i > 0 && userScore != lastScore)
				{
					maxScore--;
				}
				userList[i]->Score += (maxScore * 10); // this was 10 -- we can just add since the total score is the LSB
				lastScore = userList[i]->UserData;
			}
		}
		PicnicGamesMessage message;
		message.ev = PGE_SCOREMULTI;

		for (uint i = 0; i < users.size(); ++i)
		{
			int32* pScoreFirst = &message.score1;
			int32* pScore = pScoreFirst + (sizeof(int32) * users[i]->uiIndex);

			*pScore = users[i]->Score;
			int32 score = users[i]->Score;
			g_barGraph->AddBar(users[i]->szName, levelScore);
		}

		if (IwGetMultiplayerHandler()->IsMultiplayer() && IwGetMultiplayerHandler()->IsMaster())
		{
			PrepareSend_PicnicGamesMessage(message, message);
			IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
			IwGetMultiplayerHandler()->Flush();
		}
	}
}

void ActiveGameState::PerformDeActivate()
{
	g_pMapGame->StopTrackingDistance();
}

void ActiveGameState::StartNextGame()
{
	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	bool allUsersReady = true;
	for (uint i = 0; i < users.size(); ++i)
	{
		users[i]->Ready = false;
	}

	if (IwGetMultiplayerHandler()->IsMaster())
	{
		g_iGameIndex++;
		int16 gameId = -1;

		if (g_iGameIndex < (int)g_gameIndices.size())
		{
			g_pGameEngine = g_games[g_gameIndices[g_iGameIndex]];
			gameId = g_gameIndices[g_iGameIndex];
		}

		PicnicGamesMessage message;
		message.ev = PGE_GAME;
		message.dataA = gameId;
		message.dataB = g_iGameIndex;

		PrepareSend_PicnicGamesMessage(message, message);
		IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
		IwGetMultiplayerHandler()->Flush();

		if (gameId >= 0)
		{
			g_gameState = PGS_INGAME;
		}
		else
		{
			this->GetGameHandler()->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
	}
	else
	{
		g_gameState = PGS_WAITING;
	}
}

uint64 g_pauseStart = -1;

void ActiveGameState::PerformUpdate()
{
	s3eDeviceBacklightOn();
	// Update the background
	bool hasUpdate = false;
	bool renderUI = false;
	bool backClicked = false;
	bool becameScore = false;
	bool willFlush = false;

	bool shouldPause = s3eDeviceCheckPauseRequest();

	if (g_gameState == PGS_WAITING)
	{
		// wait for the goahead
		// TODO - render the score & a waiting dialog

		renderUI = true;
		g_dAlpha = 75;

		g_barGraph->Update();

		CIwUIRect rect = g_pTitleBar->GetFrame();
		if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
		{
			backClicked = true;
		}
		else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
		{
			if (g_showLastGameScore)
			{
				g_showLastGameScore = false;
				
				CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();
				for (uint i = 0; i < users.size(); ++i)
				{
					g_barGraph->AddBar(users[i]->szName, (int16)(users[i]->Score & 0xFFFF));
				}
			}
			else
			{
				if (IwGetMultiplayerHandler()->IsMaster())
				{
					// Hide the score & send the game to everyone else?
					HideScore();
					StartNextGame();
					//HideScore();
					//hasUpdate = true;
				}
			}
		}
	}
	else if (g_gameState == PGS_INGAME)
	{
		IwGetUIAnimManager()->StopAllAnims();

		bool isLoading = !(g_iStartGameCount > g_iIntroCount);

		if (g_iStartGameCount == 0)
		{
			g_showLastGameScore = true;
			g_pGameEngine->Prepare();
		}
		else if (!isLoading && !g_bRenderGame)
		{
			g_uiStartTime = s3eTimerGetMs();
			g_bRenderGame = true;
			g_pTimer->SetTotal(g_pGameEngine->GetGameLength());
			g_pGameEngine->Start();
		}

		Utils::UpdateLocation();
		
		int pulseInterval = (IwGetMultiplayerHandler()->IsMaster()) ? 500 : 500;
		if (IwGetMultiplayerHandler()->InSocketMode())
		{
			// TODO: increase interval if we are tilting
			pulseInterval = (IwGetMultiplayerHandler()->IsMaster()) ? 350 : 200;

			if (!becameScore && g_bIsScore)
			{
				pulseInterval = 2500;
			}
		}

		uint64 timer = s3eTimerGetMs();

		if (isLoading)
		{
			// show the 1 2 3 here
			g_iStartGameCount++;
		}
		else if (g_bIsScore)
		{
			renderUI = true;
			g_dAlpha = 75;

			CIwUIRect rect = g_pTitleBar->GetFrame();
			if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
			{
				backClicked = true;
			}
			else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
			{
				if (IwGetMultiplayerHandler()->IsMaster())
				{
					// Hide the score & send the game to everyone else?
					//HideScore();
					//hasUpdate = true;
				}
			}
		}
		else if (g_bGameOver)
		{
			this->GetGameHandler()->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
		else
		{
			if (shouldPause)
			{
				MessageBox::Show("Game is Paused", "Click OK to unpause the game", "OK", "Cancel", NULL, NULL);
				uint64 pauseOffset = s3eTimerGetMs() - g_pauseStart;

				g_uiStartTime += pauseOffset;
			}

			CIwUIRect rect = g_pTitleBar->GetFrame();
			g_dAlpha = 150;
			if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
			{
				backClicked = true;
			}
			else
			{
				int32 gameLength = g_pGameEngine->GetGameLength();

				// Update the game engine here
				uint64 elapsedTime = (s3eTimerGetMs() - g_uiStartTime);
				uint64 remainingTime = gameLength - elapsedTime;
				g_pTimer->SetCurrent(elapsedTime);
				float fScore = 0;
				char szUnits[50];

				if (elapsedTime < gameLength)
				{
					g_pGameEngine->Update(remainingTime, &fScore, szUnits);
					g_pTimer->Update();
					g_pContentBlock->Update();
					g_pScoreKeeper->SetValue(fScore);
					g_pScoreKeeper->SetUnits(szUnits);
					g_pScoreKeeper->Update();
				}

				static bool bNeedVibra = true;
				if (bNeedVibra && elapsedTime > gameLength && elapsedTime < gameLength + 1000)
				{
					bNeedVibra = false;
					g_pTimer->Reset();
					s3eVibraVibrate(255, 100);
				}

				if (elapsedTime > gameLength + 3000)
				{
					bNeedVibra = true;
					g_gameState = PGS_WAITING;
					ShowScore();
					float finalScore = g_pGameEngine->End();
					g_iTotalScore += (int16)(finalScore);

					if (!IwGetMultiplayerHandler()->IsMaster())
					{
						PicnicGamesMessage message;
						message.ev = PGE_SCORESINGLE;
						message.dataA = IwGetMultiplayerHandler()->GetMe()->uiIndex;
						message.dataB = (int16)(finalScore);

						PrepareSend_PicnicGamesMessage(message, message);
						IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
					}
					else
					{
						SetUserScore(IwGetMultiplayerHandler()->GetMe()->uiIndex, ((int32)finalScore) << 16, true);
					}

					hasUpdate = true;
				}
			}
		}
		// Always write level state
		if (willFlush || hasUpdate)
		{
			g_mpLastFlush = timer;
			IwGetMultiplayerHandler()->Flush();
		}
	}

	if (backClicked)
	{
		bool result = MessageBox::Show((char*)"Quit Game", (char*)"Are you sure you want to quit this game?", (char*)"Yes", (char*)"No", GameState::MessageRenderBackground, this);

		if (result)
		{
			if (g_pGameEngine)
			{
				g_pGameEngine->End();
			}
			IwGetMultiplayerHandler()->EndGame();
			g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
	}

	if (renderUI)
	{
		IwGetUIController()->Update();
		IwGetUIView()->Update(1000/20);
	}
	g_pauseStart = s3eTimerGetMs();
}

void ActiveGameState::PerformRender()
{
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(-1);
	RenderBackground();

	if (g_gameState == PGS_WAITING)
	{
		// wait for the goahead
		// we should draw a waiting sign here
		// TODO - render the score & a waiting dialog
	
		CIwRect backLoc(0,0,Iw2DGetSurfaceWidth(),Iw2DGetSurfaceHeight());
		Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

		IwGxLightingOn();
		IwGxFontSetFont(g_pFontLarge);
		IwGxFontSetCol(0xff000000);
		IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
		IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);

		int barHeight = 80 * Utils::GetTextScalingFactor();

		//Render the level
		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetColour(g_IwGxColours[IW_GX_COLOUR_WHITE]);
		Iw2DFillRect(CIwSVec2(0, 0), CIwSVec2(Iw2DGetSurfaceWidth(), barHeight));

		//Render the continue
		Iw2DSetColour(g_IwGxColours[IW_GX_COLOUR_WHITE]);
		Iw2DFillRect(CIwSVec2(0, Iw2DGetSurfaceHeight()-barHeight), CIwSVec2(Iw2DGetSurfaceWidth(), barHeight));

		IwGxFlush();

		CIwRect rectLevel(0, 0, Iw2DGetSurfaceWidth(), barHeight);
		IwGxFontSetRect(rectLevel);

		char szLevel[30];

		if (g_showLastGameScore)
		{
			sprintf(szLevel, "Level %d", g_iGameIndex+1, g_gameIndices.size());
		}
		else
		{
			sprintf(szLevel, "After %d of %d Levels", g_iGameIndex+1, g_gameIndices.size());
		}
		IwGxFontDrawText(szLevel);

		CIwRect rectScore(0, Iw2DGetSurfaceHeight()-barHeight, Iw2DGetSurfaceWidth(), barHeight);

		if (!g_allUsersReady)
		{
			IwGxFontSetRect(rectScore);
			IwGxFontDrawText("Waiting for all user's scores...");
		}
		else
		{
			IwGxFontSetFont(g_pFont);
			IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
			IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
			IwGxFontSetRect(rectScore);
			if (IwGetMultiplayerHandler()->IsMaster())
			{
				IwGxFontDrawText("Tap the Screen to Continue...");
			}
			else
			{
				IwGxFontDrawText("Waiting for the next game...");
			}
		}
		IwGxLightingOff();
		IwGxFlush();

		if (g_allUsersReady)
		{
			g_barGraph->Render();
		}
	}
	else if (g_gameState == PGS_INGAME)
	{
		// Render 2-d stuff here
		if (!g_bRenderGame)
		{
/*
			CIw2DImage* img;

			if (g_iStartGameCount < 20)
			{
				img = g3;
			}
			else if (g_iStartGameCount < 40)
			{
				img = g2;
			}
			else
			{
				img = g1;
			}
			CIwSVec2 pos;
			pos.x = (int16)((Iw2DGetSurfaceWidth() - img->GetWidth()) / 2);
			pos.y = (int16)((Iw2DGetSurfaceHeight() - img->GetHeight()) / 2);

			Iw2DSetColour(0xFFFFFFFF);
			IwGxSetColClear(0, 0, 0, 0);
			Iw2DDrawImage(img, pos);
*/
			CIwTexture* pTexture;
			if (g_iStartGameCount < g_iIntroReadyCount)
			{
				pTexture = g_pReady;
			}
			else if (g_iStartGameCount < g_iIntroSetCount)
			{
				pTexture = g_pSet;
			}
			else
			{
				pTexture = g_pGo;
			}

			int textureHeight = pTexture->GetHeight() * Utils::GetImageScalingFactor();
			int textureWidth = pTexture->GetWidth() * Utils::GetImageScalingFactor();

			// Render the game here
			CIwRect backLoc(0,0,Iw2DGetSurfaceWidth(),Iw2DGetSurfaceHeight());
			Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

			CIwRect textLoc((Iw2DGetSurfaceWidth() - textureWidth) / 2, 10, textureWidth, textureHeight);
			Utils::AlphaRenderImage(pTexture, textLoc, 255);

			CIwRect contentBounds = g_pContentBlock->GetInnerBounds();

			g_pContentBlock->Render();
			//g_pScoreKeeper->Render();
			//g_pTimer->Render();
			IwGxFlush();
			IwGxClear(IW_GX_DEPTH_BUFFER_F);
			IwGxSetScreenSpaceSlot(0);
			g_pGameEngine->RenderInfo(contentBounds);
		}
		else if (g_bIsScore)
		{
			IwGxFlush();
			IwGxClear(IW_GX_DEPTH_BUFFER_F);
			IwGxSetScreenSpaceSlot(0);

			CIwUIRect rect = g_pTitleBar->GetFrame();
			IwGetUIView()->Render();
		}
		else
		{
			// Render the game here
			CIwRect backLoc(0,0,Iw2DGetSurfaceWidth(),Iw2DGetSurfaceHeight());
			Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

			g_pScoreKeeper->Render();
			g_pTimer->Render();
			g_pContentBlock->Render();
			
			IwGxFlush();

			CIwRect contentBounds = g_pContentBlock->GetInnerBounds();
			g_pGameEngine->Render(contentBounds);
		}
	}
	Iw2DFinishDrawing();
}

void ActiveGameState::ShowScore()
{
	// TODO - only set it visible once everyone has reported their score, plus some time (5 seconds)
	g_bIsScore = true;
}

void ActiveGameState::HideScore()
{
	g_bIsScore = false;
	
	g_uiStartTime = s3eTimerGetMs();
	g_iStartGameCount = 0;
	g_bRenderGame = false;
}

void ActiveGameState::RenderBackground()
{
}
