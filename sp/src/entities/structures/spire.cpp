#include "spire.h"

#include <tinker/cvar.h>

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "game/gameserver.h"
#include "entities/bots/worker.h"
#include "entities/enemies/eater.h"
#include "entities/star.h"
#include "ui/command_menu.h"

REGISTER_ENTITY(CSpire);

NETVAR_TABLE_BEGIN(CSpire);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSpire);
	SAVEDATA_DEFINE(CSaveData::DATA_STRING, tstring, m_sBaseName);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flBuildStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flNextMonster);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSpire);
INPUTS_TABLE_END();

void CSpire::Precache()
{
	PrecacheMaterial("textures/spire.mat");
}

void CSpire::Spawn()
{
	m_oVoxelTree.SetSpire(this);

	m_aabbPhysBoundingBox = AABB(Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 15.0f, 1.0f));

	BaseClass::Spawn();

	SetTotalHealth(100);

	m_flBuildStart = 0;
	m_flNextMonster = 0;

	GameData().SetCommandMenuParameters(Vector(0, 1, 0), 0.5f, 1.0f);
}

CVar build_time_worker("build_time_worker", "4");

void CSpire::Think()
{
	BaseClass::Think();

	if (GameServer()->GetGameTime() > m_flBuildStart + build_time_worker.GetFloat())
		EndBuild();

	if (GameServer()->GetGameTime() > m_flNextMonster)
	{
		CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
		Vector vecToStar = (pStar->GetGlobalOrigin() - GetGlobalOrigin()).GetUnits(SCALE_METER).Normalized();
		float flTimeOfDay = vecToStar.Dot(GetGlobalTransform().GetUpVector());

		if (flTimeOfDay < -0.15f)
		{
			TVector vecSpire = GetLocalOrigin();
			TVector vecSpawnSpot;
			bool bFoundSpot = false;

			for (size_t i = 0; i < 4; i++)
			{
				float flDistance = RandomFloat(50, 100);

				float flDegree = RandomFloat(0, 2*M_PI);

				Vector vecRight = GetLocalTransform().GetRightVector();
				Vector vecForward = GetLocalTransform().GetForwardVector();

				Vector vecOffset = sin(flDegree) * vecRight + cos(flDegree) * vecForward;

				vecSpawnSpot = vecSpire + vecOffset * flDistance;

				DoubleVector vecSpawnSpotMeters = vecSpawnSpot.GetUnits(SCALE_METER);

				CTraceResult tr;
				GamePhysics()->TraceLine(tr, GameData().TransformPointLocalToPhysics(vecSpawnSpotMeters) + Vector(0, 100, 0), GameData().TransformPointLocalToPhysics(vecSpawnSpotMeters) - Vector(0, 100, 0));

				if (tr.m_flFraction < 1)
					vecSpawnSpot = TVector(GameData().TransformPointPhysicsToLocal(tr.m_vecHit));
				else
					continue;

//				if (SPGame()->GetSPRenderer()->IsInFrustumAtScaleSidesOnly(SCALE_METER, vecSpawnSpot.GetUnits(SCALE_METER), 1))
//					continue;

				bFoundSpot = true;
				break;
			}

			if (bFoundSpot)
			{
				CEater* pMonster = GameServer()->Create<CEater>("CEater");
				pMonster->GameData().SetPlanet(GameData().GetPlanet());
				pMonster->SetMoveParent(GameData().GetPlanet());
				pMonster->SetLocalOrigin(vecSpawnSpot);
			}
		}

		m_flNextMonster = GameServer()->GetGameTime() + 20;
	}
}

bool CSpire::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CSpire::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	GetVoxelTree()->Render();

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
	vecUp = GetLocalTransform().GetUpVector();
	vecRight *= m_aabbPhysBoundingBox.m_vecMaxs.x;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();

	if (vecPosition.LengthSqr() > 1000*1000)
		return;

	if (m_flBuildStart)
		vecPosition += Vector(RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f));

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial("textures/spire.mat");

	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
		c.SetUniform("vecColor", Vector4D(1, 0, 0, 1));

	c.BeginRenderTriFan();
		c.TexCoord(0.0f, 1.0f);
		c.Vertex(-vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMaxs.y);
		c.TexCoord(0.0f, 0.0f);
		c.Vertex(-vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMins.y);
		c.TexCoord(1.0f, 0.0f);
		c.Vertex(vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMins.y);
		c.TexCoord(1.0f, 1.0f);
		c.Vertex(vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMaxs.y);
	c.EndRender();
}

void CSpire::PerformStructureTask(CSPCharacter* pUser)
{
	CPlayerCharacter* pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(pUser);
	if (pPlayerCharacter)
	{
		CCommandMenu* pMenu = GameData().CreateCommandMenu(pPlayerCharacter);
		SetupMenuButtons();

		if (!pMenu->GetNumActiveButtons())
			GameData().CloseCommandMenu();
	}
}

void CSpire::SetupMenuButtons()
{
	CCommandMenu* pMenu = GameData().GetCommandMenu();

	pMenu->SetButton(0, "Worker", "worker");
}

void CSpire::MenuCommand(const tstring& sCommand)
{
	if (sCommand == "worker")
		StartBuildWorker();

	GameData().CloseCommandMenu();
}

void CSpire::PostConstruction()
{
	BaseClass::PostConstruction();

	StartBuildWorker();
}

void CSpire::StartBuildWorker()
{
	if (m_flBuildStart)
		return;

	m_flBuildStart = GameServer()->GetGameTime();
}

void CSpire::EndBuild()
{
	if (!m_flBuildStart)
		return;

	m_flBuildStart = 0;

	CWorkerBot* pWorker = GameServer()->Create<CWorkerBot>("CWorkerBot");
	pWorker->GameData().SetPlayerOwner(GameData().GetPlayerOwner());
	pWorker->GameData().SetPlanet(GameData().GetPlanet());
	pWorker->SetOwner(GetOwner());
	pWorker->SetGlobalOrigin(GameData().GetPlanet()->GetGlobalOrigin()); // Avoid floating point precision problems
	pWorker->SetMoveParent(GameData().GetPlanet());
	pWorker->SetLocalOrigin(GetLocalOrigin() + GetLocalTransform().GetUpVector() + GetLocalTransform().GetRightVector()*2);

	CTraceResult tr;
	GamePhysics()->TraceLine(tr, pWorker->GetPhysicsTransform().GetTranslation() + Vector(0, 10, 0), pWorker->GetPhysicsTransform().GetTranslation() - Vector(0, 10, 0), this);

	if (tr.m_flFraction < 1)
	{
		Matrix4x4 mTransform = pWorker->GetPhysicsTransform();
		mTransform.SetTranslation(tr.m_vecHit + Vector(0, 1, 0));
	
		pWorker->SetPhysicsTransform(mTransform);
	}
}
