#include "hud.h"

#include <tengine/game/gameserver.h>
#include <tengine/renderer/renderer.h>

#include "../planet.h"
#include "../sp_character.h"

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
			CScalableVector vecScalablePlanet = pPlanet->GetScalableRenderOrigin();

			CScalableFloat flDistance = vecScalablePlanet.Length();
			if (flDistance < pPlanet->GetRadius()*2)
				continue;

			Vector vecPlanet = vecScalablePlanet.GetUnits(SCALE_METER);

			if (vecForward.Dot((vecPlanet).Normalized()) < 0)
				continue;

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = pPlanet->GetPlanetName() + " - " + sprintf("%.2fkm", flDistance.GetUnits(SCALE_KILOMETER));

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}
	}
}
