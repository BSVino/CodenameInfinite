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

const Matrix4x4 CSPEntity::GetRenderTransform() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetUnits(eScale);
}

const Vector CSPEntity::GetRenderOrigin() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetTranslation().GetUnits(eScale);
}
