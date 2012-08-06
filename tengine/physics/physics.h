#ifndef TINKER_PHYSICS_H
#define TINKER_PHYSICS_H

#include <matrix.h>

#include <models/models.h>

typedef enum collision_type_e
{
	CT_NONE = 0,
	CT_STATIC_MESH,	// Not animated, never moves. World geometry.
	CT_KINEMATIC,	// Not simulated by the engine, but collides with dynamic objects. Animated externally by game code.
	CT_CHARACTER,	// Kinematically animated character controller.
	CT_TRIGGER,		// Does not collide, but reports intersections.
} collision_type_t;

class CPhysicsModel
{
public:
	virtual					~CPhysicsModel() {}

public:
	virtual void			AddEntity(class CBaseEntity* pEnt, collision_type_t eCollisionType) {};
	virtual void			RemoveEntity(class CBaseEntity* pEnt) {};
	virtual void			RemoveAllEntities() {};

	virtual void			LoadCollisionMesh(const tstring& sModel, size_t iTris, int* aiTris, size_t iVerts, float* aflVerts) {};
	virtual void			UnloadCollisionMesh(const tstring& sModel) {};

	virtual void			Simulate() {};

	virtual void			DebugDraw(int iLevel) {};

	virtual collision_type_t	GetEntityCollisionType(class CBaseEntity* pEnt) { return CT_NONE; };

	virtual void			SetEntityTransform(class CBaseEntity* pEnt, const Matrix4x4& mTransform) {};
	virtual void			SetEntityVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity) {};
	virtual Vector			GetEntityVelocity(class CBaseEntity* pEnt) { return Vector(0, 0, 0); };
	virtual void			SetControllerWalkVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity) {};
	virtual void			SetControllerColliding(class CBaseEntity* pEnt, bool bColliding) {};
	virtual void			SetEntityGravity(class CBaseEntity* pEnt, const Vector& vecGravity) {};
	virtual void			SetEntityUpVector(class CBaseEntity* pEnt, const Vector& vecUp) {};
	virtual void			SetLinearFactor(class CBaseEntity* pEnt, const Vector& vecFactor) {};
	virtual void			SetAngularFactor(class CBaseEntity* pEnt, const Vector& vecFactor) {};

	virtual void			CharacterJump(class CBaseEntity* pEnt) {};
};

class CPhysicsManager
{
public:
							CPhysicsManager();
							~CPhysicsManager();

public:
	CPhysicsModel*			GetModel() { return m_pModel; }

protected:
	CPhysicsModel*			m_pModel;
};

CPhysicsModel* GamePhysics();

#endif
