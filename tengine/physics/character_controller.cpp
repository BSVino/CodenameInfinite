#include "character_controller.h"

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"

#include <game/entities/baseentity.h>
#include <game/entities/character.h>
#include <tinker/application.h>

#include "bullet_physics.h"

// Originally forked from Bullet's btKinematicCharacterController

// static helper method
static btVector3 getNormalizedVector(const btVector3& v)
{
	btVector3 n = v.normalized();
	if (n.length() < SIMD_EPSILON) {
		n.setValue(0, 0, 0);
	}
	return n;
}

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	btKinematicClosestNotMeConvexResultCallback (CCharacterController* pController, btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
		: btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)), m_me(me), m_up(up), m_minSlopeDot(minSlopeDot)
	{
		m_pController = pController;
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if (!m_pController->IsColliding())
			return 1;

		if (convexResult.m_hitCollisionObject == m_me)
			return btScalar(1.0);

		if (convexResult.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup & btBroadphaseProxy::SensorTrigger)
			return 1;

		if ((size_t)convexResult.m_hitCollisionObject->getUserPointer() >= GameServer()->GetMaxEntities())
		{
			size_t iIndex = (size_t)convexResult.m_hitCollisionObject->getUserPointer() - GameServer()->GetMaxEntities();
			CBaseEntity* pControllerEntity = m_pController->GetEntity();
			TAssert(pControllerEntity);
			if (pControllerEntity)
			{
				TAssert(normalInWorldSpace);
				if (!pControllerEntity->ShouldCollideWithExtra(iIndex, Vector(convexResult.m_hitPointLocal)))
					return 1;
			}
		}
		else
		{
			CEntityHandle<CBaseEntity> hCollidedEntity((size_t)convexResult.m_hitCollisionObject->getUserPointer());
			TAssert(hCollidedEntity != NULL);
			if (hCollidedEntity.GetPointer())
			{
				CBaseEntity* pControllerEntity = m_pController->GetEntity();
				TAssert(pControllerEntity);
				if (pControllerEntity)
				{
					TAssert(normalInWorldSpace);
					if (!pControllerEntity->ShouldCollideWith(hCollidedEntity, Vector(convexResult.m_hitPointLocal)))
						return 1;
				}
			}
		}

		btVector3 hitNormalWorld;
		if (normalInWorldSpace)
		{
			hitNormalWorld = convexResult.m_hitNormalLocal;
		} else
		{
			///need to transform normal into worldspace
			hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
		}

		btScalar dotUp = m_up.dot(hitNormalWorld);
		if (dotUp < m_minSlopeDot)
			return btScalar(1.0);

		return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
	}

protected:
	CCharacterController*	m_pController;
	btCollisionObject*		m_me;
	const btVector3			m_up;
	btScalar				m_minSlopeDot;
};

CCharacterController::CCharacterController(CCharacter* pEntity, btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight)
{
	m_hEntity = pEntity;
	m_flAddedMargin = 0.02f;
	m_vecWalkDirection.setValue(0,0,0);
	m_pGhostObject = ghostObject;
	m_flStepHeight = stepHeight;
	m_pConvexShape = convexShape;	
	m_flVerticalVelocity = 0.0;
	m_flVerticalOffset = 0.0;
	m_vecGravity = btVector3(0, -9.8f, 0);
	m_vecUpVector = btVector3(0, 1, 0);
	m_vecLinearFactor = btVector3(1, 1, 1);
	m_flMaxFallSpeed = 55.0; // Terminal velocity of a sky diver in m/s.
	m_flJumpSpeed = 6.0;
	m_flCurrentStepOffset = m_flStepHeight;
	m_bWasOnGround = false;
	m_bWasJumping = false;
	m_bColliding = true;
	SetMaxSlope(btRadians(45.0));
}

CCharacterController::~CCharacterController ()
{
}

void CCharacterController::updateAction(btCollisionWorld* pCollisionWorld, btScalar deltaTime)
{
	if (!m_bColliding)
		return;

	TAssert(dynamic_cast<CCharacter*>(GetEntity()));

	static_cast<CCharacter*>(GetEntity())->CharacterMovement(pCollisionWorld, deltaTime);
}

void CCharacterController::CharacterMovement(btCollisionWorld* pCollisionWorld, btScalar deltaTime)
{
	// Grab the new player transform before doing movement steps in case the player has been moved,
	// such as by a platform or teleported. No need to do a physics trace for it, the penetration
	// functions should handle that.
	btTransform mCharacter;
	CPhysicsEntity* pPhysicsEntity = static_cast<CBulletPhysics*>(GamePhysics())->GetPhysicsEntity(m_hEntity);
	pPhysicsEntity->m_oMotionState.getWorldTransform(mCharacter);

	m_pGhostObject->setWorldTransform(mCharacter);

	bool bMovement = PreStep(pCollisionWorld);
	bMovement |= PlayerStep(pCollisionWorld, deltaTime);

	FindGround(pCollisionWorld);

	if (bMovement)
		pPhysicsEntity->m_oMotionState.setWorldTransform(m_pGhostObject->getWorldTransform());
}

void CCharacterController::preStep(btCollisionWorld* pCollisionWorld)
{
	PreStep(pCollisionWorld);
}

bool CCharacterController::PreStep(btCollisionWorld* pCollisionWorld)
{
	m_bTouchingContact = false;
	int i = 0;
	while (RecoverFromPenetration(pCollisionWorld))
	{
		i++;
		m_bTouchingContact = true;

		if (i > 4)
		{
			TMsg("Character controller couldn't recover from penetration.\n");
			break;
		}
	}

	m_vecCurrentPosition = m_pGhostObject->getWorldTransform().getOrigin();
	m_vecTargetPosition = m_vecCurrentPosition;

	return m_bTouchingContact;
}

void CCharacterController::playerStep(btCollisionWorld* pCollisionWorld, btScalar dt)
{
	PlayerStep(pCollisionWorld, dt);
}

bool CCharacterController::PlayerStep(btCollisionWorld* pCollisionWorld, btScalar dt)
{
	//	printf("playerStep(): ");
	//	printf("  dt = %f", dt);

	m_bWasOnGround = onGround();

	// Update fall velocity.
	m_flVerticalVelocity += m_vecGravity.y() * dt;

	if (m_vecUpVector.y() > 0)
	{
		if (m_flVerticalVelocity > 0.0 && m_flVerticalVelocity > m_flJumpSpeed)
			m_flVerticalVelocity = m_flJumpSpeed;

		if (m_flVerticalVelocity < 0.0 && btFabs(m_flVerticalVelocity) > btFabs(m_flMaxFallSpeed))
			m_flVerticalVelocity = -btFabs(m_flMaxFallSpeed);

		m_flVerticalOffset = m_flVerticalVelocity * dt;
	}
	else
	{
		if (m_flVerticalVelocity < 0.0 && m_flVerticalVelocity < -m_flJumpSpeed)
			m_flVerticalVelocity = -m_flJumpSpeed;

		if (m_flVerticalVelocity > 0.0 && btFabs(m_flVerticalVelocity) > btFabs(m_flMaxFallSpeed))
			m_flVerticalVelocity = btFabs(m_flMaxFallSpeed);

		m_flVerticalOffset = -m_flVerticalVelocity * dt;
	}

	btTransform mWorld;
	mWorld = m_pGhostObject->getWorldTransform();

	btVector3 vecOriginalPosition = mWorld.getOrigin();

	bool bMovement = m_vecWalkDirection.length2() > 0.001f || fabs(m_flVerticalOffset) > 0.001f;

	if (bMovement)
	{
		StepUp(pCollisionWorld);
		StepForwardAndStrafe(pCollisionWorld, m_vecWalkDirection * dt);
		StepDown(pCollisionWorld, dt);
	}

	// printf("\n");

	mWorld.setOrigin(mWorld.getOrigin() + (m_vecCurrentPosition - vecOriginalPosition) * m_vecLinearFactor);
	m_pGhostObject->setWorldTransform (mWorld);

	return bMovement;
}

void CCharacterController::setWalkDirection(const btVector3& walkDirection)
{
	SetLateralVelocity(walkDirection);
}

void CCharacterController::setVelocityForTimeInterval(const btVector3& velocity, btScalar timeInterval)
{
	SetLateralVelocity(velocity/timeInterval);
}

void CCharacterController::warp(const btVector3& origin)
{
	btTransform mNew;
	mNew.setIdentity();
	mNew.setOrigin(origin);
	m_pGhostObject->setWorldTransform(mNew);
}

bool CCharacterController::canJump() const
{
	return onGround();
}

void CCharacterController::jump()
{
	if (!canJump())
		return;

	SetJumpSpeed((float)m_hEntity->JumpStrength());

	if (m_vecUpVector.y() > 0)
		m_flVerticalVelocity = m_flJumpSpeed;
	else
		m_flVerticalVelocity = -m_flJumpSpeed;

	m_bWasJumping = true;
}

bool CCharacterController::onGround() const
{
	return m_flVerticalVelocity == 0.0 && m_flVerticalOffset == 0.0;
}

void CCharacterController::SetLateralVelocity(const btVector3& velocity)
{
	m_vecWalkDirection = velocity;
	m_vecNormalizedDirection = getNormalizedVector(velocity);
}

void CCharacterController::SetVerticalVelocity(btScalar flVerticalVelocity)
{
	m_flVerticalVelocity = flVerticalVelocity;
}

void CCharacterController::SetFallSpeed (btScalar flFallSpeed)
{
	m_flMaxFallSpeed = flFallSpeed;
}

void CCharacterController::SetJumpSpeed (btScalar flJumpSpeed)
{
	m_flJumpSpeed = flJumpSpeed;
}

void CCharacterController::SetGravity(const btVector3& vecGravity)
{
	m_vecGravity = vecGravity;
}

btVector3 CCharacterController::GetGravity() const
{
	return m_vecGravity;
}

btVector3 CCharacterController::GetVelocity() const
{
	return btVector3(m_vecWalkDirection.x(), m_flVerticalVelocity, m_vecWalkDirection.z());
}

void CCharacterController::SetMaxSlope(btScalar slopeRadians)
{
	m_flMaxSlopeRadians = slopeRadians;
	m_flMaxSlopeCosine = btCos(slopeRadians);
}

btScalar CCharacterController::GetMaxSlope() const
{
	return m_flMaxSlopeRadians;
}

btPairCachingGhostObject* CCharacterController::getGhostObject()
{
	return m_pGhostObject;
}

CBaseEntity* CCharacterController::GetEntity() const
{
	return m_hEntity;
}

// Returns the reflection direction of a ray going 'direction' hitting a surface with normal 'normal'
// from: http://www-cs-students.stanford.edu/~adityagp/final/node3.html
btVector3 CCharacterController::ComputeReflectionDirection(const btVector3& direction, const btVector3& normal)
{
	return direction - (btScalar(2.0) * direction.dot(normal)) * normal;
}

// Returns the portion of 'direction' that is parallel to 'normal'
btVector3 CCharacterController::ParallelComponent(const btVector3& direction, const btVector3& normal)
{
	btScalar magnitude = direction.dot(normal);
	return normal * magnitude;
}

// Returns the portion of 'direction' that is perpindicular to 'normal'
btVector3 CCharacterController::PerpendicularComponent(const btVector3& direction, const btVector3& normal)
{
	return direction - ParallelComponent(direction, normal);
}

bool CCharacterController::RecoverFromPenetration(btCollisionWorld* pCollisionWorld)
{
	if (!IsColliding())
		return false;

	bool bPenetration = false;

	pCollisionWorld->getDispatcher()->dispatchAllCollisionPairs(m_pGhostObject->getOverlappingPairCache(), pCollisionWorld->getDispatchInfo(), pCollisionWorld->getDispatcher());

	m_vecCurrentPosition = m_pGhostObject->getWorldTransform().getOrigin();
	btVector3 vecOriginalPosition = m_vecCurrentPosition;

	btScalar maxPen = btScalar(0.0);
	for (int i = 0; i < m_pGhostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
	{
		m_aManifolds.resize(0);

		btBroadphasePair* pCollisionPair = &m_pGhostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

		if (pCollisionPair->m_algorithm)
			pCollisionPair->m_algorithm->getAllContactManifolds(m_aManifolds);

		for (int j = 0; j < m_aManifolds.size(); j++)
		{
			btPersistentManifold* pManifold = m_aManifolds[j];

			btCollisionObject* obA = static_cast<btCollisionObject*>(pManifold->getBody0());
			btCollisionObject* obB = static_cast<btCollisionObject*>(pManifold->getBody1());

			btScalar directionSign;
			CEntityHandle<CBaseEntity> hOther;
			size_t iExtra;
			if (obA == m_pGhostObject)
			{
				if (obB->getBroadphaseHandle()->m_collisionFilterGroup & btBroadphaseProxy::SensorTrigger)
					continue;

				directionSign = btScalar(-1.0);
				hOther = CEntityHandle<CBaseEntity>((size_t)obB->getUserPointer());
				iExtra = (size_t)obB->getUserPointer()-GameServer()->GetMaxEntities();

				if (obB->getCollisionFlags()&btCollisionObject::CF_CHARACTER_OBJECT)
				{
					// If I'm heavier than he, don't let him push me around
					if (hOther->GetMass() < m_hEntity->GetMass())
						continue;
				}
			}
			else
			{
				if (obA->getBroadphaseHandle()->m_collisionFilterGroup & btBroadphaseProxy::SensorTrigger)
					continue;

				directionSign = btScalar(1.0);
				hOther = CEntityHandle<CBaseEntity>((size_t)obA->getUserPointer());
				iExtra = (size_t)obB->getUserPointer()-GameServer()->GetMaxEntities();

				if (obA->getCollisionFlags()&btCollisionObject::CF_CHARACTER_OBJECT)
				{
					// If I'm heavier than he, don't let him push me around
					if (hOther->GetMass() < m_hEntity->GetMass())
						continue;
				}
			}

			for (int p = 0; p < pManifold->getNumContacts(); p++)
			{
				const btManifoldPoint& pt = pManifold->getContactPoint(p);

				if (obA == m_pGhostObject)
				{
					if (hOther)
					{
						if (!m_hEntity->ShouldCollideWith(hOther, Vector(pt.getPositionWorldOnB())))
							continue;
					}
					else
					{
						if (!m_hEntity->ShouldCollideWithExtra(iExtra, Vector(pt.getPositionWorldOnB())))
							continue;
					}
				}
				else
				{
					if (hOther)
					{
						if (!m_hEntity->ShouldCollideWith(hOther, Vector(pt.getPositionWorldOnA())))
							continue;
					}
					else
					{
						if (!m_hEntity->ShouldCollideWithExtra(iExtra, Vector(pt.getPositionWorldOnA())))
							continue;
					}
				}

				btScalar dist = pt.getDistance();

				if (dist < -0.0001f)
				{
					if (dist < maxPen)
					{
						maxPen = dist;
						m_vecTouchingNormal = pt.m_normalWorldOnB * directionSign;
					}

					btScalar flDot = pt.m_normalWorldOnB.dot(GetUpVector());
					if (flDot > 0.9999f)
						m_vecCurrentPosition += GetUpVector() * (directionSign * dist * 1.001f);
					else if (flDot > 0.707)
					{
						TAssert(obA == m_pGhostObject);

						// Find the point at which it should intersect if we move it straight up.
						Vector vecNewTouch;
						RayIntersectsPlane(Ray(Vector(pt.m_positionWorldOnA), Vector(GetUpVector())), Vector(pt.m_positionWorldOnB), Vector(pt.m_normalWorldOnB), &vecNewTouch);
						btVector3 vecNewTouch2(vecNewTouch.x, vecNewTouch.y, vecNewTouch.z);

						m_vecCurrentPosition += btVector3(0, ((vecNewTouch2.y() - pt.m_positionWorldOnA.y()) * 1.001f), 0);
					}
					//else if (flDot < 0.01f)
					//	m_vecCurrentPosition += (m_vecCurrentPosition - pt.m_positionWorldOnB).normalized() * (directionSign * dist * 1.001f);
					else
						m_vecCurrentPosition += pt.m_normalWorldOnB * (directionSign * dist * 1.001f);

					bPenetration = true;
				} else {
					//printf("touching %f\n", dist);
				}
			}

			//pManifold->clearManifold();
		}
	}

	btTransform mNew = m_pGhostObject->getWorldTransform();
	mNew.setOrigin(mNew.getOrigin() + (m_vecCurrentPosition - vecOriginalPosition) * m_vecLinearFactor);
	m_pGhostObject->setWorldTransform(mNew);

	//printf("m_vecTouchingNormal = %f,%f,%f\n", m_vecTouchingNormal[0], m_vecTouchingNormal[1], m_vecTouchingNormal[2]);

	return bPenetration;
}

void CCharacterController::StepUp(btCollisionWorld* pWorld)
{
	// phase 1: up
	btTransform start, end;
	m_vecTargetPosition = m_vecCurrentPosition + GetUpVector() * (m_flStepHeight + (m_flVerticalOffset > 0.f?m_flVerticalOffset:0.f));

	start.setIdentity ();
	end.setIdentity ();

	/* FIXME: Handle penetration properly */
	start.setOrigin (m_vecCurrentPosition + GetUpVector() * (m_pConvexShape->getMargin() + m_flAddedMargin));
	end.setOrigin (m_vecTargetPosition);

	btKinematicClosestNotMeConvexResultCallback callback(this, m_pGhostObject, -GetUpVector(), btScalar(0.7071));
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

	m_pGhostObject->convexSweepTest (m_pConvexShape, start, end, callback, pWorld->getDispatchInfo().m_allowedCcdPenetration);

	if (callback.hasHit())
	{
		// Only modify the position if the hit was a slope and not a wall or ceiling.
		if(callback.m_hitNormalWorld.dot(GetUpVector()) > 0.0)
		{
			// we moved up only a fraction of the step height
			m_flCurrentStepOffset = m_flStepHeight * callback.m_closestHitFraction;
			m_vecCurrentPosition.setInterpolate3(m_vecCurrentPosition, m_vecTargetPosition, callback.m_closestHitFraction);
		}
		m_flVerticalVelocity = 0.0;
		m_flVerticalOffset = 0.0;
	} else {
		m_flCurrentStepOffset = m_flStepHeight;
		m_vecCurrentPosition = m_vecTargetPosition;
	}
}

void CCharacterController::UpdateTargetPositionBasedOnCollision(const btVector3& vecHitNormal, btScalar flTangentMag, btScalar flNormalMag)
{
	btVector3 vecMovementDirection = m_vecTargetPosition - m_vecCurrentPosition;
	btScalar flMovementLength = vecMovementDirection.length();

	if (flMovementLength > SIMD_EPSILON)
	{
		vecMovementDirection.normalize();

		btVector3 vecReflectDir = ComputeReflectionDirection(vecMovementDirection, vecHitNormal);
		vecReflectDir.normalize();

		btVector3 vecParallelDir, vecPerpendicularDir;

		vecParallelDir = ParallelComponent(vecReflectDir, vecHitNormal);
		vecPerpendicularDir = PerpendicularComponent(vecReflectDir, vecHitNormal);

		m_vecTargetPosition = m_vecCurrentPosition;

		if (flNormalMag != 0.0)
		{
			btVector3 vecPerpComponent = vecPerpendicularDir * (flNormalMag*flMovementLength);
			//printf("vecPerpComponent = %f %f %f\n", vecPerpComponent[0], vecPerpComponent[1], vecPerpComponent[2]);
			m_vecTargetPosition += vecPerpComponent;
		}
	}
	else
	{
		//printf("flMovementLength don't normalize a zero vector\n");
	}
}

void CCharacterController::StepForwardAndStrafe(btCollisionWorld* pCollisionWorld, const btVector3& vecWalkMove)
{
	//printf("m_vecNormalizedDirection=%f,%f,%f\n", m_vecNormalizedDirection[0],m_vecNormalizedDirection[1],m_vecNormalizedDirection[2]);

	// phase 2: forward and strafe
	btTransform mStart, mEnd;
	m_vecTargetPosition = m_vecCurrentPosition + vecWalkMove;

	mStart.setIdentity ();
	mEnd.setIdentity ();

	btScalar flFraction = 1.0;
	btScalar flDistance2 = (m_vecCurrentPosition-m_vecTargetPosition).length2();

	if (m_bTouchingContact)
	{
		if (m_vecNormalizedDirection.dot(m_vecTouchingNormal) > btScalar(0.0))
			UpdateTargetPositionBasedOnCollision(m_vecTouchingNormal);
	}

	int iMaxIter = 10;

	while (flFraction > btScalar(0.01) && iMaxIter-- > 0)
	{
		mStart.setOrigin(m_vecCurrentPosition);
		mEnd.setOrigin(m_vecTargetPosition);
		btVector3 sweepDirNegative(m_vecCurrentPosition - m_vecTargetPosition);

		btKinematicClosestNotMeConvexResultCallback callback(this, m_pGhostObject, sweepDirNegative, btScalar(0.0));
		callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
		callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

		btScalar margin = m_pConvexShape->getMargin();
		m_pConvexShape->setMargin(margin + m_flAddedMargin);

		m_pGhostObject->convexSweepTest (m_pConvexShape, mStart, mEnd, callback, pCollisionWorld->getDispatchInfo().m_allowedCcdPenetration);

		m_pConvexShape->setMargin(margin);

		flFraction -= callback.m_closestHitFraction;

		if (callback.hasHit())
		{	
			// we moved only a fraction
			btScalar hitDistance;
			hitDistance = (callback.m_hitPointWorld - m_vecCurrentPosition).length();

			UpdateTargetPositionBasedOnCollision (callback.m_hitNormalWorld);
			btVector3 currentDir = m_vecTargetPosition - m_vecCurrentPosition;
			flDistance2 = currentDir.length2();
			if (flDistance2 > SIMD_EPSILON)
			{
				currentDir.normalize();
				/* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
				if (currentDir.dot(m_vecNormalizedDirection) <= btScalar(0.0))
					break;
			} else
			{
				//printf("currentDir: don't normalize a zero vector\n");
				break;
			}

		} else {
			// we moved whole way
			m_vecCurrentPosition = m_vecTargetPosition;
		}
	}
}

void CCharacterController::StepDown(btCollisionWorld* pCollisionWorld, btScalar dt)
{
	btTransform mStart, mEnd;

	// phase 3: down
	btScalar flDownVelocity;
	if (m_vecUpVector.y() < 0)
		flDownVelocity = (m_flVerticalVelocity>0.f?m_flVerticalVelocity:0.f) * dt;
	else
		flDownVelocity = (m_flVerticalVelocity<0.f?-m_flVerticalVelocity:0.f) * dt;

	if(flDownVelocity > 0.0 && flDownVelocity < m_flStepHeight
		&& (m_bWasOnGround || !m_bWasJumping))
	{
		flDownVelocity = m_flStepHeight;
	}

	btVector3 vecStepDrop = GetUpVector() * (m_flCurrentStepOffset + flDownVelocity);
	m_vecTargetPosition -= vecStepDrop;

	mStart.setIdentity();
	mEnd.setIdentity();

	mStart.setOrigin(m_vecCurrentPosition);
	mEnd.setOrigin(m_vecTargetPosition);

	btKinematicClosestNotMeConvexResultCallback callback(this, m_pGhostObject, GetUpVector(), m_flMaxSlopeCosine);
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

	m_pGhostObject->convexSweepTest (m_pConvexShape, mStart, mEnd, callback, pCollisionWorld->getDispatchInfo().m_allowedCcdPenetration);

	if (callback.hasHit())
	{
		// we dropped a fraction of the height -> hit floor
		m_vecCurrentPosition.setInterpolate3(m_vecCurrentPosition, m_vecTargetPosition, callback.m_closestHitFraction);
		m_flVerticalVelocity = 0.0;
		m_flVerticalOffset = 0.0;
		m_bWasJumping = false;
	} else {
		// we dropped the full height
		m_vecCurrentPosition = m_vecTargetPosition;
	}
}

void CCharacterController::FindGround(btCollisionWorld* pCollisionWorld)
{
	if (GetVelocity().dot(GetUpVector()) > m_flJumpSpeed/2.0f)
	{
		m_hEntity->SetGroundEntity(nullptr);
		return;
	}

	btTransform mStart, mEnd;

	btVector3 vecStepDrop = GetUpVector() * m_flCurrentStepOffset;
	btVector3 vecGroundPosition = m_pGhostObject->getWorldTransform().getOrigin() - vecStepDrop;

	mStart.setIdentity();
	mEnd.setIdentity();

	mStart.setOrigin(m_pGhostObject->getWorldTransform().getOrigin());
	mEnd.setOrigin(m_vecTargetPosition);

	btKinematicClosestNotMeConvexResultCallback callback(this, m_pGhostObject, GetUpVector(), m_flMaxSlopeCosine);
	callback.m_collisionFilterGroup = getGhostObject()->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = getGhostObject()->getBroadphaseHandle()->m_collisionFilterMask;

	m_pGhostObject->convexSweepTest (m_pConvexShape, mStart, mEnd, callback, pCollisionWorld->getDispatchInfo().m_allowedCcdPenetration);

	if (callback.hasHit())
	{
		btScalar flDot = GetUpVector().dot(callback.m_hitNormalWorld);
		if (flDot < m_flMaxSlopeCosine)
			m_hEntity->SetGroundEntity(nullptr);
		else
		{
			CEntityHandle<CBaseEntity> hOther = CEntityHandle<CBaseEntity>((size_t)callback.m_hitCollisionObject->getUserPointer());
			if (hOther)
				m_hEntity->SetGroundEntity(hOther);
			else
				m_hEntity->SetGroundEntityExtra((size_t)callback.m_hitCollisionObject->getUserPointer()-GameServer()->GetMaxEntities());
		}
	}
	else
		m_hEntity->SetGroundEntity(nullptr);
}
