#include "trees.h"

#include <renderer/renderingcontext.h>
#include <tengine/renderer/game_renderer.h>

#include "entities/sp_game.h"
#include "entities/sp_playercharacter.h"
#include "sp_renderer.h"

#include "planet.h"
#include "planet_terrain.h"
#include "terrain_chunks.h"

CTreeManager::CTreeManager(CPlanet* pPlanet)
{
	m_pPlanet = pPlanet;
}

void CTreeManager::GenerateTrees(class CTerrainChunk* pChunk)
{
	CChunkTrees* pChunkTrees = GetChunkTrees(pChunk->GetTerrain(), pChunk->GetMin2D());

	if (pChunkTrees)
		return;

	AddChunkTrees(pChunk->GetTerrain(), pChunk->GetMin2D())->GenerateTrees(pChunk);
}

void CTreeManager::LoadChunk(class CTerrainChunk* pChunk)
{
	CChunkTrees* pChunkTrees = GetChunkTrees(pChunk->GetTerrain(), pChunk->GetMin2D());

	TAssert(pChunkTrees);
	if (!pChunkTrees)
		return;

	pChunkTrees->Load();
}

void CTreeManager::UnloadChunk(class CTerrainChunk* pChunk)
{
	CChunkTrees* pChunkTrees = GetChunkTrees(pChunk->GetTerrain(), pChunk->GetMin2D());

	TAssert(pChunkTrees);
	if (!pChunkTrees)
		return;

	pChunkTrees->Unload();
}

void CTreeManager::Render() const
{
	if (SPGame()->GetSPRenderer()->GetRenderingScale() != SCALE_RENDER)
		return;

	double flScale = CScalableFloat::ConvertUnits(1, m_pPlanet->GetScale(), SCALE_METER);

	DoubleVector vecCharacterOrigin;
	if (SPGame()->GetLocalPlayerCharacter()->GetMoveParent() == m_pPlanet)
		vecCharacterOrigin = m_pPlanet->GetCharacterLocalOrigin()*flScale;
	else
		vecCharacterOrigin = (m_pPlanet->GetGlobalToLocalTransform() * SPGame()->GetLocalPlayerCharacter()->GetGlobalOrigin()).GetUnits(SCALE_METER);

	for (auto it = m_aChunkTrees.begin(); it != m_aChunkTrees.end(); it++)
	{
		const CChunkTrees* pTrees = &it->second;

		if (!SPGame()->GetSPRenderer()->IsInFrustumAtScale(SCALE_METER, Vector(pTrees->m_vecLocalCenter*flScale-vecCharacterOrigin), (float)(pTrees->m_flRadius*flScale)))
			continue;

		pTrees->Render();
	}
}

CChunkTrees* CTreeManager::GetChunkTrees(size_t iTerrain, const DoubleVector2D& vecMin)
{
	CChunkAddress oChunkAddress;
	oChunkAddress.iTerrain = iTerrain;
	oChunkAddress.vecMin = vecMin;

	auto it = m_aChunkTrees.find(oChunkAddress);

	if (it == m_aChunkTrees.end())
		return nullptr;

	return &it->second;
}

CChunkTrees* CTreeManager::AddChunkTrees(size_t iTerrain, const DoubleVector2D& vecMin)
{
	CChunkAddress oChunkAddress;
	oChunkAddress.iTerrain = iTerrain;
	oChunkAddress.vecMin = vecMin;

	return &m_aChunkTrees.insert(tpair<CChunkAddress, CChunkTrees>(oChunkAddress, CChunkTrees(this))).first->second;
}

void CChunkTrees::GenerateTrees(class CTerrainChunk* pChunk)
{
	m_bTreesGenerated = false;

	m_avecOrigins.clear();

	m_iTerrain = pChunk->GetTerrain();
	m_vecMin = pChunk->GetMin2D();
	m_vecMax = pChunk->GetMax2D();

	m_vecLocalCenter = pChunk->GetLocalCenter();
	m_flRadius = pChunk->GetRadius();

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	// Calculate how many meters to a side of this chunk.
	DoubleVector vecSideMin = pTerrain->CoordToWorld(m_vecMin);
	DoubleVector vecSideMax = pTerrain->CoordToWorld(DoubleVector2D(m_vecMax.x, m_vecMin.y));

	size_t iRows = (size_t)CScalableFloat((vecSideMax - vecSideMin).Length(), m_pManager->m_pPlanet->GetScale()).GetMeters();

	// One tree per every five meters.
	iRows /= 10;

	for (size_t x = 0; x < iRows; x++)
	{
		double flX = RemapVal((double)x, 0, (double)iRows-1, m_vecMin.x, m_vecMax.x);
		for (size_t y = 0; y < iRows; y++)
		{
			double flY = RemapVal((double)y, 0, (double)iRows-1, m_vecMin.y, m_vecMax.y);
			DoubleVector2D vecPoint(flX, flY);

			float flTree = pTerrain->GenerateTrees(vecPoint);

			if (flTree == 0 || RandomFloat(0, 1) > flTree)
				continue;

			float flXOffset = RemapVal(RandomFloat(0, 1), 0, 1, 0, (float)(m_vecMax.x - m_vecMin.x)/iRows);
			float flYOffset = RemapVal(RandomFloat(0, 1), 0, 1, 0, (float)(m_vecMax.y - m_vecMin.y)/iRows);

			DoubleVector2D vecTree2D = vecPoint + DoubleVector2D(flXOffset, flYOffset);

			DoubleVector vecTree = pTerrain->CoordToWorld(vecTree2D) + pTerrain->GenerateOffset(vecTree2D);

			m_avecOrigins.push_back(vecTree);
		}
	}

	m_bTreesGenerated = true;
}

void CChunkTrees::Load()
{
	TAssert(m_pManager->m_pPlanet->GetChunkManager()->HasGroupCenter());
	TAssert(m_avecOrigins.size());
	TAssert(!m_aiPhysicsBoxes.size());

	DoubleMatrix4x4 mPlanetToGroup = m_pManager->m_pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform();

	float flScale = (float)CScalableFloat::ConvertUnits(1, m_pManager->m_pPlanet->GetScale(), SCALE_METER);

	for (size_t i = 0; i < m_avecOrigins.size(); i++)
	{
		size_t iPhysicsBox = GamePhysics()->AddExtraBox(mPlanetToGroup * (m_avecOrigins[i]*flScale), Vector(1, 5, 1));

		m_aiPhysicsBoxes.push_back(iPhysicsBox);
	}
}

void CChunkTrees::Unload()
{
	for (size_t i = 0; i < m_aiPhysicsBoxes.size(); i++)
		GamePhysics()->RemoveExtra(m_aiPhysicsBoxes[i]);
}

void CChunkTrees::Render() const
{
	if (!m_bTreesGenerated)
		return;

	if (!m_avecOrigins.size())
		return;

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
	vecRight *= 2.5;
	vecUp = m_avecOrigins[m_avecOrigins.size()/2].Normalized() * 10;

	Vector v1 = -vecRight + vecUp;
	Vector v2 = -vecRight;
	Vector v3 = vecRight;
	Vector v4 = vecRight + vecUp;

	float flScale = (float)CScalableFloat::ConvertUnits(1, m_pManager->m_pPlanet->GetScale(), SCALE_METER);

	CRenderingContext r(GameServer()->GetRenderer(), true);

	r.UseMaterial("textures/tree.mat");
	r.SetUniform("vecColor", Vector4D(1, 1, 1, 1));

	size_t iTrees = m_avecOrigins.size();
	for (size_t i = 0; i < iTrees; i++)
	{
		DoubleVector vecTree = m_avecOrigins[i];

		r.ResetTransformations();
		r.Translate((vecTree - m_pManager->m_pPlanet->GetCharacterLocalOrigin())*flScale - vecUp*0.1f);

		r.BeginRenderTriFan();
			r.TexCoord(0.0f, 1.0f);
			r.Vertex(v1);
			r.TexCoord(0.0f, 0.0f);
			r.Vertex(v2);
			r.TexCoord(1.0f, 0.0f);
			r.Vertex(v3);
			r.TexCoord(1.0f, 1.0f);
			r.Vertex(v4);
		r.EndRender();
	}
}
