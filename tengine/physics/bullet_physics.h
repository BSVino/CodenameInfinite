#ifndef TINKER_BULLET_PHYSICS_H
#define TINKER_BULLET_PHYSICS_H

#include "physics.h"

#include <btBulletDynamicsCommon.h>

#include <game/entityhandle.h>

#include "character_controller.h"
#include "trigger_controller.h"

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
	virtual void			AddModel(class CBaseEntity* pEnt, collision_type_t eCollisionType, size_t iModel);
	virtual void			AddModelTris(class CBaseEntity* pEnt, collision_type_t eCollisionType, size_t iModel);
	virtual void			RemoveEntity(class CBaseEntity* pEnt);
	virtual void			RemoveEntity(CPhysicsEntity* pEntity);
	virtual void			RemoveAllEntities();

	virtual void			LoadCollisionMesh(const tstring& sModel, size_t iTris, int* aiTris, size_t iVerts, float* aflVerts);
	virtual void			UnloadCollisionMesh(const tstring& sModel);

	virtual void			Simulate();

	virtual void			DebugDraw(int iLevel);

	virtual collision_type_t	GetEntityCollisionType(class CBaseEntity* pEnt);

	virtual void			SetEntityTransform(class CBaseEntity* pEnt, const Matrix4x4& mTransform);
	virtual void			SetEntityVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity);
	virtual Vector			GetEntityVelocity(class CBaseEntity* pEnt);
	virtual void			SetControllerWalkVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity);
	virtual void			SetControllerColliding(class CBaseEntity* pEnt, bool bColliding);
	virtual void			SetEntityGravity(class CBaseEntity* pEnt, const Vector& vecGravity);
	virtual void			SetEntityUpVector(class CBaseEntity* pEnt, const Vector& vecUp);
	virtual void			SetLinearFactor(class CBaseEntity* pEnt, const Vector& vecFactor);
	virtual void			SetAngularFactor(class CBaseEntity* pEnt, const Vector& vecFactor);

	virtual void			CharacterJump(class CBaseEntity* pEnt);

	virtual CPhysicsEntity*	GetPhysicsEntity(class CBaseEntity* pEnt);

protected:
	tvector<CPhysicsEntity>					m_aEntityList;

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
	tmap<tstring, btConvexShape*>		m_apCharacterShapes;

	class CPhysicsDebugDrawer*				m_pDebugDrawer;
};

#endif
