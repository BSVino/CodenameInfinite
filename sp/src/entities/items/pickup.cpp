#include "pickup.h"

#include <mtrand.h>

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_character.h"

REGISTER_ENTITY(CPickup);

NETVAR_TABLE_BEGIN(CPickup);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPickup);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, item_t, m_eItem);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPickup);
INPUTS_TABLE_END();

void CPickup::Precache()
{
	PrecacheMaterial("textures/items/dirt.mat");
	PrecacheMaterial("textures/items/stone.mat");
	PrecacheMaterial("textures/items/wood.mat");
}

void CPickup::Spawn()
{
	BaseClass::Spawn();

	m_flNextPlayerCheck = 0;
}

void CPickup::Think()
{
	BaseClass::Think();

	if (GameServer()->GetGameTime() > m_flNextPlayerCheck)
	{
		CSPCharacter* pNearestCharacter = nullptr;
		for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
		{
			CBaseEntity* pEnt = CBaseEntity::GetEntity(i);
			if (!pEnt)
				continue;

			CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(pEnt);
			if (!pCharacter)
				continue;

			if (!pCharacter->CanPickUp(this))
				continue;

			if (!pNearestCharacter)
			{
				pNearestCharacter = pCharacter;
				continue;
			}

			if ((pCharacter->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pNearestCharacter->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
				pNearestCharacter = pCharacter;
		}

		if (pNearestCharacter && (pNearestCharacter->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < CScalableFloat(2.5f, SCALE_METER).Squared())
			pNearestCharacter->PickUp(this);

		m_flNextPlayerCheck = GameServer()->GetGameTime() + (double)RandomFloat(0.4f, 0.8f);
	}
}

bool CPickup::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CPickup::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);

	float flBlockSize = 0.25f;

	vecUp = GetLocalTransform().GetUpVector() * flBlockSize;
	vecRight *= flBlockSize;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();

	if (vecPosition.LengthSqr() > 100*100)
		return;

	CRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial(GetItemMaterial(m_eItem));

	c.BeginRenderTriFan();
		c.TexCoord(0.0f, 1.0f);
		c.Vertex(-vecRight + vecUp);
		c.TexCoord(0.0f, 0.0f);
		c.Vertex(-vecRight - vecUp);
		c.TexCoord(1.0f, 0.0f);
		c.Vertex(vecRight - vecUp);
		c.TexCoord(1.0f, 1.0f);
		c.Vertex(vecRight + vecUp);
	c.EndRender();
}

void CPickup::SetItem(item_t eItem)
{
	m_eItem = eItem;
}
