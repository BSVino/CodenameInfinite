#include "hud.h"

#include <tengine/game/gameserver.h>
#include <tengine/renderer/game_renderer.h>
#include <glgui/rootpanel.h>
#include <glgui/label.h>
#include <tinker/cvar.h>

#include "planet/planet.h"
#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "entities/bots/helper.h"

CSPHUD::CSPHUD()
	: CHUDViewport()
{
}

tstring GetStringDistance(const CScalableFloat& v)
{
	float flDistance = (float)v.GetUnits(SCALE_KILOMETER);
	if (flDistance > 10000000)
	{
		flDistance /= 149598000;		// Km in an AU
		if (flDistance > 30000)
		{
			flDistance /= 63239.6717f;	// AU in a LY
			return sprintf("%.2fly", flDistance);
		}

		if (flDistance < 1)
			return sprintf("%.4fau", flDistance);
		else
			return sprintf("%.2fau", flDistance);
	}

	return sprintf("%.2fkm", flDistance);
}

void CSPHUD::Paint(float x, float y, float w, float h)
{
	BaseClass::Paint(x, y, w, h);

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(pEntity);
		if (pPlanet)
		{
			CScalableVector vecScalablePlanet = pPlanet->GameData().GetScalableRenderOrigin();

			CScalableFloat flDistance = vecScalablePlanet.Length();
			if (flDistance < pPlanet->GetRadius()*2.0f)
				continue;

			Vector vecPlanet = vecScalablePlanet.GetUnits(SCALE_METER);

			if (vecForward.Dot((vecPlanet).Normalized()) < 0)
				continue;

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = pPlanet->GetPlanetName() + " - " + GetStringDistance(flDistance);

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}
	}

	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();
	CHelperBot* pHelper = pLocalPlayer->GetHelperBot();
	if (pHelper)
	{
		CScalableVector vecScalablePlanet = pHelper->GameData().GetScalableRenderOrigin();

		Vector vecPlanet = vecScalablePlanet.GetUnits(SCALE_METER);

		if (vecForward.Dot((vecPlanet).Normalized()) > 0)
		{
			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = "Helper-Bot";

			float flFontHeight = glgui::CLabel::GetFontHeight("sans-serif", 16);
			float flTextWidth = glgui::CLabel::GetTextWidth(sLabel, sLabel.length(), "sans-serif", 16);

			vecScreen.x += 15;
			vecScreen.y -= 15;

			if (vecScreen.x < 0)
				vecScreen.x = 0;
			if (vecScreen.y < 0)
				vecScreen.y = 0;
			if (vecScreen.x > glgui::RootPanel()->GetWidth() - flTextWidth)
				vecScreen.x = glgui::RootPanel()->GetWidth() - flTextWidth;
			if (vecScreen.y > glgui::RootPanel()->GetHeight() - flFontHeight)
				vecScreen.y = glgui::RootPanel()->GetHeight() - flFontHeight;

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x, vecScreen.y);
		}
	}

	Debug_Paint();
}

void CSPHUD::Debug_Paint()
{
	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CPlayerCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
}
