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

class CTraceResult
{
public:
	CTraceResult()
	{
		m_flFraction = 1.0f;
		m_pHit = nullptr;
	}

public:
	class CTraceHit
	{
	public:
		CTraceHit()
		{
			m_flFraction = 1.0f;
			m_pHit = nullptr;
		}

	public:
		float              m_flFraction;
		Vector             m_vecHit;
		class CBaseEntity* m_pHit;
	};

	// All hits.
	tvector<CTraceHit>     m_aHits;

	// Nearest result.
	float                  m_flFraction;
	Vector                 m_vecHit;
	class CBaseEntity*     m_pHit;
};

class CPhysicsModel
{
public:
	virtual					~CPhysicsModel() {}

public:
	virtual void			AddEntity(class CBaseEntity* pEnt, collision_type_t eCollisionType) {};
	virtual void			RemoveEntity(class CBaseEntity* pEnt) {};
	virtual size_t          AddExtra(size_t iExtraMesh, const Vector& vecOrigin) { return ~0; };  // Input is result from LoadExtraCollisionMesh
	virtual size_t          AddExtraCube(const Vector& vecCenter, float flSize) { return ~0; };
	virtual void            RemoveExtra(size_t iExtra) {};               // Input is result from AddExtra*
	virtual void			RemoveAllEntities() {};

	virtual void			LoadCollisionMesh(const tstring& sModel, size_t iTris, int* aiTris, size_t iVerts, float* aflVerts) {};
	virtual void			UnloadCollisionMesh(const tstring& sModel) {};
	virtual size_t          LoadExtraCollisionMesh(size_t iTris, int* aiTris, size_t iVerts, float* aflVerts) { return ~0; };
	virtual void            UnloadExtraCollisionMesh(size_t iMesh) {};

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

	virtual void            CharacterMovement(class CBaseEntity* pEnt, class btCollisionWorld* pCollisionWorld, float flDelta) {};

	virtual void            TraceLine(CTraceResult& tr, const Vector& v1, const Vector& v2, class CBaseEntity* pIgnore=nullptr) {};
	virtual void            CheckSphere(CTraceResult& tr, float flRadius, const Vector& vecCenter, class CBaseEntity* pIgnore=nullptr) {};

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
