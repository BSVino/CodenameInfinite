#include "pickup.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"

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
	PrecacheMaterial("textures/items1.mat");
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

	c.UseMaterial("textures/items1.mat");

	c.BeginRenderTriFan();
		c.TexCoord(0.0f, 1.0f);
		c.Vertex(-vecRight + vecUp);
		c.TexCoord(0.0f, 0.75f);
		c.Vertex(-vecRight - vecUp);
		c.TexCoord(0.25f, 0.75f);
		c.Vertex(vecRight - vecUp);
		c.TexCoord(0.25f, 1.0f);
		c.Vertex(vecRight + vecUp);
	c.EndRender();
}

void CPickup::SetItem(item_t eItem)
{
	m_eItem = eItem;
}
