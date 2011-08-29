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
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CBaseEntity>, m_hScalableMoveParent);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYVECTOR, CEntityHandle<CBaseEntity>, m_ahScalableMoveChildren);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bGlobalScalableTransformsDirty);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableMatrix, m_mGlobalScalableTransform);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecGlobalScalableGravity);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableMatrix, m_mLocalScalableTransform);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLocalScalableOrigin);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLastLocalScalableOrigin);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, EAngle, m_angLocalScalableAngles);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLocalScalableVelocity);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPEntity);
INPUTS_TABLE_END();

ISPEntity::ISPEntity()
{
	m_bGlobalScalableTransformsDirty = true;
}

void ISPEntity::SetScalableMoveParent(ISPEntity* pParent)
{
	if (GetScalableMoveParent() == pParent)
		return;

	ISPEntity* pOldParent = GetScalableMoveParent();

	if (pOldParent != NULL)
	{
		CScalableMatrix mPreviousGlobal = GetGlobalScalableTransform();
		CScalableVector vecPreviousVelocity = GetGlobalScalableVelocity();
		CScalableVector vecPreviousLastOrigin = mPreviousGlobal * GetLastLocalScalableOrigin();

		for (size_t i = 0; i < pOldParent->m_ahScalableMoveChildren.size(); i++)
		{
			if (pOldParent->m_ahScalableMoveChildren[i]->GetHandle() == dynamic_cast<CBaseEntity*>(this)->GetHandle())
			{
				pOldParent->m_ahScalableMoveChildren.erase(pOldParent->m_ahScalableMoveChildren.begin() + i);
				break;
			}
		}
		m_hScalableMoveParent = NULL;

		m_vecLocalScalableVelocity = vecPreviousVelocity;
		m_mLocalScalableTransform = mPreviousGlobal;
		m_vecLastLocalScalableOrigin = vecPreviousLastOrigin;
		m_vecLocalScalableOrigin = mPreviousGlobal.GetTranslation();
		m_angLocalScalableAngles = mPreviousGlobal.GetAngles();

		InvalidateGlobalScalableTransforms();
	}

	CScalableVector vecPreviousVelocity = GetLocalScalableVelocity();
	CScalableVector vecPreviousLastOrigin = GetLastLocalScalableOrigin();
	CScalableMatrix mPreviousTransform = GetLocalScalableTransform();

	m_hScalableMoveParent = dynamic_cast<CBaseEntity*>(pParent);

	if (!pParent)
		return;

	pParent->m_ahScalableMoveChildren.push_back(dynamic_cast<CBaseEntity*>(this));

	CScalableMatrix mGlobalToLocal = GetScalableMoveParent()->GetGlobalToLocalScalableTransform();

	m_vecLastLocalScalableOrigin = mGlobalToLocal * vecPreviousLastOrigin;
	m_mLocalScalableTransform = mGlobalToLocal * mPreviousTransform;
	m_vecLocalScalableOrigin = m_mLocalScalableTransform.GetTranslation();
	m_angLocalScalableAngles = m_mLocalScalableTransform.GetAngles();

	CScalableFloat flVelocityLength = vecPreviousVelocity.Length();
	if (flVelocityLength.GetUnits(SCALE_METER) > 0)
	{
		mGlobalToLocal.SetTranslation(CScalableVector());
		m_vecLocalScalableVelocity = (mGlobalToLocal * (vecPreviousVelocity/flVelocityLength))*flVelocityLength;
	}
	else
		m_vecLocalScalableVelocity = CScalableVector();

	InvalidateGlobalScalableTransforms();
}

ISPEntity* ISPEntity::GetScalableMoveParent() const
{
	CBaseEntity* pEntity = m_hScalableMoveParent;
	if (!pEntity)
		return NULL;

	return dynamic_cast<ISPEntity*>(pEntity);
}

void ISPEntity::InvalidateGlobalScalableTransforms()
{
	m_bGlobalScalableTransformsDirty = true;

	for (size_t i = 0; i < m_ahScalableMoveChildren.size(); i++)
		dynamic_cast<ISPEntity*>(m_ahScalableMoveChildren[i].GetPointer())->InvalidateGlobalScalableTransforms();
}

const CScalableMatrix& ISPEntity::GetGlobalScalableTransform()
{
	if (!m_bGlobalScalableTransformsDirty)
		return m_mGlobalScalableTransform;

	if (m_hScalableMoveParent == NULL)
		m_mGlobalScalableTransform = m_mLocalScalableTransform;
	else
		m_mGlobalScalableTransform = GetScalableMoveParent()->GetGlobalScalableTransform() * m_mLocalScalableTransform;

	m_bGlobalScalableTransformsDirty = false;

	return m_mGlobalScalableTransform;
}

CScalableMatrix ISPEntity::GetGlobalScalableTransform() const
{
	if (!m_bGlobalScalableTransformsDirty)
		return m_mGlobalScalableTransform;

	// We don't want to have to call this function to get the global transform when we can't recalculate it
	// due to being const. It's okay for objects with no move parent but if it happens a lot with objects
	// with a move parent it can become a perf problem.
	TAssert(m_hScalableMoveParent == NULL);

	if (m_hScalableMoveParent == NULL)
		return m_mLocalScalableTransform;
	else
		return GetScalableMoveParent()->GetGlobalScalableTransform() * m_mLocalScalableTransform;
}

void ISPEntity::SetGlobalScalableTransform(const CScalableMatrix& m)
{
	m_hScalableMoveParent = NULL;
	m_mGlobalScalableTransform = m_mLocalScalableTransform = m;
	m_bGlobalScalableTransformsDirty = false;
}

CScalableMatrix ISPEntity::GetGlobalToLocalScalableTransform()
{
	CScalableMatrix mInverse = GetGlobalScalableTransform();
	mInverse.InvertTR();
	return mInverse;
}

CScalableMatrix ISPEntity::GetGlobalToLocalScalableTransform() const
{
	CScalableMatrix mInverse = GetGlobalScalableTransform();
	mInverse.InvertTR();
	return mInverse;
}

CScalableVector ISPEntity::GetGlobalScalableOrigin()
{
	return GetGlobalScalableTransform().GetTranslation();
}

EAngle ISPEntity::GetGlobalScalableAngles()
{
	return GetGlobalScalableTransform().GetAngles();
}

CScalableVector ISPEntity::GetGlobalScalableOrigin() const
{
	return GetGlobalScalableTransform().GetTranslation();
}

EAngle ISPEntity::GetGlobalScalableAngles() const
{
	return GetGlobalScalableTransform().GetAngles();
}

void ISPEntity::SetGlobalScalableOrigin(const CScalableVector& vecOrigin)
{
	if (GetScalableMoveParent() == NULL)
		SetLocalScalableOrigin(vecOrigin);
	else
	{
		CScalableMatrix mGlobalToLocal = GetScalableMoveParent()->GetGlobalToLocalScalableTransform();
		SetLocalScalableOrigin(mGlobalToLocal * vecOrigin);
	}
}

void ISPEntity::SetGlobalScalableAngles(const EAngle& angAngles)
{
	if (GetScalableMoveParent() == NULL)
		SetLocalScalableAngles(angAngles);
	else
	{
		CScalableMatrix mGlobalToLocal = GetScalableMoveParent()->GetGlobalToLocalScalableTransform();
		mGlobalToLocal.SetTranslation(CScalableVector());
		CScalableMatrix mGlobalAngles;
		mGlobalAngles.SetAngles(angAngles);
		CScalableMatrix mLocalAngles = mGlobalToLocal * mGlobalAngles;
		SetLocalScalableAngles(mLocalAngles.GetAngles());
	}
}

CScalableVector ISPEntity::GetGlobalScalableVelocity()
{
	CScalableMatrix mGlobalToLocalRotation = GetGlobalToLocalScalableTransform();
	mGlobalToLocalRotation.SetTranslation(CScalableVector());

	CScalableFloat flLength = m_vecLocalScalableVelocity.Length();
	if (!flLength.IsZero())
		return (mGlobalToLocalRotation * (m_vecLocalScalableVelocity/flLength))*flLength;
	else
		return CScalableVector(CScalableVector());
}

CScalableVector ISPEntity::GetGlobalScalableVelocity() const
{
	CScalableMatrix mGlobalToLocalRotation = GetGlobalToLocalScalableTransform();
	mGlobalToLocalRotation.SetTranslation(CScalableVector(CScalableVector()));

	CScalableFloat flLength = m_vecLocalScalableVelocity.Length();
	if (!flLength.IsZero())
		return (mGlobalToLocalRotation * (m_vecLocalScalableVelocity/flLength))*flLength;
	else
		return CScalableVector();
}

void ISPEntity::SetLocalScalableTransform(const CScalableMatrix& m)
{
	SetLocalScalableOrigin(m.GetTranslation());
	SetLocalScalableAngles(m.GetAngles());

	m_mLocalScalableTransform = m;

	InvalidateGlobalScalableTransforms();
}

void ISPEntity::SetLocalScalableOrigin(const CScalableVector& vecOrigin)
{
//	if (!m_vecLocalScalableOrigin.IsInitialized())
//		m_vecLocalScalableOrigin = vecOrigin;

//	if ((vecOrigin - m_vecLocalScalableOrigin).LengthSqr() == 0)
//		return;

	OnSetLocalScalableOrigin(vecOrigin);
	m_vecLocalScalableOrigin = vecOrigin;

	m_mLocalScalableTransform.SetTranslation(vecOrigin);

	InvalidateGlobalScalableTransforms();
};

CScalableVector ISPEntity::GetLastGlobalScalableOrigin() const
{
	if (GetScalableMoveParent())
		return GetScalableMoveParent()->GetGlobalScalableTransform() * GetLastLocalScalableOrigin();
	else
		return GetLastLocalScalableOrigin();
}

void ISPEntity::SetLocalScalableVelocity(const CScalableVector& vecVelocity)
{
//	if (!m_vecLocalScalableVelocity.IsInitialized())
//		m_vecLocalScalableOrigin = vecVelocity;

//	if ((vecVelocity - m_vecLocalScalableVelocity).LengthSqr() == 0)
//		return;

	m_vecLocalScalableVelocity = vecVelocity;
}

void ISPEntity::SetLocalScalableAngles(const EAngle& angAngles)
{
//	if (!m_angLocalScalableAngles.IsInitialized())
//		m_angLocalScalableAngles = angAngles;

	EAngle angDifference = angAngles - m_angLocalScalableAngles;
	if (fabs(angDifference.p) < 0.001f && fabs(angDifference.y) < 0.001f && fabs(angDifference.r) < 0.001f)
		return;

	m_angLocalScalableAngles = angAngles;

	m_mLocalScalableTransform.SetAngles(angAngles);

	InvalidateGlobalScalableTransforms();
}

CScalableMatrix ISPEntity::GetScalableRenderTransform() const
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

	CScalableMatrix mTransform = GetGlobalScalableTransform();
	mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

	return mTransform;
}

CScalableVector ISPEntity::GetScalableRenderOrigin() const
{
	return GetScalableRenderTransform().GetTranslation();
}

Matrix4x4 ISPEntity::GetRenderTransform() const
{
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();
	return GetScalableRenderTransform().GetUnits(pRenderer->GetRenderingScale());
}

Vector ISPEntity::GetRenderOrigin() const
{
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();
	return GetScalableRenderTransform().GetTranslation().GetUnits(pRenderer->GetRenderingScale());
}

EAngle ISPEntity::GetRenderAngles() const
{
	return GetScalableRenderTransform().GetAngles();
}

bool ISPEntity::IsTouching(ISPEntity* pOther, CScalableVector& vecPoint)
{
	if ((pOther->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	if (GetScalableMoveParent() == pOther)
	{
		bool bResult = pOther->CollideLocal(GetLastLocalScalableOrigin(), GetLocalScalableOrigin(), vecPoint);
		if (bResult)
			vecPoint = GetScalableMoveParent()->GetGlobalScalableTransform() * vecPoint;
		return bResult;
	}
	else
		return pOther->Collide(GetLastGlobalScalableOrigin(), GetGlobalScalableOrigin(), vecPoint);
}

bool ISPEntity::CollideLocal(const CScalableVector& v1, const CScalableVector& v2, CScalableVector& vecPoint)
{
	if (GetRadius().IsZero())
		return false;

	if (v1 == v2)
	{
		vecPoint = v1;
		return (v1.Length() < GetRadius());
	}

	return LineSegmentIntersectsSphere(v1, v2, CScalableVector(), GetRadius(), vecPoint);
}

bool ISPEntity::Collide(const CScalableVector& v1, const CScalableVector& v2, CScalableVector& vecPoint)
{
	if (GetRadius().IsZero())
		return false;

	if (v1 == v2)
		return ((v1-GetGlobalScalableOrigin()).Length() < GetRadius());

	return LineSegmentIntersectsSphere(v1, v2, GetGlobalScalableOrigin(), GetRadius(), vecPoint);
}
