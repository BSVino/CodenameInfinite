#include "hud.h"

#include <tengine/game/gameserver.h>
#include <tengine/renderer/renderer.h>

#include "../planet.h"

CSPHUD::CSPHUD()
	: glgui::CPanel(0, 0, glgui::CRootPanel::Get()->GetWidth(), glgui::CRootPanel::Get()->GetHeight())
{
}

void CSPHUD::Paint(int x, int y, int w, int h)
{
	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			return;

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(pEntity);
		if (pPlanet)
		{
			Vector vecPlanet = pPlanet->GetScalableRenderOrigin().GetUnits(SCALE_METER);

			if (vecForward.Dot((vecPlanet).Normalized()) < 0)
				continue;

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			glgui::CLabel::PaintText(pPlanet->GetPlanetName(), pPlanet->GetPlanetName().length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}
	}
}
