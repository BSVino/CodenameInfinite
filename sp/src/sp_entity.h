#ifndef SP_ENTITY_H
#define SP_ENTITY_H

#include <tengine/game/baseentity.h>

#include "sp_common.h"

class ISPEntity
{
public:
									ISPEntity();

public:
	void							SetScalableMoveParent(ISPEntity* pParent);
	ISPEntity*						GetScalableMoveParent() const;
	bool							HasScalableMoveParent() const { return GetScalableMoveParent() != NULL; };
	void							InvalidateGlobalScalableTransforms();

	const CScalableMatrix&			GetGlobalScalableTransform();
	CScalableMatrix					GetGlobalScalableTransform() const;
	void							SetGlobalScalableTransform(const CScalableMatrix& m);

	CScalableMatrix					GetGlobalToLocalScalableTransform();
	CScalableMatrix					GetGlobalToLocalScalableTransform() const;

	virtual CScalableVector			GetGlobalScalableOrigin();
	virtual EAngle					GetGlobalScalableAngles();

	virtual CScalableVector			GetGlobalScalableOrigin() const;
	virtual EAngle					GetGlobalScalableAngles() const;

	void							SetGlobalScalableOrigin(const CScalableVector& vecOrigin);
	void							SetGlobalScalableAngles(const EAngle& angAngles);

	virtual CScalableVector			GetGlobalScalableVelocity();
	virtual CScalableVector			GetGlobalScalableVelocity() const;

	virtual inline CScalableVector	GetGlobalScalableGravity() const { return m_vecGlobalScalableGravity; };
	void							SetGlobalScalableGravity(CScalableVector vecGravity) { m_vecGlobalScalableGravity = vecGravity; };

	const CScalableMatrix&			GetLocalScalableTransform() const { return m_mLocalScalableTransform; }
	void							SetLocalScalableTransform(const CScalableMatrix& m);

	virtual inline CScalableVector	GetLocalScalableOrigin() const { return m_vecLocalScalableOrigin; };
	void							SetLocalScalableOrigin(const CScalableVector& vecOrigin);
	virtual void					OnSetLocalScalableOrigin(const CScalableVector& vecOrigin) {}

	inline CScalableVector			GetLastLocalScalableOrigin() const { return m_vecLastLocalScalableOrigin; };
	void							SetLastLocalScalableOrigin(const CScalableVector& vecOrigin) { m_vecLastLocalScalableOrigin = vecOrigin; };
	inline CScalableVector			GetLastGlobalScalableOrigin() const;

	inline CScalableVector			GetLocalScalableVelocity() const { return m_vecLocalScalableVelocity; };
	void							SetLocalScalableVelocity(const CScalableVector& vecVelocity);

	inline EAngle					GetLocalScalableAngles() const { return m_angLocalScalableAngles; };
	void							SetLocalScalableAngles(const EAngle& angLocalAngles);

	virtual CScalableFloat			GetRadius() const { return CScalableFloat(); };
	virtual CScalableFloat			GetScalableRenderRadius() const { return GetRadius(); };
	virtual CScalableMatrix			GetScalableRenderTransform() const;
	virtual CScalableVector			GetScalableRenderOrigin() const;
	virtual EAngle					GetScalableRenderAngles() const { return GetGlobalScalableAngles(); };

	virtual Matrix4x4				GetRenderTransform() const;
	virtual Vector					GetRenderOrigin() const;
	virtual EAngle					GetRenderAngles() const;

	virtual bool					ShouldTouch(ISPEntity* pOther) { return true; };
	virtual bool					IsTouchingLocal(ISPEntity* pOther, const CScalableVector& vecDestination, CScalableVector& vecPoint);
	virtual bool					IsTouching(ISPEntity* pOther, const CScalableVector& vecDestination, CScalableVector& vecPoint);
	virtual bool					CollideLocal(const CScalableVector& v1, const CScalableVector& v2, CScalableVector& vecPoint);
	virtual bool					Collide(const CScalableVector& v1, const CScalableVector& v2, CScalableVector& vecPoint);
	virtual void					Touching(ISPEntity* pOther) {};

protected:
	CEntityHandle<CBaseEntity>		m_hScalableMoveParent;
	eastl::vector<CEntityHandle<CBaseEntity>>	m_ahScalableMoveChildren;

	bool							m_bGlobalScalableTransformsDirty;
	CScalableMatrix					m_mGlobalScalableTransform;
	CScalableVector					m_vecGlobalScalableGravity;

	CScalableMatrix					m_mLocalScalableTransform;
	CScalableVector					m_vecLocalScalableOrigin;
	CScalableVector					m_vecLastLocalScalableOrigin;
	EAngle							m_angLocalScalableAngles;
	CScalableVector					m_vecLocalScalableVelocity;
};

class CSPEntity : public CBaseEntity, public ISPEntity
{
	REGISTER_ENTITY_CLASS_INTERFACES(CSPEntity, CBaseEntity, 1);

public:
	// For purposes of rendering and physics, units are assumed to be in this scale.
	virtual scale_t					GetScale() const { return SCALE_METER; }
	virtual bool					ShouldRenderAtScale(scale_t eScale) const { return (eScale == GetScale()); };
};

#endif
