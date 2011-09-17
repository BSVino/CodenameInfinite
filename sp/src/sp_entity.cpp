#include "sp_entity.h"

#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"
#include "planet.h"
#include "star.h"

bool CSPEntityData::ShouldRenderAtScale(scale_t eScale) const
{
	// Preferably should do this in some way that doesn't require dynamic_cast.
	CPlanet* pPlanet = dynamic_cast<CPlanet*>(m_pEntity);

	if (pPlanet)
		return pPlanet->ShouldRenderAtScale(eScale);

	if (dynamic_cast<CStar*>(m_pEntity))
		return true;

	return (eScale == GetScale());
}

bool CSPEntityData::CollideLocal(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	if (v1.Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::CollideLocal(pWith, v1, v2, tr);
}

bool CSPEntityData::Collide(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	if ((v1 - m_pEntity->GetGlobalOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	return BaseClass::Collide(pWith, v1, v2, tr);
}

CScalableMatrix CSPEntityData::GetScalableRenderTransform() const
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

	CScalableMatrix mTransform = m_pEntity->GetGlobalTransform();
	mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

	return mTransform;
}

CScalableVector CSPEntityData::GetScalableRenderOrigin() const
{
	return GetScalableRenderTransform().GetTranslation();
}

Matrix4x4 CSPEntityData::GetRenderTransform() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetUnits(eScale);
}

Vector CSPEntityData::GetRenderOrigin() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetTranslation().GetUnits(eScale);
}
