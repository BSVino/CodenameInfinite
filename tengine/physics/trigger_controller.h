#pragma once

#include "LinearMath/btVector3.h"

#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

#include <game/entityhandle.h>

class CBaseEntity;
class btPairCachingGhostObject;

class CTriggerController : public btActionInterface
{
public:
	CTriggerController(CBaseEntity* pEntity, btPairCachingGhostObject* ghostObject);

public:
	// btActionInterface
	virtual void	updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime);
	void			debugDraw(btIDebugDraw* debugDrawer) {};

	btPairCachingGhostObject* GetGhostObject();

	CBaseEntity*	GetEntity() const;

protected:
	btManifoldArray				m_aManifolds;

	CEntityHandle<CBaseEntity>	m_hEntity;

	btPairCachingGhostObject*	m_pGhostObject;
};
