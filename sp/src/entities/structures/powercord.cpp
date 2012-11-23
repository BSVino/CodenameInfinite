#include "powercord.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>
#include <tengine/renderer/roperenderer.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"

#include "structure.h"

REGISTER_ENTITY(CPowerCord);

NETVAR_TABLE_BEGIN(CPowerCord);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPowerCord);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CStructure, m_hSource);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CStructure, m_hDestination);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPowerCord);
INPUTS_TABLE_END();

void CPowerCord::Precache()
{
	PrecacheMaterial("textures/cord.mat");
}

void CPowerCord::Spawn()
{
	BaseClass::Spawn();
}

bool CPowerCord::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CPowerCord::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	if (!m_hSource)
		return;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();
	Vector vecGoal;
	if (m_hDestination)
		vecGoal = m_hDestination->BaseGetRenderOrigin() + m_hDestination->GetLocalOrigin().Normalized();
	else
		vecGoal = GameServer()->GetRenderer()->GetCameraPosition() + GameServer()->GetRenderer()->GetCameraVector();

	CRopeRenderer c(GameServer()->GetRenderer(), CMaterialHandle("textures/cord.mat"), vecPosition, 0.01f);

	Vector vecUp;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, nullptr, &vecUp);

	float a = 1;
	float cosh1 = a*cosh(1/a);

	size_t iLinks = 11;
	for (size_t i = 1; i < iLinks-1; i++)
	{
		Vector vecPositionLateral = LerpValue<Vector>(vecPosition, vecGoal, (float)i, 0, (float)iLinks-1);
		Vector vecCatenary = (cosh1 - a * cosh(RemapVal((float)i, 0, (float)iLinks-1, -1, 1) / a)) * vecUp;
		c.AddLink(vecPositionLateral - vecCatenary);
	}

	c.Finish(vecGoal);
}

void CPowerCord::SetSource(CStructure* pSource)
{
	TAssert(pSource);
	TAssert(!m_hSource);
	m_hSource = pSource;

	SetMoveParent(pSource->GetMoveParent());
	GameData().SetPlanet(pSource->GameData().GetPlanet());
	SetLocalOrigin(pSource->GetLocalOrigin() + pSource->GetLocalOrigin().Normalized());
}

void CPowerCord::SetDestination(CStructure* pDestination)
{
	TAssert(pDestination);

	if (!pDestination)
		return;

	TAssert(!m_hDestination);
	m_hDestination = pDestination;

	pDestination->SetPowerCord(this);
}

CStructure* CPowerCord::GetSource() const
{
	return m_hSource;
}
