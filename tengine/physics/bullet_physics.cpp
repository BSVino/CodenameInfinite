#include "bullet_physics.h"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <game/gameserver.h>
#include <game/entities/character.h>
#include <models/models.h>
#include <toys/toy.h>
#include <tinker/profiler.h>
#include <tinker/application.h>
#include <tinker/cvar.h>

#include "physics_debugdraw.h"

CBulletPhysics::CBulletPhysics()
{
	// Allocate all memory up front to avoid reallocations
	m_aEntityList.set_capacity(GameServer()->GetMaxEntities());

	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
	m_pDispatcher = new	btCollisionDispatcher(m_pCollisionConfiguration);
	m_pBroadphase = new btDbvtBroadphase();
	m_pBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(m_pGhostPairCallback = new btGhostPairCallback());

	m_pDynamicsWorld = new btDiscreteDynamicsWorld(m_pDispatcher, m_pBroadphase, NULL, m_pCollisionConfiguration);
	m_pDynamicsWorld->setGravity(btVector3(0, -9.8f, 0));
	m_pDynamicsWorld->setForceUpdateAllAabbs(false);

	m_pDebugDrawer = NULL;
}

CBulletPhysics::~CBulletPhysics()
{
	delete m_pDynamicsWorld;
	for (tmap<size_t, CCollisionMesh>::iterator it = m_apCollisionMeshes.begin(); it != m_apCollisionMeshes.end(); it++)
	{
		delete it->second.m_pCollisionShape;
		delete it->second.m_pIndexVertexArray;
	}
	delete m_pBroadphase;
	delete m_pGhostPairCallback;
	delete m_pDispatcher;
	delete m_pCollisionConfiguration;

	if (m_pDebugDrawer)
		delete m_pDebugDrawer;
}

void CBulletPhysics::AddEntity(CBaseEntity* pEntity, collision_type_t eCollisionType)
{
	TAssert(eCollisionType != CT_NONE);
	if (eCollisionType == CT_NONE)
		return;

	TAssert(pEntity);
	if (!pEntity)
		return;

	size_t iHandle = pEntity->GetHandle();
	if (m_aEntityList.size() <= iHandle)
		m_aEntityList.resize(iHandle+1);

	CPhysicsEntity* pPhysicsEntity = &m_aEntityList[iHandle];
	pPhysicsEntity->m_eCollisionType = eCollisionType;
	pPhysicsEntity->m_oMotionState.m_hEntity = pEntity;
	pPhysicsEntity->m_oMotionState.m_pPhysics = this;

	if (eCollisionType == CT_CHARACTER)
	{
		pPhysicsEntity->m_bCenterMassOffset = true;

		AABB r = pEntity->GetPhysBoundingBox();
		float flRadiusX = r.m_vecMaxs.x - r.Center().x;
		float flRadiusZ = r.m_vecMaxs.z - r.Center().z;
		float flRadius = flRadiusX;
		if (flRadiusZ > flRadiusX)
			flRadius = flRadiusZ;
		float flHeight = r.m_vecMaxs.y - r.m_vecMins.y;

		CCharacter* pCharacter = dynamic_cast<CCharacter*>(pEntity);

		tstring sIdentifier;
		if (pEntity->GetModel())
			sIdentifier = pEntity->GetModel()->m_sFilename;
		else
			sIdentifier = pEntity->GetClassName();

		auto it = m_apCharacterShapes.find(sIdentifier);
		if (it == m_apCharacterShapes.end())
		{
			if (pCharacter->UsePhysicsModelForController())
			{
				TAssert(pEntity->GetModelID() != ~0);

				Vector vecHalf = pEntity->GetModel()->m_aabbPhysBoundingBox.m_vecMaxs - pEntity->GetModel()->m_aabbPhysBoundingBox.Center();
				m_apCharacterShapes[sIdentifier] = new btBoxShape(ToBTVector(vecHalf));
			}
			else
			{
				TAssert(flHeight > flRadius*2); // Couldn't very well make a capsule this way could we?

				m_apCharacterShapes[sIdentifier] = new btCapsuleShape(flRadius, flHeight - flRadius*2);
			}
		}

#ifdef _DEBUG
		btCapsuleShape* pCapsuleShape = dynamic_cast<btCapsuleShape*>(m_apCharacterShapes[sIdentifier]);
		if (pCapsuleShape)
		{
			// Varying character sizes not yet supported!
			TAssert(fabs(pCapsuleShape->getRadius() - flRadius) < 0.00001f);
			TAssert(fabs(pCapsuleShape->getHalfHeight()*2 - flRadius) - flHeight < 0.001f);
		}
#endif

		btConvexShape* pConvexShape = m_apCharacterShapes[sIdentifier];

		btTransform mTransform;
		mTransform.setIdentity();
		Matrix4x4 mGlobalTransform = pEntity->GetPhysicsTransform();
		mTransform.setFromOpenGLMatrix(&mGlobalTransform.m[0][0]);

		pPhysicsEntity->m_pGhostObject = new btPairCachingGhostObject();
		pPhysicsEntity->m_pGhostObject->setWorldTransform(mTransform);
		pPhysicsEntity->m_pGhostObject->setCollisionShape(pConvexShape);
		pPhysicsEntity->m_pGhostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
		pPhysicsEntity->m_pGhostObject->setUserPointer((void*)iHandle);

		float flStepHeight = 0.2f;
		if (pCharacter)
			flStepHeight = (float)pCharacter->GetMaxStepHeight();

		pPhysicsEntity->m_pCharacterController = new CCharacterController(pCharacter, pPhysicsEntity->m_pGhostObject, pConvexShape, flStepHeight);

		m_pDynamicsWorld->addCollisionObject(pPhysicsEntity->m_pGhostObject, pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
		m_pDynamicsWorld->addAction(pPhysicsEntity->m_pCharacterController);
	}
	else if (eCollisionType == CT_TRIGGER)
	{
		btCollisionShape* pCollisionShape;

		pPhysicsEntity->m_bCenterMassOffset = true;

		AABB aabbBoundingBox;

		if (pEntity->GetModelID() != ~0)
			aabbBoundingBox = pEntity->GetModel()->m_aabbPhysBoundingBox;
		else
			aabbBoundingBox = pEntity->GetVisBoundingBox();

		aabbBoundingBox.m_vecMins += pEntity->GetGlobalOrigin();
		aabbBoundingBox.m_vecMaxs += pEntity->GetGlobalOrigin();

		Vector vecHalf = (aabbBoundingBox.m_vecMaxs - aabbBoundingBox.Center()) * pEntity->GetScale();
		pCollisionShape = new btBoxShape(ToBTVector(vecHalf));

		TAssert(pCollisionShape);

		btTransform mTransform;
		mTransform.setIdentity();

		if (pEntity->GetModelID() != ~0)
			mTransform.setFromOpenGLMatrix(&pEntity->GetPhysicsTransform().m[0][0]);
		else
			mTransform.setOrigin(ToBTVector(aabbBoundingBox.Center()));

		btVector3 vecLocalInertia(0, 0, 0);

		pPhysicsEntity->m_pGhostObject = new btPairCachingGhostObject();
		pPhysicsEntity->m_pGhostObject->setWorldTransform(mTransform);
		pPhysicsEntity->m_pGhostObject->setCollisionShape(pCollisionShape);
		pPhysicsEntity->m_pGhostObject->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
		pPhysicsEntity->m_pGhostObject->setUserPointer((void*)iHandle);

		pPhysicsEntity->m_pGhostObject->setCollisionFlags(pPhysicsEntity->m_pGhostObject->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		pPhysicsEntity->m_pGhostObject->setActivationState(DISABLE_DEACTIVATION);

		pPhysicsEntity->m_pTriggerController = new CTriggerController(pEntity, pPhysicsEntity->m_pGhostObject);

		m_pDynamicsWorld->addCollisionObject(pPhysicsEntity->m_pGhostObject, pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
		m_pDynamicsWorld->addAction(pPhysicsEntity->m_pTriggerController);
	}
	else
	{
		if (eCollisionType == CT_STATIC_MESH)
		{
			if (pEntity->GetModelID() != ~0)
				AddModel(pEntity, eCollisionType, pEntity->GetModelID());
			else
				AddShape(pEntity, eCollisionType);
		}
		else if (eCollisionType == CT_KINEMATIC)
		{
			AddModel(pEntity, eCollisionType, pEntity->GetModelID());
		}
		else
		{
			TAssert(!"Unimplemented collision type");
		}
	}
}

void CBulletPhysics::AddShape(class CBaseEntity* pEntity, collision_type_t eCollisionType)
{
	CPhysicsEntity* pPhysicsEntity = &m_aEntityList[pEntity->GetHandle()];

	btCollisionShape* pCollisionShape;

	pPhysicsEntity->m_bCenterMassOffset = true;

	Vector vecHalf = pEntity->GetPhysBoundingBox().m_vecMaxs-pEntity->GetPhysBoundingBox().Center();
	pCollisionShape = new btBoxShape(ToBTVector(vecHalf));

	btTransform mTransform;
	mTransform.setIdentity();
	mTransform.setFromOpenGLMatrix((Matrix4x4)pEntity->GetPhysicsTransform());

	btVector3 vecLocalInertia(0, 0, 0);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(0, &pPhysicsEntity->m_oMotionState, pCollisionShape, vecLocalInertia);

	pPhysicsEntity->m_apPhysicsShapes.push_back(new btRigidBody(rbInfo));
	pPhysicsEntity->m_apPhysicsShapes.back()->setUserPointer((void*)pEntity->GetHandle());
	pPhysicsEntity->m_apPhysicsShapes.back()->setWorldTransform(mTransform);

	if (eCollisionType == CT_KINEMATIC)
	{
		pPhysicsEntity->m_apPhysicsShapes.back()->setCollisionFlags((pPhysicsEntity->m_pRigidBody?pPhysicsEntity->m_pRigidBody->getCollisionFlags():0) | btCollisionObject::CF_KINEMATIC_OBJECT);
		pPhysicsEntity->m_apPhysicsShapes.back()->setActivationState(DISABLE_DEACTIVATION);
	}
	else if (eCollisionType == CT_STATIC_MESH)
		pPhysicsEntity->m_apPhysicsShapes.back()->setActivationState(DISABLE_SIMULATION);

	m_pDynamicsWorld->addRigidBody(pPhysicsEntity->m_apPhysicsShapes.back(), pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
}

void CBulletPhysics::AddModel(class CBaseEntity* pEntity, collision_type_t eCollisionType, size_t iModel)
{
	CModel* pModel = CModelLibrary::GetModel(iModel);
	if (!pModel)
		return;

	for (size_t i = 0; i < pModel->m_pToy->GetNumSceneAreas(); i++)
	{
		size_t iArea = CModelLibrary::FindModel(pModel->m_pToy->GetSceneAreaFileName(i));
		AddModel(pEntity, eCollisionType, iArea);
	}

	AddModelTris(pEntity, eCollisionType, iModel);

	CPhysicsEntity* pPhysicsEntity = &m_aEntityList[pEntity->GetHandle()];

	for (size_t i = 0; i < pModel->m_pToy->GetPhysicsNumBoxes(); i++)
	{
		btCollisionShape* pCollisionShape;

		pPhysicsEntity->m_bCenterMassOffset = true;

		Vector vecHalf = pModel->m_pToy->GetPhysicsBoxHalfSize(i);
		pCollisionShape = new btBoxShape(ToBTVector(vecHalf));

		btTransform mTransform;
		mTransform.setIdentity();
		mTransform.setFromOpenGLMatrix((Matrix4x4)pEntity->GetPhysicsTransform()*pModel->m_pToy->GetPhysicsBox(i).GetMatrix4x4(false, false));

		btVector3 vecLocalInertia(0, 0, 0);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(0, &pPhysicsEntity->m_oMotionState, pCollisionShape, vecLocalInertia);

		pPhysicsEntity->m_apPhysicsShapes.push_back(new btRigidBody(rbInfo));
		pPhysicsEntity->m_apPhysicsShapes.back()->setUserPointer((void*)pEntity->GetHandle());
		pPhysicsEntity->m_apPhysicsShapes.back()->setWorldTransform(mTransform);

		if (eCollisionType == CT_KINEMATIC)
		{
			pPhysicsEntity->m_apPhysicsShapes.back()->setCollisionFlags((pPhysicsEntity->m_pRigidBody?pPhysicsEntity->m_pRigidBody->getCollisionFlags():0) | btCollisionObject::CF_KINEMATIC_OBJECT);
			pPhysicsEntity->m_apPhysicsShapes.back()->setActivationState(DISABLE_DEACTIVATION);
		}
		else if (eCollisionType == CT_STATIC_MESH)
			pPhysicsEntity->m_apPhysicsShapes.back()->setActivationState(DISABLE_SIMULATION);

		m_pDynamicsWorld->addRigidBody(pPhysicsEntity->m_apPhysicsShapes.back(), pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
	}
}

void CBulletPhysics::AddModelTris(class CBaseEntity* pEntity, collision_type_t eCollisionType, size_t iModel)
{
	CModel* pModel = CModelLibrary::GetModel(iModel);
	if (!pModel)
		return;

	if (!pModel->m_pToy->GetPhysicsNumTris())
		return;

	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEntity);

	btCollisionShape* pCollisionShape;
	float flMass;

	if (eCollisionType == CT_STATIC_MESH)
	{
		pPhysicsEntity->m_bCenterMassOffset = false;

		flMass = 0;
		pCollisionShape = m_apCollisionMeshes[iModel].m_pCollisionShape;
	}
	else if (eCollisionType == CT_KINEMATIC)
	{
		pPhysicsEntity->m_bCenterMassOffset = false;

		flMass = 0;
		pCollisionShape = m_apCollisionMeshes[iModel].m_pCollisionShape;
	}
	else
	{
		TAssert(!"Unimplemented collision type");
	}

	TAssert(pCollisionShape);

	btTransform mTransform;
	mTransform.setIdentity();
	mTransform.setFromOpenGLMatrix(&pEntity->GetPhysicsTransform().m[0][0]);

	bool bDynamic = (flMass != 0.f);

	btVector3 vecLocalInertia(0, 0, 0);
	if (bDynamic)
		pCollisionShape->calculateLocalInertia(flMass, vecLocalInertia);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(flMass, &pPhysicsEntity->m_oMotionState, pCollisionShape, vecLocalInertia);

	if (pEntity->GetModelID() == iModel)
	{
		pPhysicsEntity->m_pRigidBody = new btRigidBody(rbInfo);
		pPhysicsEntity->m_pRigidBody->setUserPointer((void*)pEntity->GetHandle());

		if (eCollisionType == CT_KINEMATIC)
		{
			pPhysicsEntity->m_pRigidBody->setCollisionFlags(pPhysicsEntity->m_pRigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			pPhysicsEntity->m_pRigidBody->setActivationState(DISABLE_DEACTIVATION);
		}
		else if (eCollisionType == CT_STATIC_MESH)
			pPhysicsEntity->m_pRigidBody->setActivationState(DISABLE_SIMULATION);

		m_pDynamicsWorld->addRigidBody(pPhysicsEntity->m_pRigidBody, pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
	}
	else
	{
		// This is a scene area. Handle it a tad differently.
		TAssert(pEntity->GetModel()->m_pToy->GetNumSceneAreas());

		btRigidBody* pBody = new btRigidBody(rbInfo);

		pPhysicsEntity->m_apAreaBodies.push_back(pBody);
		pBody->setUserPointer((void*)pEntity->GetHandle());

		if (eCollisionType == CT_KINEMATIC)
		{
			pBody->setCollisionFlags(pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			pBody->setActivationState(DISABLE_DEACTIVATION);
		}
		else if (eCollisionType == CT_STATIC_MESH)
			pPhysicsEntity->m_pRigidBody->setActivationState(DISABLE_SIMULATION);

		m_pDynamicsWorld->addRigidBody(pBody, pEntity->GetCollisionGroup(), GetMaskForGroup(pEntity->GetCollisionGroup()));
	}
}

void CBulletPhysics::RemoveEntity(CBaseEntity* pEntity)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEntity);
	if (!pPhysicsEntity)
		return;

	RemoveEntity(pPhysicsEntity);
}

void CBulletPhysics::RemoveEntity(CPhysicsEntity* pPhysicsEntity)
{
	if (!pPhysicsEntity)
		return;

	if (pPhysicsEntity->m_pRigidBody)
		m_pDynamicsWorld->removeRigidBody(pPhysicsEntity->m_pRigidBody);
	delete pPhysicsEntity->m_pRigidBody;
	pPhysicsEntity->m_pRigidBody = NULL;

	for (size_t i = 0; i < pPhysicsEntity->m_apAreaBodies.size(); i++)
	{
		m_pDynamicsWorld->removeRigidBody(pPhysicsEntity->m_apAreaBodies[i]);
		delete pPhysicsEntity->m_apAreaBodies[i];
	}
	pPhysicsEntity->m_apAreaBodies.clear();

	for (size_t i = 0; i < pPhysicsEntity->m_apPhysicsShapes.size(); i++)
	{
		m_pDynamicsWorld->removeRigidBody(pPhysicsEntity->m_apPhysicsShapes[i]);
		delete pPhysicsEntity->m_apPhysicsShapes[i];
	}
	pPhysicsEntity->m_apPhysicsShapes.clear();

	if (pPhysicsEntity->m_pGhostObject)
		m_pDynamicsWorld->removeCollisionObject(pPhysicsEntity->m_pGhostObject);
	delete pPhysicsEntity->m_pGhostObject;
	pPhysicsEntity->m_pGhostObject = NULL;

	if (pPhysicsEntity->m_pCharacterController)
		m_pDynamicsWorld->removeAction(pPhysicsEntity->m_pCharacterController);
	delete pPhysicsEntity->m_pCharacterController;
	pPhysicsEntity->m_pCharacterController = NULL;

	if (pPhysicsEntity->m_pTriggerController)
		m_pDynamicsWorld->removeAction(pPhysicsEntity->m_pTriggerController);
	delete pPhysicsEntity->m_pTriggerController;
	pPhysicsEntity->m_pTriggerController = NULL;
}

size_t CBulletPhysics::AddExtra(size_t iExtraMesh, const Vector& vecOrigin)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apExtraEntityList.size(); i++)
	{
		if (!m_apExtraEntityList[i])
		{
			iIndex = i;
			m_apExtraEntityList[i] = new CPhysicsEntity();
			break;
		}
	}

	if (iIndex == ~0)
	{
		iIndex = m_apExtraEntityList.size();
		m_apExtraEntityList.push_back(new CPhysicsEntity());
	}

	CPhysicsEntity* pPhysicsEntity = m_apExtraEntityList[iIndex];
	pPhysicsEntity->m_bCenterMassOffset = false;

	TAssert(m_apExtraCollisionMeshes[iExtraMesh]);
	if (!m_apExtraCollisionMeshes[iExtraMesh])
		return ~0;

	float flMass = 0;
	btCollisionShape* pCollisionShape = m_apExtraCollisionMeshes[iExtraMesh]->m_pCollisionShape;

	TAssert(pCollisionShape);

	btTransform mTransform;
	mTransform.setIdentity();
	mTransform.setOrigin(ToBTVector(vecOrigin));

	bool bDynamic = (flMass != 0.f);

	btVector3 vecLocalInertia(0, 0, 0);
	if (bDynamic)
		pCollisionShape->calculateLocalInertia(flMass, vecLocalInertia);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(flMass, nullptr, pCollisionShape, vecLocalInertia);

	pPhysicsEntity->m_pRigidBody = new btRigidBody(rbInfo);
	pPhysicsEntity->m_pRigidBody->setUserPointer((void*)(GameServer()->GetMaxEntities()+iIndex));
	pPhysicsEntity->m_pRigidBody->setWorldTransform(mTransform);
	pPhysicsEntity->m_pRigidBody->setActivationState(DISABLE_SIMULATION);

	m_pDynamicsWorld->addRigidBody(pPhysicsEntity->m_pRigidBody, CG_STATIC, GetMaskForGroup(CG_STATIC));

	return iIndex;
}

size_t CBulletPhysics::AddExtraBox(const Vector& vecCenter, const Vector& vecSize)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apExtraEntityList.size(); i++)
	{
		if (!m_apExtraEntityList[i])
		{
			iIndex = i;
			m_apExtraEntityList[i] = new CPhysicsEntity();
			break;
		}
	}

	if (iIndex == ~0)
	{
		iIndex = m_apExtraEntityList.size();
		m_apExtraEntityList.push_back(new CPhysicsEntity());
	}

	CPhysicsEntity* pPhysicsEntity = m_apExtraEntityList[iIndex];
	pPhysicsEntity->m_bCenterMassOffset = false;

	float flMass = 0;
	pPhysicsEntity->m_pExtraShape = new btBoxShape(ToBTVector(vecSize)/2);

	btTransform mTransform;
	mTransform.setIdentity();
	mTransform.setOrigin(ToBTVector(vecCenter));

	bool bDynamic = (flMass != 0.f);

	btVector3 vecLocalInertia(0, 0, 0);
	if (bDynamic)
		pPhysicsEntity->m_pExtraShape->calculateLocalInertia(flMass, vecLocalInertia);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(flMass, nullptr, pPhysicsEntity->m_pExtraShape, vecLocalInertia);

	pPhysicsEntity->m_pRigidBody = new btRigidBody(rbInfo);
	pPhysicsEntity->m_pRigidBody->setUserPointer((void*)(GameServer()->GetMaxEntities()+iIndex));
	pPhysicsEntity->m_pRigidBody->setWorldTransform(mTransform);
	pPhysicsEntity->m_pRigidBody->setActivationState(DISABLE_SIMULATION);

	m_pDynamicsWorld->addRigidBody(pPhysicsEntity->m_pRigidBody, CG_STATIC, GetMaskForGroup(CG_STATIC));

	return iIndex;
}

void CBulletPhysics::RemoveExtra(size_t iExtra)
{
	CPhysicsEntity* pPhysicsEntity = m_apExtraEntityList[iExtra];

	TAssert(pPhysicsEntity);
	if (!pPhysicsEntity)
		return;

	m_pDynamicsWorld->removeRigidBody(pPhysicsEntity->m_pRigidBody);

	delete m_apExtraEntityList[iExtra]->m_pExtraShape;
	delete m_apExtraEntityList[iExtra]->m_pRigidBody;
	delete m_apExtraEntityList[iExtra];
	m_apExtraEntityList[iExtra] = nullptr;
}

void CBulletPhysics::RemoveAllEntities()
{
	for (size_t i = 0; i < m_aEntityList.size(); i++)
	{
		auto pPhysicsEntity = &m_aEntityList[i];

		RemoveEntity(pPhysicsEntity);
	}

	for (size_t i = 0; i < m_apExtraEntityList.size(); i++)
	{
		if (m_apExtraEntityList[i])
			RemoveExtra(i);
	}
}

bool CBulletPhysics::IsEntityAdded(CBaseEntity* pEntity)
{
	TAssert(pEntity);
	if (!pEntity)
		return false;

	size_t iHandle = pEntity->GetHandle();

	if (m_aEntityList.size() <= iHandle)
		return false;

	CPhysicsEntity* pPhysicsEntity = &m_aEntityList[iHandle];

	return (pPhysicsEntity->m_pCharacterController || pPhysicsEntity->m_pExtraShape || pPhysicsEntity->m_pGhostObject || pPhysicsEntity->m_pRigidBody || pPhysicsEntity->m_pTriggerController);
}

void CBulletPhysics::LoadCollisionMesh(const tstring& sModel, size_t iTris, int* aiTris, size_t iVerts, float* aflVerts)
{
	size_t iModel = CModelLibrary::FindModel(sModel);

	TAssert(iModel != ~0);

	TAssert(!m_apCollisionMeshes[iModel].m_pIndexVertexArray);
	TAssert(!m_apCollisionMeshes[iModel].m_pCollisionShape);
	if (m_apCollisionMeshes[iModel].m_pIndexVertexArray)
		return;

	m_apCollisionMeshes[iModel].m_pIndexVertexArray = new btTriangleIndexVertexArray();

	btIndexedMesh m;
	m.m_numTriangles = iTris;
	m.m_triangleIndexBase = (const unsigned char *)aiTris;
	m.m_triangleIndexStride = sizeof(int)*3;
	m.m_numVertices = iVerts;
	m.m_vertexBase = (const unsigned char *)aflVerts;
	m.m_vertexStride = sizeof(Vector);
	m_apCollisionMeshes[iModel].m_pIndexVertexArray->addIndexedMesh(m, PHY_INTEGER);

	m_apCollisionMeshes[iModel].m_pCollisionShape = new btBvhTriangleMeshShape(m_apCollisionMeshes[iModel].m_pIndexVertexArray, true);
}

void CBulletPhysics::UnloadCollisionMesh(const tstring& sModel)
{
	size_t iModel = CModelLibrary::FindModel(sModel);

	TAssert(iModel != ~0);
	if (iModel == ~0)
		return;

	auto it = m_apCollisionMeshes.find(iModel);
	TAssert(it != m_apCollisionMeshes.end());
	if (it == m_apCollisionMeshes.end())
		return;

	// Make sure there are no objects using this collision shape.
	for (int i = 0; i < m_pDynamicsWorld->getCollisionObjectArray().size(); i++)
	{
		auto pObject = m_pDynamicsWorld->getCollisionObjectArray()[i];
		TAssert(pObject->getCollisionShape() != it->second.m_pCollisionShape);
		if (pObject->getCollisionShape() == it->second.m_pCollisionShape)
		{
			CEntityHandle<CBaseEntity> hEntity(GetBaseEntity(pObject));
			if (hEntity.GetPointer())
				TError("Entity found with collision shape '" + sModel + "' which is being unloaded: " + tstring(hEntity->GetClassName()) + ":" + hEntity->GetName() + "\n");
			else
				TError("Entity found with 'extra' collision shape which is being unloaded\n");

			RemoveEntity(hEntity);
		}
	}

	delete it->second.m_pCollisionShape;
	delete it->second.m_pIndexVertexArray;

	m_apCollisionMeshes.erase(it->first);
}

size_t CBulletPhysics::LoadExtraCollisionMesh(size_t iTris, int* aiTris, size_t iVerts, float* aflVerts)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apExtraCollisionMeshes.size(); i++)
	{
		if (!m_apExtraCollisionMeshes[i])
		{
			iIndex = i;
			m_apExtraCollisionMeshes[i] = new CCollisionMesh();
			break;
		}
	}

	if (iIndex == ~0)
	{
		iIndex = m_apExtraCollisionMeshes.size();
		m_apExtraCollisionMeshes.push_back(new CCollisionMesh());
	}

	m_apExtraCollisionMeshes[iIndex]->m_pIndexVertexArray = new btTriangleIndexVertexArray();

	btIndexedMesh m;
	m.m_numTriangles = iTris;
	m.m_triangleIndexBase = (const unsigned char *)aiTris;
	m.m_triangleIndexStride = sizeof(int)*3;
	m.m_numVertices = iVerts;
	m.m_vertexBase = (const unsigned char *)aflVerts;
	m.m_vertexStride = sizeof(Vector);
	m_apExtraCollisionMeshes[iIndex]->m_pIndexVertexArray->addIndexedMesh(m, PHY_INTEGER);

	m_apExtraCollisionMeshes[iIndex]->m_pCollisionShape = new btBvhTriangleMeshShape(m_apExtraCollisionMeshes[iIndex]->m_pIndexVertexArray, true);

	return iIndex;
}

void CBulletPhysics::UnloadExtraCollisionMesh(size_t iIndex)
{
	TAssert(m_apExtraCollisionMeshes[iIndex]);
	if (!m_apExtraCollisionMeshes[iIndex])
		return;

	// Make sure there are no objects using this collision shape.
	for (int i = 0; i < m_pDynamicsWorld->getCollisionObjectArray().size(); i++)
	{
		auto pObject = m_pDynamicsWorld->getCollisionObjectArray()[i];
		TAssert(pObject->getCollisionShape() != m_apExtraCollisionMeshes[iIndex]->m_pCollisionShape);
		if (pObject->getCollisionShape() == m_apExtraCollisionMeshes[iIndex]->m_pCollisionShape)
		{
			RemoveExtra(iIndex);
			TError("Entity found with collision mesh which is being unloaded\n");
		}
	}

	delete m_apExtraCollisionMeshes[iIndex]->m_pCollisionShape;
	delete m_apExtraCollisionMeshes[iIndex]->m_pIndexVertexArray;

	delete m_apExtraCollisionMeshes[iIndex];
	m_apExtraCollisionMeshes[iIndex] = nullptr;
}

#ifdef _DEBUG
#define PHYS_TIMESTEP "0.03333333333"
#else
#define PHYS_TIMESTEP "0.01666666666"
#endif

CVar phys_timestep("phys_timestep", PHYS_TIMESTEP);

void CBulletPhysics::Simulate()
{
	TPROF("CBulletPhysics::Simulate");

	m_pDynamicsWorld->stepSimulation((float)GameServer()->GetFrameTime(), 10, phys_timestep.GetFloat());
}

void CBulletPhysics::DebugDraw(int iLevel)
{
	if (!m_pDebugDrawer)
	{
		m_pDebugDrawer = new CPhysicsDebugDrawer();
		m_pDynamicsWorld->setDebugDrawer(m_pDebugDrawer);
	}

	if (iLevel == 0)
		m_pDebugDrawer->setDebugMode(CPhysicsDebugDrawer::DBG_NoDebug);
	else if (iLevel == 1)
		m_pDebugDrawer->setDebugMode(CPhysicsDebugDrawer::DBG_DrawWireframe);
	else if (iLevel >= 2)
		m_pDebugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe|btIDebugDraw::DBG_DrawContactPoints);

	m_pDebugDrawer->SetDrawing(true);
	m_pDynamicsWorld->debugDrawWorld();
	m_pDebugDrawer->SetDrawing(false);
}

collision_type_t CBulletPhysics::GetEntityCollisionType(class CBaseEntity* pEnt)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return CT_NONE;

	return pPhysicsEntity->m_eCollisionType;
}

void CBulletPhysics::SetEntityTransform(class CBaseEntity* pEnt, const Matrix4x4& mTransform)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	if (pPhysicsEntity->m_bCenterMassOffset)
	{
		Matrix4x4 mCenter;
		mCenter.SetTranslation(pEnt->GetPhysBoundingBox().Center());

		btTransform m;
		m.setFromOpenGLMatrix(mCenter * mTransform);

		if (pPhysicsEntity->m_pRigidBody)
			pPhysicsEntity->m_pRigidBody->setCenterOfMassTransform(m);
		else if (pPhysicsEntity->m_pGhostObject)
			pPhysicsEntity->m_pGhostObject->setWorldTransform(m);

		CModel* pModel = pEnt->GetModel();
		CToy* pToy = pModel?pModel->m_pToy:nullptr;
		if (pToy)
		{
			for (size_t i = 0; i < pPhysicsEntity->m_apPhysicsShapes.size(); i++)
			{
				mCenter = pModel->m_pToy->GetPhysicsBox(i).GetMatrix4x4(false, false);

				m.setFromOpenGLMatrix(mCenter * mTransform);

				pPhysicsEntity->m_apPhysicsShapes[i]->setCenterOfMassTransform(m);
			}
		}
	}
	else
	{
		btTransform m;
		m.setFromOpenGLMatrix(mTransform);

		if (pPhysicsEntity->m_pRigidBody)
			pPhysicsEntity->m_pRigidBody->setCenterOfMassTransform(m);
		else if (pPhysicsEntity->m_pGhostObject)
			pPhysicsEntity->m_pGhostObject->setWorldTransform(m);

		for (size_t i = 0; i < pPhysicsEntity->m_apPhysicsShapes.size(); i++)
		{
			TUnimplemented(); // This may work but it also may cause boxes to be misaligned.
			pPhysicsEntity->m_apPhysicsShapes[i]->setCenterOfMassTransform(m);
		}
	}
}

void CBulletPhysics::SetEntityVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	btVector3 v(vecVelocity.x, vecVelocity.y, vecVelocity.z);

	if (pPhysicsEntity->m_pRigidBody)
		pPhysicsEntity->m_pRigidBody->setLinearVelocity(v);
	else if (pPhysicsEntity->m_pCharacterController)
		TAssert(false);
}

Vector CBulletPhysics::GetEntityVelocity(class CBaseEntity* pEnt)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return Vector();

	if (pPhysicsEntity->m_pRigidBody)
		return Vector(pPhysicsEntity->m_pRigidBody->getLinearVelocity());
	else if (pPhysicsEntity->m_pCharacterController)
		return Vector(pPhysicsEntity->m_pCharacterController->GetVelocity());

	return Vector();
}

void CBulletPhysics::SetControllerMoveVelocity(class CBaseEntity* pEnt, const Vector& vecVelocity)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	TAssert(pPhysicsEntity->m_pCharacterController);
	if (pPhysicsEntity->m_pCharacterController)
		pPhysicsEntity->m_pCharacterController->SetMoveVelocity(ToBTVector(vecVelocity));
}

void CBulletPhysics::SetControllerColliding(class CBaseEntity* pEnt, bool bColliding)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	TAssert(pPhysicsEntity->m_pCharacterController);
	if (pPhysicsEntity->m_pCharacterController)
		pPhysicsEntity->m_pCharacterController->SetColliding(bColliding);
}

void CBulletPhysics::SetEntityGravity(class CBaseEntity* pEnt, const Vector& vecGravity)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	btVector3 v(vecGravity.x, vecGravity.y, vecGravity.z);

	if (pPhysicsEntity->m_pRigidBody)
		pPhysicsEntity->m_pRigidBody->setGravity(v);
	else if (pPhysicsEntity->m_pCharacterController)
		pPhysicsEntity->m_pCharacterController->SetGravity(v);
}

void CBulletPhysics::SetEntityUpVector(class CBaseEntity* pEnt, const Vector& vecUp)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	btVector3 v(vecUp.x, vecUp.y, vecUp.z);

	TAssert(pPhysicsEntity->m_pCharacterController);
	if (pPhysicsEntity->m_pCharacterController)
		pPhysicsEntity->m_pCharacterController->SetUpVector(v);
}

void CBulletPhysics::SetLinearFactor(class CBaseEntity* pEnt, const Vector& vecFactor)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	btVector3 v(vecFactor.x, vecFactor.y, vecFactor.z);

	if (pPhysicsEntity->m_pRigidBody)
		pPhysicsEntity->m_pRigidBody->setLinearFactor(v);
	else if (pPhysicsEntity->m_pCharacterController)
		pPhysicsEntity->m_pCharacterController->SetLinearFactor(v);
}

void CBulletPhysics::SetAngularFactor(class CBaseEntity* pEnt, const Vector& vecFactor)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	btVector3 v(vecFactor.x, vecFactor.y, vecFactor.z);

	TAssert(!pPhysicsEntity->m_pCharacterController);
	if (pPhysicsEntity->m_pRigidBody)
		pPhysicsEntity->m_pRigidBody->setAngularFactor(v);
}

void CBulletPhysics::CharacterMovement(class CBaseEntity* pEnt, class btCollisionWorld* pCollisionWorld, float flDelta)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	if (!pPhysicsEntity->m_pCharacterController)
		return;

	pPhysicsEntity->m_pCharacterController->CharacterMovement(pCollisionWorld, flDelta);
}

void CBulletPhysics::TraceLine(CTraceResult& tr, const Vector& v1, const Vector& v2, class CBaseEntity* pIgnore)
{
	btVector3 vecFrom, vecTo;
	vecFrom = ToBTVector(v1);
	vecTo = ToBTVector(v2);

	CClosestRayResultCallback callback(vecFrom, vecTo, pIgnore?GetPhysicsEntity(pIgnore)->m_pRigidBody:nullptr);

	m_pDynamicsWorld->rayTest(vecFrom, vecTo, callback);

	if (callback.m_closestHitFraction < tr.m_flFraction)
	{
		tr.m_flFraction = callback.m_closestHitFraction;
		tr.m_vecHit = ToTVector(callback.m_hitPointWorld);
		tr.m_vecNormal = ToTVector(callback.m_hitNormalWorld);
		tr.m_pHit = CEntityHandle<CBaseEntity>((size_t)callback.m_collisionObject->getUserPointer()).GetPointer();
		if ((size_t)callback.m_collisionObject->getUserPointer() >= GameServer()->GetMaxEntities())
			tr.m_iHitExtra = (size_t)callback.m_collisionObject->getUserPointer() - GameServer()->GetMaxEntities();
	}
}

void CBulletPhysics::TraceEntity(CTraceResult& tr, class CBaseEntity* pEntity, const Vector& v1, const Vector& v2)
{
	btVector3 vecFrom, vecTo;
	vecFrom = ToBTVector(v1);
	vecTo = ToBTVector(v2);

	btTransform mFrom, mTo;
	mFrom.setIdentity();
	mTo.setIdentity();
	mFrom.setOrigin(vecFrom);
	mTo.setOrigin(vecTo);

	CPhysicsEntity* pPhysicsEntity = pEntity?GetPhysicsEntity(pEntity):nullptr;
	TAssert(pPhysicsEntity);
	if (!pPhysicsEntity)
		return;

	TAssert(pPhysicsEntity->m_pRigidBody || pPhysicsEntity->m_pGhostObject);
	if (!pPhysicsEntity->m_pRigidBody && !pPhysicsEntity->m_pGhostObject)
		return;

	btConvexShape* pShape = dynamic_cast<btConvexShape*>(pPhysicsEntity->m_pRigidBody?pPhysicsEntity->m_pRigidBody->getCollisionShape():pPhysicsEntity->m_pGhostObject->getCollisionShape());
	TAssert(pShape);
	if (!pShape)
		return;

	btCollisionObject* pObject = pPhysicsEntity->m_pRigidBody;
	if (!pObject)
		pObject = pPhysicsEntity->m_pGhostObject;

	TAssert(pObject);

	CClosestConvexResultCallback callback(vecFrom, vecTo, pObject);

	m_pDynamicsWorld->convexSweepTest(pShape, mFrom, mTo, callback);

	if (callback.m_closestHitFraction < tr.m_flFraction)
	{
		tr.m_flFraction = callback.m_closestHitFraction;
		tr.m_vecHit = ToTVector(callback.m_hitPointWorld);
		tr.m_vecNormal = ToTVector(callback.m_hitNormalWorld);
		tr.m_pHit = CEntityHandle<CBaseEntity>((size_t)callback.m_hitCollisionObject->getUserPointer()).GetPointer();
		if ((size_t)callback.m_hitCollisionObject->getUserPointer() >= GameServer()->GetMaxEntities())
			tr.m_iHitExtra = (size_t)callback.m_hitCollisionObject->getUserPointer() - GameServer()->GetMaxEntities();
	}
}

void CBulletPhysics::CheckSphere(CTraceResult& tr, float flRadius, const Vector& vecCenter, class CBaseEntity* pIgnore)
{
	btTransform mTransform = btTransform::getIdentity();
	mTransform.setOrigin(ToBTVector(vecCenter));

	CPhysicsEntity* pIgnorePhysicsEntity = pIgnore?GetPhysicsEntity(pIgnore):nullptr;
	btCollisionObject* pIgnoreCollisionObject = nullptr;
	if (pIgnorePhysicsEntity)
	{
		if (pIgnorePhysicsEntity->m_pGhostObject)
			pIgnoreCollisionObject = pIgnorePhysicsEntity->m_pGhostObject;
		else
			pIgnoreCollisionObject = pIgnorePhysicsEntity->m_pRigidBody;
	}

	CAllContactResultsCallback callback(tr, pIgnoreCollisionObject);

	std::shared_ptr<btSphereShape> pSphereShape(new btSphereShape(flRadius));
	btRigidBody::btRigidBodyConstructionInfo rbInfo(0, nullptr, pSphereShape.get());
	std::shared_ptr<btRigidBody> pSphere(new btRigidBody(rbInfo));
	pSphere->setWorldTransform(mTransform);

	m_pDynamicsWorld->contactTest(pSphere.get(), callback);
}

void CBulletPhysics::CharacterJump(class CBaseEntity* pEnt)
{
	CPhysicsEntity* pPhysicsEntity = GetPhysicsEntity(pEnt);
	if (!pPhysicsEntity)
		return;

	TAssert(pPhysicsEntity->m_pCharacterController);
	if (!pPhysicsEntity->m_pCharacterController)
		return;

	pPhysicsEntity->m_pCharacterController->jump();
}

CPhysicsEntity* CBulletPhysics::GetPhysicsEntity(class CBaseEntity* pEnt)
{
	TAssert(pEnt);
	if (!pEnt)
		return NULL;

	size_t iHandle = pEnt->GetHandle();
	TAssert(m_aEntityList.size() > iHandle);
	if (m_aEntityList.size() <= iHandle)
		return NULL;

	CPhysicsEntity* pPhysicsEntity = &m_aEntityList[iHandle];
	TAssert(pPhysicsEntity);

	return pPhysicsEntity;
}

CBaseEntity* CBulletPhysics::GetBaseEntity(btCollisionObject* pObject)
{
	CEntityHandle<CBaseEntity> hEntity((size_t)pObject->getUserPointer());
	return hEntity;
}

short CBulletPhysics::GetMaskForGroup(collision_group_t eGroup)
{
	switch (eGroup)
	{
	case CG_NONE:
	case CG_DEFAULT:
	default:
		return btBroadphaseProxy::AllFilter;

	case CG_STATIC:
		return CG_DEFAULT|CG_CHARACTER_CLIP|CG_CHARACTER_PASS;

	case CG_TRIGGER:
		return CG_DEFAULT|CG_CHARACTER_CLIP|CG_CHARACTER_PASS;

	case CG_CHARACTER_PASS:
		return CG_DEFAULT|CG_STATIC|CG_TRIGGER|CG_CHARACTER_CLIP;

	case CG_CHARACTER_CLIP:
		return CG_DEFAULT|CG_STATIC|CG_TRIGGER|CG_CHARACTER_CLIP|CG_CHARACTER_PASS;
	}
}

CPhysicsManager::CPhysicsManager()
{
	m_pModel = new CBulletPhysics();
}

CPhysicsManager::~CPhysicsManager()
{
	delete m_pModel;
}

void CMotionState::getWorldTransform(btTransform& mCenterOfMass) const
{
	if (m_pPhysics->GetPhysicsEntity(m_hEntity)->m_bCenterMassOffset)
	{
		Matrix4x4 mCenter;
		mCenter.SetTranslation(m_hEntity->GetPhysBoundingBox().Center());

		mCenterOfMass.setFromOpenGLMatrix(mCenter * m_hEntity->GetPhysicsTransform());
	}
	else
		mCenterOfMass.setFromOpenGLMatrix(m_hEntity->GetPhysicsTransform());
}

void CMotionState::setWorldTransform(const btTransform& mCenterOfMass)
{
	Matrix4x4 mGlobal;
	mCenterOfMass.getOpenGLMatrix(mGlobal);

	if (m_pPhysics->GetPhysicsEntity(m_hEntity)->m_bCenterMassOffset)
	{
		Matrix4x4 mCenter;
		mCenter.SetTranslation(m_hEntity->GetPhysBoundingBox().Center());

		m_hEntity->SetPhysicsTransform(mCenter.InvertedRT() * mGlobal);
	}
	else
		m_hEntity->SetPhysicsTransform(mGlobal);
}

CPhysicsModel* GamePhysics()
{
	static CPhysicsModel* pModel = new CBulletPhysics();
	return pModel;
}
