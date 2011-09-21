#include "hud.h"

#include <tengine/game/gameserver.h>
#include <tengine/renderer/renderer.h>
#include <tinker/cvar.h>

#include "../planet.h"
#include "../terrain_chunks.h"
#include "../sp_playercharacter.h"
#include "../sp_game.h"

CSPHUD::CSPHUD()
	: glgui::CPanel(0, 0, glgui::CRootPanel::Get()->GetWidth(), glgui::CRootPanel::Get()->GetHeight())
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

void CSPHUD::Paint(int x, int y, int w, int h)
{
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

	Debug_Paint();
}

CVar r_chunkdepth("r_chunkdepth", "off");

void CSPHUD::Debug_Paint()
{
	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CPlayerCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();

	if (r_chunkdepth.GetBool() && pLocalCharacter)
	{
		CPlanet* pPlanet = pLocalCharacter->GetNearestPlanet();
		if (pPlanet)
		{
			const CTerrainChunkManager* pChunks = pPlanet->GetTerrainChunkManager();
			size_t iChunks = pChunks->GetNumChunks();
			for (size_t i = 0; i < iChunks; i++)
			{
				CTerrainChunk* pChunk = pChunks->GetChunk(i);
				if (!pChunk)
					continue;

				CScalableVector vecBranchCenter = pPlanet->GetGlobalTransform() * CScalableVector(pChunk->GetBranch()->GetCenter(), pPlanet->GetScale());
				vecBranchCenter -= pLocalCharacter->GetGlobalOrigin();

				Vector vecPlanet = vecBranchCenter.GetUnits(SCALE_METER);

				if (vecForward.Dot((vecPlanet).Normalized()) < 0)
					continue;

				Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);
				CScalableFloat flDistance = vecBranchCenter.Length();

				tstring sLabel = GetStringDistance(flDistance) + " - " + sprintf("%d", pChunk->GetDepth());

				glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
			}
		}
	}
}
