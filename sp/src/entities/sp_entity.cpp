#include "sp_entity.h"

#include <tinker/application.h>
#include <tinker/profiler.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "entities/star.h"
#include "planet/terrain_chunks.h"
#include "planet/terrain_lumps.h"
#include "ui/command_menu.h"

CSPEntityData::CSPEntityData()
{
	m_pCommandMenu = nullptr;

	m_vecCommandMenuRenderOffset = Vector(0, 0, 0);
	m_flCommandMenuProjectionDistance = 0.4f;
	m_flCommandMenuProjectionRadius = 1.0f;
}

bool CSPEntityData::ShouldRenderAtScale(scale_t eScale) const
{
	// Preferably should do this in some way that doesn't require dynamic_cast.
	CPlanet* pPlanet = dynamic_cast<CPlanet*>(m_pEntity);

	if (pPlanet)
		return pPlanet->ShouldRenderAtScale(eScale);

	if (dynamic_cast<CStar*>(m_pEntity))
		return true;

	return (eScale == GetScale());
}

CScalableMatrix CSPEntityData::GetScalableRenderTransform() const
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pCharacterPlanet = pCharacter->GetNearestPlanet();
	CPlanet* pPlanet = GetPlanet();

	if (pCharacterPlanet == m_pEntity)
	{
		CScalableVector vecCharacterOrigin;
		if (pCharacter->GetMoveParent() == m_pEntity)
			vecCharacterOrigin = pCharacter->GetLocalOrigin();
		else
			vecCharacterOrigin = m_pEntity->GetGlobalToLocalTransform() * pCharacter->GetGlobalOrigin();

		CScalableMatrix mTransform;
		mTransform.SetTranslation(-vecCharacterOrigin);

		return mTransform;
	}
	else if (pCharacterPlanet)
	{
		if (pPlanet == pCharacterPlanet)
		{
			CScalableVector vecCharacterOrigin = pPlanet->GetGlobalToLocalTransform() * pCharacter->GetGlobalOrigin();

			CScalableMatrix mTransform = pPlanet->GetGlobalToLocalTransform() * m_pEntity->GetGlobalTransform();
			mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

			return mTransform;
		}
		else
		{
			CScalableMatrix mGlobalToLocal = pCharacterPlanet->GetGlobalToLocalTransform();

			CScalableVector vecCharacterOrigin = mGlobalToLocal * pCharacter->GetGlobalOrigin();

			CScalableMatrix mTransform = mGlobalToLocal * m_pEntity->GetGlobalTransform();
			mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

			return mTransform;
		}
	}
	else
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		CScalableMatrix mTransform = m_pEntity->GetGlobalTransform();
		mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

		return mTransform;
	}
}

CScalableVector CSPEntityData::GetScalableRenderOrigin() const
{
	return GetScalableRenderTransform().GetTranslation();
}

const Matrix4x4 CSPEntityData::GetRenderTransform() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetUnits(eScale);
}

const Vector CSPEntityData::GetRenderOrigin() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetTranslation().GetUnits(eScale);
}

const DoubleVector CSPEntityData::TransformPointPhysicsToLocal(const Vector& v) const
{
	CPlanet* pPlanet = GetPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetGroupCenterToPlanetTransform() * v;
}

const Vector CSPEntityData::TransformPointLocalToPhysics(const DoubleVector& v) const
{
	CPlanet* pPlanet = GetPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform() * v;
}

const DoubleVector CSPEntityData::TransformVectorPhysicsToLocal(const Vector& v) const
{
	CPlanet* pPlanet = GetPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetGroupCenterToPlanetTransform().TransformVector(v);
}

const Vector CSPEntityData::TransformVectorLocalToPhysics(const Vector& v) const
{
	CPlanet* pPlanet = GetPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform().TransformVector(v);
}

void CSPEntityData::SetPlanet(CPlanet* pPlanet)
{
	m_hPlanet = pPlanet;
}

CPlanet* CSPEntityData::GetPlanet() const
{
	return m_hPlanet;
}

CTerrainLump* CSPEntityData::GetLump() const
{
	if (m_oLump.iTerrain == ~0)
		return nullptr;

	if (!GetPlanet())
		return nullptr;

	return GetPlanet()->GetLumpManager()->GetLump(m_oLump);
}

CVoxelTree* CSPEntityData::GetVoxelTree() const
{
	CTerrainLump* pLump = GetLump();

	if (!pLump)
		return nullptr;

	return pLump->GetVoxelTree();
}

void CSPEntityData::SetPlayerOwner(CSPPlayer* pPlayer)
{
	m_hPlayerOwner = pPlayer;
}

CSPPlayer* CSPEntityData::GetPlayerOwner() const
{
	return m_hPlayerOwner;
}

CCommandMenu* CSPEntityData::CreateCommandMenu(CPlayerCharacter* pRequester)
{
	CloseCommandMenu();

	m_pCommandMenu = new CCommandMenu(m_pEntity, pRequester);

	return GetCommandMenu();
}

void CSPEntityData::CloseCommandMenu()
{
	if (!m_pCommandMenu)
		return;

	delete m_pCommandMenu;
	m_pCommandMenu = nullptr;
}

CCommandMenu* CSPEntityData::GetCommandMenu() const
{
	return m_pCommandMenu;
}

void CSPEntityData::SetCommandMenuParameters(const Vector& vecRenderOffset, float flProjectionDistance, float flProjectionRadius)
{
	m_vecCommandMenuRenderOffset = vecRenderOffset;
	m_flCommandMenuProjectionDistance = flProjectionDistance;
	m_flCommandMenuProjectionRadius = flProjectionRadius;
}
