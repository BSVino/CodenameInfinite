#include "sp_game.h"

#include <tinker/application.h>
#include <renderer/renderer.h>
#include <game/level.h>

#include "sp_player.h"
#include "sp_character.h"
#include "sp_camera.h"
#include "sp_renderer.h"

CGame* CreateGame()
{
	return GameServer()->Create<CSPGame>("CSPGame");
}

CRenderer* CreateRenderer()
{
	return new CSPRenderer();
}

CCamera* CreateCamera()
{
	CCamera* pCamera = new CSPCamera();
	return pCamera;
}

CLevel* CreateLevel()
{
	return new CLevel();
}

REGISTER_ENTITY(CSPGame);

NETVAR_TABLE_BEGIN(CSPGame);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPGame);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPGame);
INPUTS_TABLE_END();

void CSPGame::Precache()
{
}

void CSPGame::Think()
{
	BaseClass::Think();
}

void CSPGame::Simulate()
{
	BaseClass::Simulate();

	float flSimulationFrameTime = 0.01f;

	eastl::vector<ISPEntity*> apSimulateList;
	apSimulateList.reserve(CBaseEntity::GetNumEntities());
	apSimulateList.clear();

	size_t iMaxEntities = GameServer()->GetMaxEntities();
	for (size_t i = 0; i < iMaxEntities; i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		if (pEntity->IsDeleted())
			continue;

		ISPEntity* pSPEntity = dynamic_cast<ISPEntity*>(pEntity);
		if (!pSPEntity)
			continue;

		pSPEntity->SetLastLocalScalableOrigin(pSPEntity->GetLocalScalableOrigin());

		if (!pEntity->ShouldSimulate())
			continue;

		apSimulateList.push_back(pSPEntity);
	}

	// Move all entities
	for (size_t i = 0; i < apSimulateList.size(); i++)
	{
		ISPEntity* pEntity = apSimulateList[i];
		if (!pEntity)
			continue;

		CScalableMatrix mGlobalToLocalRotation = pEntity->GetGlobalToLocalScalableTransform();
		mGlobalToLocalRotation.SetTranslation(CScalableVector());

		// Break simulations up into very small steps in order to preserve accuracy.
		// I think floating point precision causes this problem but I'm not sure. Anyway this works better for my projectiles.
		for (float flCurrentSimulationTime = GameServer()->GetSimulationTime(); flCurrentSimulationTime < GameServer()->GetGameTime(); flCurrentSimulationTime += flSimulationFrameTime)
		{
			CScalableVector vecVelocity = pEntity->GetLocalScalableVelocity();
			if (vecVelocity.IsZero())
				continue;

			CScalableVector vecGlobalGravity = pEntity->GetGlobalScalableGravity();
			CScalableVector vecLocalGravity;
			if (!vecGlobalGravity.IsZero())
			{
				CScalableFloat flLength = vecGlobalGravity.Length();
				vecLocalGravity = (mGlobalToLocalRotation * (vecGlobalGravity/flLength))*flLength;
				pEntity->SetLocalScalableVelocity(vecVelocity + vecLocalGravity * flSimulationFrameTime);
			}
			else
				vecLocalGravity = CScalableVector();

			pEntity->SetLocalScalableOrigin(pEntity->GetLocalScalableOrigin() + vecVelocity * flSimulationFrameTime);
		}
	}

	while (GameServer()->GetSimulationTime() < GameServer()->GetGameTime())
		GameServer()->AdvanceSimulationTime(flSimulationFrameTime);

	for (size_t i = 0; i < apSimulateList.size(); i++)
	{
		ISPEntity* pEntity = apSimulateList[i];
		if (!pEntity)
			continue;

		for (size_t j = 0; j < iMaxEntities; j++)
		{
			CBaseEntity* pEntity2 = CBaseEntity::GetEntity(j);

			if (!pEntity2)
				continue;

			if (pEntity2->IsDeleted())
				continue;

			ISPEntity* pSPEntity2 = dynamic_cast<ISPEntity*>(pEntity2);
			if (!pSPEntity2)
				continue;

			if (pSPEntity2 == pEntity)
				continue;

			if (!pEntity->ShouldTouch(pSPEntity2))
				continue;

			CScalableVector vecPoint;
			if (pEntity->IsTouching(pSPEntity2, vecPoint))
			{
				pEntity->SetGlobalScalableOrigin(vecPoint);
				pEntity->Touching(pSPEntity2);
			}
		}
	}
}

CSPCharacter* CSPGame::GetLocalPlayerCharacter()
{
	return static_cast<CSPCharacter*>(GetLocalPlayer()->GetCharacter());
}

CSPRenderer* CSPGame::GetSPRenderer()
{
	return static_cast<CSPRenderer*>(GameServer()->GetRenderer());
}

CSPCamera* CSPGame::GetSPCamera()
{
	return static_cast<CSPCamera*>(GameServer()->GetCamera());
}
