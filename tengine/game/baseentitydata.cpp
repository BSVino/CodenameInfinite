#include "baseentitydata.h"

#include "baseentity.h"

void CBaseEntityData::SetEntity(CBaseEntity* pEntity)
{
	m_pEntity = pEntity;
}

void CBaseEntityData::Think()
{
	return m_pEntity->Think();
}

Matrix4x4 CBaseEntityData::GetRenderTransform() const
{
	return m_pEntity->GetRenderTransform();
}

Vector CBaseEntityData::GetRenderOrigin() const
{
	return m_pEntity->GetRenderOrigin();
}

bool CBaseEntityData::CollideLocal(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return m_pEntity->CollideLocal(pWith, v1, v2, tr);
}

bool CBaseEntityData::Collide(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return m_pEntity->Collide(pWith, v1, v2, tr);
}

