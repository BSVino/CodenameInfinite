#include "sp_entity.h"

#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_character.h"

REGISTER_ENTITY(CSPEntity);

NETVAR_TABLE_BEGIN(CSPEntity);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPEntity);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPEntity);
INPUTS_TABLE_END();

bool CSPEntity::IsTouchingLocal(CBaseEntity* pOther, const TVector& vecDestination, TVector& vecPoint)
{
	if ((pOther->GetGlobalOrigin() - GetGlobalOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::IsTouchingLocal(pOther, vecDestination, vecPoint);
}

bool CSPEntity::IsTouching(CBaseEntity* pOther, const TVector& vecDestination, TVector& vecPoint)
{
	if ((pOther->GetGlobalOrigin() - GetGlobalOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::IsTouchingLocal(pOther, vecDestination, vecPoint);
}

CScalableMatrix CSPEntity::GetScalableRenderTransform() const
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
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
