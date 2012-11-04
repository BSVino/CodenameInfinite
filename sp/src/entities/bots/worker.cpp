#include "worker.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "ui/command_menu.h"
#include "entities/structures/spire.h"
#include "entities/structures/mine.h"
#include "entities/structures/pallet.h"

REGISTER_ENTITY(CWorkerBot);

NETVAR_TABLE_BEGIN(CWorkerBot);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CWorkerBot);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CWorkerBot);
INPUTS_TABLE_END();

void CWorkerBot::Precache()
{
	PrecacheMaterial("textures/workerbot.mat");
}

void CWorkerBot::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-0.3f, -0.3f, -0.3f), Vector(0.3f, 0.3f, 0.3f));

	BaseClass::Spawn();

	SetTotalHealth(4);
}

void CWorkerBot::Think()
{
	BaseClass::Think();
}

bool CWorkerBot::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CWorkerBot::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	float flRadius = GetPhysBoundingBox().Size().Length()/2;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.UseMaterial("textures/workerbot.mat");
	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
		c.SetUniform("vecColor", Vector4D(1, 0, 0, 1));

	c.ResetTransformations();
	c.Transform(BaseGetRenderTransform());

	c.RenderBillboard("textures/workerbot.mat", flRadius);

	if (IsHoldingABlock())
	{
		CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		Vector vecPosition = GetLocalOrigin() - pCharacter->GetLocalOrigin() + GetLocalUpVector()*0.8f;
		vecPosition -= Vector(0.25f, 0.25f, 0.25f);

		Vector vecForward = BaseGetRenderTransform().GetForwardVector()/2;
		Vector vecUp = BaseGetRenderTransform().GetUpVector()/2;
		Vector vecRight = BaseGetRenderTransform().GetRightVector()/2;

		CGameRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();
		c.Translate(vecPosition);

		c.UseMaterial("textures/items1.mat");

		c.BeginRenderTriFan();
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(Vector(0, 0, 0));
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecRight);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecRight + vecUp);
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecUp);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecUp + vecForward);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecForward);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecForward + vecRight);
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecRight);
		c.EndRender();

		c.BeginRenderTriFan();
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecRight + vecUp + vecForward);
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(vecUp + vecForward);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecUp);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecRight + vecUp);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecRight);
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(vecForward + vecRight);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecForward);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecForward + vecUp);
		c.EndRender();
	}
}

void CWorkerBot::SetupMenuButtons()
{
	CCommandMenu* pMenu = GameData().GetCommandMenu();

	pMenu->SetTitle("Worker");
	pMenu->SetSubtitle("Task: " + TaskToString(GetTask()));
	pMenu->SetButton(0, "Build", "build");
	pMenu->SetButton(1, "Mine", "mine");
	pMenu->SetButton(4, "On me", "follow");
	pMenu->SetButton(5, "Sit tight", "nothing");

	CSpire* pSpire = GetSpire();
	if (pSpire)
	{
		bool bThingsToBuild = false;
		bool bPalletWithStuff = false;
		for (size_t i = 0; i < pSpire->GetStructures().size(); i++)
		{
			CStructure* pStructure = pSpire->GetStructures()[i];
			TAssert(pStructure);
			if (!pStructure)
				continue;

			if (!bPalletWithStuff)
			{
				CPallet* pPallet = dynamic_cast<CPallet*>(pStructure);
				if (pPallet && pPallet->GetBlockQuantity())
				{
					bPalletWithStuff = true;
					continue;
				}
			}

			if (!pStructure->IsUnderConstruction())
				continue;

			bThingsToBuild = true;
			break;
		}

		bool bDesignations = false;
		if (!bThingsToBuild)
		{
			IVector vecDesignation = pSpire->GetVoxelTree()->FindNearbyDesignation(GetLocalOrigin());
			if (vecDesignation != IVector(0, 0, 0))
				bDesignations = true;
		}

		bool bMinesAvailable = false;
		for (size_t i = 0; i < pSpire->GetMines().size(); i++)
		{
			CMine* pMine = pSpire->GetMines()[i];
			TAssert(pMine);
			if (!pMine)
				continue;

			if (pMine->IsUnderConstruction())
				continue;

			bMinesAvailable = true;
			break;
		}

		if (!bThingsToBuild && (!bDesignations || !bPalletWithStuff))
		{
			pMenu->SetButtonEnabled(0, false);
			if (bDesignations && !bPalletWithStuff)
				pMenu->SetButtonToolTip(0, "No building materials");
			else
				pMenu->SetButtonToolTip(0, "No structures to build");
		}

		if (!bMinesAvailable)
		{
			pMenu->SetButtonEnabled(1, false);
			pMenu->SetButtonToolTip(1, "No mines available");
		}
	}
	else
	{
		pMenu->SetButtonEnabled(0, false);
		pMenu->SetButtonEnabled(1, false);
	}
}

void CWorkerBot::MenuCommand(const tstring& sCommand)
{
	if (sCommand == "build")
		SetTask(TASK_BUILD);
	else if (sCommand == "mine")
		SetTask(TASK_MINE);
	else if (sCommand == "follow")
		SetTask(TASK_FOLLOWME);
	else
		SetTask(TASK_NONE);

	GameData().CloseCommandMenu();
}

CScalableFloat CWorkerBot::CharacterSpeed()
{
	return CScalableFloat(2.0f, SCALE_METER);
}
