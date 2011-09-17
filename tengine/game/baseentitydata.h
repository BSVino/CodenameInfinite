#ifndef TINKER_BASE_ENTITY_DATA_H
#define TINKER_BASE_ENTITY_DATA_H

#include "traceresult.h"

class CBaseEntityData
{
public:
	void						SetEntity(class CBaseEntity* pEntity);

	virtual void				Think();

	virtual Matrix4x4			GetRenderTransform() const;
	virtual Vector				GetRenderOrigin() const;

	virtual bool				CollideLocal(const class CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);
	virtual bool				Collide(const class CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);

public:
	class CBaseEntity*			m_pEntity;
};

#endif
