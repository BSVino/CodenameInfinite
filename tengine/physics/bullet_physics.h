#ifndef TINKER_BULLET_PHYSICS_H
#define TINKER_BULLET_PHYSICS_H

#include "physics.h"

#include <btBulletDynamicsCommon.h>

#include <game/entityhandle.h>

#include "character_controller.h"
#include "trigger_controller.h"

inline btVector3 ToBTVector(const Vector& v)
{
	return btVector3(v.x, v.y, v.z);
}

inline Vector ToTVector(const btVector3& v)
{
	return Vector(v.x(), v.y(), v.z());
}

class CClosestRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	CClosestRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, btRigidBody* pIgnore=nullptr)
		: btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld)
	{
		m_pIgnore = pIgnore;
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == m_pIgnore)
			return 1.0;

		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}

protected:
	btRigidBody*     m_pIgnore;
};

class CAllContactResultsCallback : public btCollisionWorld::ContactResultCallback
{
public:
	CAllContactResultsCallback(CTraceResult& tr, btCollisionObject* pIgnore=nullptr)
		: m_tr(tr), btCollisionWorld::ContactResultCallback()
	{
		m_pIgnore = pIgnore;
	}

	virtual btScalar addSingleResult(btManifoldPoint& cp,	const btCollisionObject* colObj0,int partId0,int index0,const btCollisionObject* colObj1,int partId1,int index1)
	{
		if (colObj0 == m_pIgnore)
			return 1.0;

		if (colObj1 == m_pIgnore)
			return 1.0;

		CTraceResult::CTraceHit& th = m_tr.m_aHits.push_back();

		th.m_flFraction = cp.getDistance();
		th.m_pHit = CEntityHandle<CBaseEntity>((size_t)colObj1->getUserPointer()).GetPointer();
		th.m_vecHit = ToTVector(cp.getPositionWorldOnB());

		if (th.m_flFraction < m_tr.m_flFraction)
		{
			m_tr.m_flFraction = th.m_flFraction;
			m_tr.m_pHit = th.m_pHit;
			m_tr.m_vecHit = th.m_vecHit;
		}

		return 0.0;
	}

protected:
	CTraceResult&        m_tr;

	btCollisionObject*   m_pIgnore;
};

class CMotionState : public btMotionState
{
public:
	CMotionState()
	{
		m_pPhysics = NULL;
	}

public:
	virtual void getWorldTransform(btTransform& mCenterOfMass) const;

	virtual void setWorldTransform(const btTransform& mCenterOfMass);

public:
	class CBulletPhysics*			m_pPhysics;
	CEntityHandle<CBaseEntity>		m_hEntity;
};

class CPhysicsEntity
{
public:
	CPhysicsEntity()
	{
		m_pRigidBody = NULL;
		m_pExtraShape = NULL;
		m_pGhostObject = NULL;
		m_pCharacterController = NULL;
		m_pTriggerController = NULL;
		m_bCenterMassOffset = true;
		m_eCollisionType = CT_NONE;
	};

	~CPhysicsEntity()
	{
	};

public:
	btRigidBody*						m_pRigidBody;
	btCollisionShape*                   m_pExtraShape;
	tvector<btRigidBody*>				m_apAreaBodies;
	tvector<btRigidBody*>				m_apPhysicsShapes;
	class btPairCachingGhostObject*		m_pGhostObject;
	CCharacterController*				m_pCharacterController;
	CTriggerController*					m_pTriggerController;
	CMotionState						m_oMotionState;
	bool								m_bCenterMassOffset;
	collision_type_t					m_eCollisionType;
};

class CBulletPhysics : public CPhysicsModel
{
public:
							CBulletPhysics();
							~CBulletPhysics();

public:
	virtual void			AddEntity(class CBaseEntity* pEnt, collision_type_t eCollisionType);
	virtual void            AddShape(class CBaseEntity* pEnt, collision_type_t eCollisionType);
	virtual void			AddModel(class CBaseEntity* pEnt, collision_type_t eCollisionType, size_t iModel);
	virtual void			AddModelTris(class CBaseEntity* pEnt, collision_type_t eCollisionType, size_t iModel);
	virtual void			RemoveEntity(class CBaseEntity* pEnt);
	virtual void			RemoveEntity(CPhysicsEntity* pEntity);
	virtual size_t          AddExtra(size_t iExtraMesh, const Vector& vecOrigin);  // Input is result from LoadExtraCollisionMesh
	virtual size_t          AddExtraCube(const Vector& vecCenter, float flSize);
	virtual void            RemoveExtra(size_t iExtra);   // Input is result from AddExtra*
	virtual void			RemoveAllEntities();
	virtual bool            IsEntityAdded(class CBaseEntity* pEnt);

	virtual void			LoadCollisionMesh(const tstring& sModel, size_t iTris, int* aiTris, size_t iVerts, float* aflVerts);
	virtual void			UnloadCollisionMesh(const tstring& sModel);
	virtual size_t          LoadExtraCollisionMesh(size_t iTris, int* aiTris, size_t iVerts, float* aflVerts);
	virtual void			UnloadExtraCollisionMesh(size_t iMesh);

	virtual void			Simulate();

	virtual void			DebugDraw(int iLevel);

	virtual collision_type_t	GetEntityCollisionType(class CBaseEntity* pEnt);

	virtual void			SetEntityTransform(class CBaseEntity* pEnt, const Matrix4x4& mTransform);
	virtual void			SetEntityVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity);
	virtual Vector			GetEntityVelocity(class CBaseEntity* pEnt);
	virtual void			SetControllerMoveVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity);
	virtual void			SetControllerColliding(class CBaseEntity* pEnt, bool bColliding);
	virtual void			SetEntityGravity(class CBaseEntity* pEnt, const Vector& vecGravity);
	virtual void			SetEntityUpVector(class CBaseEntity* pEnt, const Vector& vecUp);
	virtual void			SetLinearFactor(class CBaseEntity* pEnt, const Vector& vecFactor);
	virtual void			SetAngularFactor(class CBaseEntity* pEnt, const Vector& vecFactor);

	virtual void            CharacterMovement(class CBaseEntity* pEnt, class btCollisionWorld* pCollisionWorld, float flDelta);

	virtual void            TraceLine(CTraceResult& tr, const Vector& v1, const Vector& v2, class CBaseEntity* pIgnore=nullptr);
	virtual void            CheckSphere(CTraceResult& tr, float flRadius, const Vector& vecCenter, class CBaseEntity* pIgnore=nullptr);

	virtual void			CharacterJump(class CBaseEntity* pEnt);

	virtual CPhysicsEntity*	GetPhysicsEntity(class CBaseEntity* pEnt);
	virtual CBaseEntity*    GetBaseEntity(class btCollisionObject* pObject);

protected:
	tvector<CPhysicsEntity>					m_aEntityList;
	tvector<CPhysicsEntity*>                m_apExtraEntityList;

	btDefaultCollisionConfiguration*		m_pCollisionConfiguration;
	btCollisionDispatcher*					m_pDispatcher;
	btDbvtBroadphase*						m_pBroadphase;
	class btGhostPairCallback*				m_pGhostPairCallback;
	btDiscreteDynamicsWorld*				m_pDynamicsWorld;

	class CCollisionMesh
	{
	public:
		btTriangleIndexVertexArray*				m_pIndexVertexArray;
		btCollisionShape*						m_pCollisionShape;
	};

	tmap<size_t, CCollisionMesh>		m_apCollisionMeshes;
	tvector<CCollisionMesh*>            m_apExtraCollisionMeshes;
	tmap<tstring, btConvexShape*>		m_apCharacterShapes;

	class CPhysicsDebugDrawer*				m_pDebugDrawer;
};

#endif
