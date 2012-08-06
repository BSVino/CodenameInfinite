#include "hudviewport.h"

#include <glgui/rootpanel.h>
#include <tinker/cvar.h>
#include <glgui/label.h>
#include <game/gameserver.h>
#include <game/entities/game.h>
#include <game/entities/character.h>

CHUDViewport::CHUDViewport()
	: glgui::CPanel(0, 0, glgui::CRootPanel::Get()->GetWidth(), glgui::CRootPanel::Get()->GetHeight())
{
}

void CHUDViewport::Layout()
{
	SetSize(glgui::CRootPanel::Get()->GetWidth(), glgui::CRootPanel::Get()->GetHeight());

	BaseClass::Layout();
}

CVar cl_debug("cl_debug", "off");

void CHUDViewport::Paint(float x, float y, float w, float h)
{
	if (cl_debug.GetBool())
	{
		glgui::CLabel::PaintText(sprintf("FPS: %d", (int)(1.0f/GameServer()->GetFrameTime())), -1, "sans-serif", 10, 15, 15);
		CPlayer* pPlayer = Game()->GetLocalPlayer();
		if (pPlayer)
		{
			CCharacter* pCharacter = pPlayer->GetCharacter();
			TVector vecPlayer = pCharacter->GetGlobalOrigin();
			TVector vecVelocity = pCharacter->GetGlobalVelocity();
			tstring sPlayer = sprintf("Player: %.2f %.2f %.2f  Velocity: %.2f %.2f %.2f",
				(float)vecPlayer.x, (float)vecPlayer.y, (float)vecPlayer.z,
				(float)vecVelocity.x, (float)vecVelocity.y, (float)vecVelocity.z);
			glgui::CLabel::PaintText(sPlayer, -1, "sans-serif", 10, 15, 30);
		}
	}

	BaseClass::Paint(x, y, w, h);
}
