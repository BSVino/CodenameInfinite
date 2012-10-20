#include "pallet.h"

#include <tinker/cvar.h>
#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "entities/items/pickup.h"

REGISTER_ENTITY(CPallet);

NETVAR_TABLE_BEGIN(CPallet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPallet);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPallet);
INPUTS_TABLE_END();

const int CPallet::PALLET_CAPACITY = 100;

void CPallet::Precache()
{
	PrecacheMaterial("textures/pallet.mat");
}

void CPallet::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-1.0f, -0.2f, -1.0f), Vector(1.0f, 1.8f, 1.0f));

	BaseClass::Spawn();

	m_iQuantity = 0;
}

void CPallet::Think()
{
	BaseClass::Think();
}

bool CPallet::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CPallet::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
	vecUp = GetLocalTransform().GetUpVector();
	vecRight *= m_aabbPhysBoundingBox.m_vecMaxs.x;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();

	if (vecPosition.LengthSqr() > 1000*1000)
		return;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial("textures/pallet.mat");

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

	if (m_iQuantity)
	{
		Vector vecPosition = BaseGetRenderTransform().GetTranslation();

		float flHorizontalScale = RemapValClamped((float)m_iQuantity, 1, PALLET_CAPACITY/10, 0.1f, 0.9f);
		float flVerticalScale = RemapValClamped((float)m_iQuantity/10, 1, PALLET_CAPACITY/10, 0.1f, 5.0f);

		Vector vecForward = GetLocalTransform().GetForwardVector() * flHorizontalScale;
		Vector vecUp = GetLocalTransform().GetUpVector() * flVerticalScale;
		Vector vecRight = GetLocalTransform().GetRightVector() * flHorizontalScale;

		vecPosition = vecPosition
			- GetLocalTransform().GetForwardVector() * (flHorizontalScale/2)
			+ GetLocalTransform().GetUpVector()/2
			- GetLocalTransform().GetRightVector() * (flHorizontalScale/2);

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

void CPallet::PerformStructureTask(class CSPCharacter* pCharacter)
{
	BaseClass::PerformStructureTask(pCharacter);

	size_t iTaken = pCharacter->TakeBlocks(m_eItem, std::min(pCharacter->MaxInventory(), m_iQuantity));

	if (iTaken > m_iQuantity)
		iTaken = m_iQuantity;

	m_iQuantity -= iTaken;
}

size_t CPallet::TakeBlocks(item_t eBlock, size_t iNumber)
{
	if (m_iQuantity != 0 && eBlock != m_eItem)
		return 0;

	if (!m_iQuantity)
		m_eItem = eBlock;

	if (iNumber + m_iQuantity > PALLET_CAPACITY)
		iNumber = PALLET_CAPACITY - m_iQuantity;

	m_iQuantity += iNumber;

	return iNumber;
}
