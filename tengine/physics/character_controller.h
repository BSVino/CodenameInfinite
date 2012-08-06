#ifndef TINKER_CHARACTER_CONTROLLER_H
#define TINKER_CHARACTER_CONTROLLER_H

#include "LinearMath/btVector3.h"

#include "BulletDynamics/Character/btCharacterControllerInterface.h"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

#include <game/entityhandle.h>

class CBaseEntity;
class CCharacter;
class btCollisionShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;
class btConvexShape;

// This class was originally forked from the Bullet library's btKinematicCharacterController class.

// btKinematicCharacterController is an object that supports a sliding motion in a world.
// It uses a ghost object and convex sweep test to test for upcoming collisions. This is combined with discrete collision detection to recover from penetrations.
// Interaction between btKinematicCharacterController and dynamic rigid bodies needs to be explicity implemented by the user.
class CCharacterController : public btCharacterControllerInterface
{
public:
	CCharacterController (CCharacter* pEntity, btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight);
	~CCharacterController ();

public:
	// btActionInterface
	virtual void	updateAction(btCollisionWorld* collisionWorld, btScalar deltaTime);
	void			debugDraw(btIDebugDraw* debugDrawer) {};

	// btCharacterControllerInterface
	virtual void	setWalkDirection(const btVector3& walkDirection);
	virtual void	setVelocityForTimeInterval(const btVector3& velocity, btScalar timeInterval);
	virtual void	reset() {};
	virtual void	warp(const btVector3& origin);
	virtual void	preStep(btCollisionWorld* collisionWorld);
	virtual void	playerStep(btCollisionWorld* collisionWorld, btScalar dt);
	virtual bool	canJump() const;
	virtual void	jump();
	virtual bool	onGround() const;

	virtual void	SetLateralVelocity(const btVector3& velocity);
	void			SetVerticalVelocity(btScalar flVerticalVelocity);

	void			SetFallSpeed(btScalar fallSpeed);
	void			SetJumpSpeed(btScalar jumpSpeed);
	void			SetMaxJumpHeight(btScalar maxJumpHeight);

	void			SetGravity(const btVector3& gravity);
	btVector3		GetGravity() const;

	btVector3		GetVelocity() const;

	void			SetUpVector(const btVector3& v) { m_vecUpVector = v; };
	btVector3		GetUpVector() const { return m_vecUpVector; };

	void			SetLinearFactor(const btVector3& v) { m_vecLinearFactor = v; };
	btVector3		GetLinearFactor() const { return m_vecLinearFactor; };

	/// The max slope determines the maximum angle that the controller can walk up.
	/// The slope angle is measured in radians.
	void			SetMaxSlope(btScalar slopeRadians);
	btScalar		GetMaxSlope() const;

	void			SetColliding(bool bColliding) { m_bColliding = bColliding; }
	bool			IsColliding() { return m_bColliding; }

	btPairCachingGhostObject* getGhostObject();

	class CBaseEntity*	GetEntity() const;

protected:
	btVector3	ComputeReflectionDirection(const btVector3& direction, const btVector3& normal);
	btVector3	ParallelComponent(const btVector3& direction, const btVector3& normal);
	btVector3	PerpendicularComponent(const btVector3& direction, const btVector3& normal);

	bool		RecoverFromPenetration(btCollisionWorld* collisionWorld);
	void		StepUp(btCollisionWorld* collisionWorld);
	void		UpdateTargetPositionBasedOnCollision(const btVector3& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
	void		StepForwardAndStrafe(btCollisionWorld* collisionWorld, const btVector3& walkMove);
	void		StepDown(btCollisionWorld* collisionWorld, btScalar dt);

	void		FindGround(btCollisionWorld* pCollisionWorld);

protected:
	CEntityHandle<CCharacter>	m_hEntity;

	btScalar		m_flHalfHeight;

	btPairCachingGhostObject* m_pGhostObject;
	btConvexShape*	m_pConvexShape;		//is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast

	btScalar		m_flVerticalVelocity;
	btScalar		m_flVerticalOffset;
	btScalar		m_flMaxFallSpeed;
	btScalar		m_flJumpSpeed;
	btScalar		m_flMaxSlopeRadians; // Slope angle that is set (used for returning the exact value)
	btScalar		m_flMaxSlopeCosine;  // Cosine equivalent of m_maxSlopeRadians (calculated once when set, for optimization)
	btVector3		m_vecGravity;

	btVector3		m_vecUpVector;

	btVector3		m_vecLinearFactor;

	btScalar		m_flStepHeight;

	btScalar		m_flAddedMargin;	//@todo: remove this and fix the code

	///this is the desired walk direction, set by the user
	btVector3		m_vecWalkDirection;
	btVector3		m_vecNormalizedDirection;

	//some internal variables
	btVector3		m_vecCurrentPosition;
	btScalar		m_flCurrentStepOffset;
	btVector3		m_vecTargetPosition;

	///keep track of the contact manifolds
	btManifoldArray	m_aManifolds;

	bool			m_bTouchingContact;
	btVector3		m_vecTouchingNormal;

	bool			m_bWasOnGround;
	bool			m_bWasJumping;
	bool			m_bColliding;
};

#endif
