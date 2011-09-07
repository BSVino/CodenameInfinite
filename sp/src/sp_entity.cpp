#include "sp_entity.h"

#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"

REGISTER_ENTITY(CSPEntity);

NETVAR_TABLE_BEGIN(CSPEntity);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPEntity);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPEntity);
INPUTS_TABLE_END();

bool CSPEntity::CollideLocal(const TVector& v1, const TVector& v2, TVector& vecPoint, TVector& vecNormal)
{
	if (v1.Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::CollideLocal(v1, v2, vecPoint, vecNormal);
}

bool CSPEntity::Collide(const TVector& v1, const TVector& v2, TVector& vecPoint, TVector& vecNormal)
{
	if ((v1 - GetGlobalOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::Collide(v1, v2, vecPoint, vecNormal);
}

CScalableMatrix CSPEntity::GetScalableRenderTransform() const
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

	CScalableMatrix mTransform = GetGlobalTransform();
	mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

	return mTransform;
}

CScalableVector CSPEntity::GetScalableRenderOrigin() const
{
	return GetScalableRenderTransform().GetTranslation();
}

Matrix4x4 CSPEntity::GetRenderTransform() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetUnits(eScale);
}

Vector CSPEntity::GetRenderOrigin() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetTranslation().GetUnits(eScale);
}
