#include "planet_terrain.h"

#include <geometry.h>
#include <mtrand.h>
#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>
#include <tengine/renderer/game_renderer.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "planet/terrain_lumps.h"

CPlanetTerrain::CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection)
{
	m_pPlanet = pPlanet;
	m_vecDirection = vecDirection;

	m_iShell1VBO = 0;
	m_iShell2VBO = 0;
	m_iShell2IBO = 0;

	m_bGeneratingShell2 = false;
}

void CPlanetTerrain::Think()
{
	if (!m_iShell1VBO)
		CreateShell1VBO();

	CMutexLocker oLock = m_pPlanet->s_pShell2Generator->GetLock();
	oLock.Lock();
	if (m_aiShell2Drop.size())
	{
		if (m_iShell2IBO)
			CRenderer::UnloadVertexDataFromGL(m_iShell2IBO);
		m_iShell2IBO = CRenderer::LoadIndexDataIntoGL(m_aiShell2Drop.size() * sizeof(unsigned int), m_aiShell2Drop.data());
		m_iShell2IBOSize = m_aiShell2Drop.size();
		m_aiShell2Drop.set_capacity(0);
	}
	oLock.Unlock();

	if (m_bGeneratingShell2)
	{
		CMutexLocker oLock2 = m_pPlanet->s_pShell2Generator->GetLock();
		oLock2.Lock();
		if (m_aflShell2Drop.size())
		{
			m_iShell2VBO = CRenderer::LoadVertexDataIntoGL(m_aflShell2Drop.size() * sizeof(float), m_aflShell2Drop.data());
			m_aflShell2Drop.set_capacity(0);
			m_bGeneratingShell2 = false;
		}
	}
	else if (!m_iShell2VBO && !m_bGeneratingShell2)
	{
		if (m_pPlanet->GameData().GetScalableRenderOrigin().LengthSqr() < CScalableFloat(100.0f, SCALE_MEGAMETER)*CScalableFloat(100.0f, SCALE_MEGAMETER))
		{
			m_bGeneratingShell2 = true;

			CShell2GenerationJob oJob;
			oJob.pTerrain = this;
			m_pPlanet->s_pShell2Generator->AddJob(&oJob, sizeof(oJob));
		}
	}
	else if (m_iShell2VBO)
	{
		if (m_pPlanet->GameData().GetScalableRenderOrigin().LengthSqr() > CScalableFloat(1.0f, SCALE_GIGAMETER)*CScalableFloat(1.0f, SCALE_GIGAMETER))
		{
			CRenderer::UnloadVertexDataFromGL(m_iShell2VBO);
			m_iShell2VBO = 0;
		}
	}
}

size_t CPlanetTerrain::BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, DoubleMatrix4x4& mPlanetToChunk, size_t iDepth, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax, const DoubleVector& vecOrigin, bool bSkirt)
{
	bSkirt = false;
	size_t iQuadsPerRow = (size_t)pow(2.0f, (float)iDepth);
	size_t iVertsPerRow = iQuadsPerRow + 1;
	avecTerrain.resize(iVertsPerRow*iVertsPerRow);

	size_t iDetailLevel = (size_t)pow(2.0, (double)m_pPlanet->m_iMeterDepth);

	float flScale = (float)CScalableFloat::ConvertUnits(1, m_pPlanet->GetScale(), SCALE_METER);

	DoubleVector2D vecCenter2D = (vecMax+vecMin)/2;

	DoubleVector vecCenter, vecUp, vecCenterForward, vecCenterRight;
	vecCenter = CoordToWorld(vecCenter2D);
	vecUp = vecCenter.Normalized();
	vecCenterForward = vecUp.Cross(CoordToWorld(DoubleVector2D(vecCenter2D.x, vecMax.y))).Normalized();
	vecCenterRight = vecCenterForward.Cross(vecUp).Normalized();

	DoubleMatrix4x4 mChunk;
	mChunk.SetForwardVector(vecCenterForward);
	mChunk.SetRightVector(vecCenterRight);
	mChunk.SetUpVector(vecUp);
	mChunk.SetTranslation(vecOrigin * flScale);

	mPlanetToChunk = mChunk.InvertedRT();

	for (size_t x = 0; x < iVertsPerRow; x++)
	{
		double flX = RemapVal((double)x, 0, (double)iVertsPerRow-1, vecMin.x, vecMax.x);

		for (size_t y = 0; y < iVertsPerRow; y++)
		{
			double flY = RemapVal((double)y, 0, (double)iVertsPerRow-1, vecMin.y, vecMax.y);

			DoubleVector2D vecPoint(flX, flY);

			CTerrainPoint& vecTerrain = avecTerrain[y*iVertsPerRow + x];

			vecTerrain.vec2DPosition = vecPoint;

			// Offsets are generated in planet space, add them right onto the quad position.
			vecTerrain.vec3DPosition = CoordToWorld(vecPoint) + GenerateOffset(vecPoint);

//			vecTerrain.vecNormal = vecTerrain.vec3DPosition.Normalized();

			vecTerrain.vecPhys = mPlanetToChunk * (vecTerrain.vec3DPosition * flScale);

			vecTerrain.vec3DPosition -= vecOrigin;

			vecTerrain.vecDetail = vecTerrain.vec2DPosition * (double)iDetailLevel;
			vecTerrain.vecDetail.x = fmod(vecTerrain.vecDetail.x, 100);
			vecTerrain.vecDetail.y = fmod(vecTerrain.vecDetail.y, 100);
		}
	}

	for (size_t x = 0; x < iVertsPerRow; x++)
	{
		for (size_t y = 0; y < iVertsPerRow; y++)
		{
			CTerrainPoint& vecTerrain = avecTerrain[y*iVertsPerRow + x];

			Vector vecNormal(0, 0, 0);

			Vector vecUp = (vecTerrain.vec3DPosition + vecOrigin).Normalized();

			if (x > 0)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[y*iVertsPerRow + (x-1)];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (x < iVertsPerRow-1)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[y*iVertsPerRow + (x+1)];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (y > 0)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[(y-1)*iVertsPerRow + x];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (y < iVertsPerRow-1)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[(y+1)*iVertsPerRow + x];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			vecTerrain.vecNormal = vecNormal.Normalized();
		}
	}

	if (bSkirt)
	{
		avecTerrain.resize(iVertsPerRow*iVertsPerRow + iVertsPerRow*4);

		float flSkirtLength = (float)(avecTerrain[0].vec3DPosition - avecTerrain[iVertsPerRow/16].vec3DPosition).Length();

		size_t iSkirt1Start = iVertsPerRow*iVertsPerRow;
		size_t iSkirt2Start = iVertsPerRow*iVertsPerRow + iVertsPerRow;
		size_t iSkirt3Start = iVertsPerRow*iVertsPerRow + iVertsPerRow*2;
		size_t iSkirt4Start = iVertsPerRow*iVertsPerRow + iVertsPerRow*3;

		for (size_t x = 0; x < iVertsPerRow; x++)
		{
			CTerrainPoint& vecSkirt1 = avecTerrain[iSkirt1Start + x];
			vecSkirt1 = avecTerrain[x];
			Vector vecToCenter = (vecSkirt1.vec3DPosition + vecOrigin).Normalized();
			vecSkirt1.vec3DPosition -= vecToCenter*flSkirtLength;

			CTerrainPoint& vecSkirt2  = avecTerrain[iSkirt2Start + x];
			vecSkirt2 = avecTerrain[iVertsPerRow*(iVertsPerRow-1) + x];
			vecToCenter = (vecSkirt2.vec3DPosition + vecOrigin).Normalized();
			vecSkirt2.vec3DPosition -= vecToCenter*flSkirtLength;

			CTerrainPoint& vecSkirt3  = avecTerrain[iSkirt3Start + x];
			vecSkirt3 = avecTerrain[x*iVertsPerRow];
			vecToCenter = (vecSkirt3.vec3DPosition + vecOrigin).Normalized();
			vecSkirt3.vec3DPosition -= vecToCenter*flSkirtLength;

			CTerrainPoint& vecSkirt4  = avecTerrain[iSkirt4Start + x];
			vecSkirt4 = avecTerrain[x*iVertsPerRow + iVertsPerRow-1];
			vecToCenter = (vecSkirt4.vec3DPosition + vecOrigin).Normalized();
			vecSkirt4.vec3DPosition -= vecToCenter*flSkirtLength;
		}
	}

	return iVertsPerRow;
}

void PushVert(tvector<float>& aflVerts, const CTerrainPoint& vecPoint)
{
	aflVerts.push_back((float)vecPoint.vec3DPosition.x);
	aflVerts.push_back((float)vecPoint.vec3DPosition.y);
	aflVerts.push_back((float)vecPoint.vec3DPosition.z);
	aflVerts.push_back(vecPoint.vecNormal.x);
	aflVerts.push_back(vecPoint.vecNormal.y);
	aflVerts.push_back(vecPoint.vecNormal.z);
	aflVerts.push_back((float)vecPoint.vec2DPosition.x);	// 2D position acts as the texture coordinate
	aflVerts.push_back((float)vecPoint.vec2DPosition.y);
	aflVerts.push_back((float)vecPoint.vecDetail.x);
	aflVerts.push_back((float)vecPoint.vecDetail.y);
}

size_t BuildVerts(tvector<float>& aflVerts, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows)
{
	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	aflVerts.reserve(iVertSize * iTriangles * 3);

	for (size_t x = 0; x < iRows-1; x++)
	{
		for (size_t y = 0; y < iRows-1; y++)
		{
			const CTerrainPoint& vecTerrain1 = avecTerrain[y*iRows + x];
			const CTerrainPoint& vecTerrain2 = avecTerrain[y*iRows + (x+1)];
			const CTerrainPoint& vecTerrain3 = avecTerrain[(y+1)*iRows + (x+1)];
			const CTerrainPoint& vecTerrain4 = avecTerrain[(y+1)*iRows + x];

			PushVert(aflVerts, vecTerrain1);
			PushVert(aflVerts, vecTerrain2);
			PushVert(aflVerts, vecTerrain3);

			PushVert(aflVerts, vecTerrain1);
			PushVert(aflVerts, vecTerrain3);
			PushVert(aflVerts, vecTerrain4);
		}
	}

	return iTriangles;
}

void BuildSkirt(tvector<unsigned int>& aiIndices, size_t iRows)
{
	unsigned int iSkirt1Start = iRows*iRows;
	unsigned int iSkirt2Start = iRows*iRows + iRows;
	unsigned int iSkirt3Start = iRows*iRows + iRows*2;
	unsigned int iSkirt4Start = iRows*iRows + iRows*3;

	unsigned int i1;
	unsigned int i2;
	unsigned int i3;
	unsigned int i4;

	for (size_t x = 0; x < iRows-1; x++)
	{
		i1 = iSkirt1Start + x;
		i2 = iSkirt1Start + (x+1);
		i3 = x+1;
		i4 = x;
		aiIndices.push_back(i1);
		aiIndices.push_back(i2);
		aiIndices.push_back(i3);

		aiIndices.push_back(i1);
		aiIndices.push_back(i3);
		aiIndices.push_back(i4);

		i1 = iSkirt2Start + x;
		i2 = iSkirt2Start + (x+1);
		i3 = iRows*(iRows-1) + (x+1);
		i4 = iRows*(iRows-1) + x;

		// Reverse winding order for the opposite side.
		aiIndices.push_back(i1);
		aiIndices.push_back(i3);
		aiIndices.push_back(i2);

		aiIndices.push_back(i1);
		aiIndices.push_back(i4);
		aiIndices.push_back(i3);

		i1 = iSkirt3Start + x;
		i2 = iSkirt3Start + (x+1);
		i3 = (x+1)*iRows;
		i4 = x*iRows;

		// Reverse winding order for the opposite side.
		aiIndices.push_back(i1);
		aiIndices.push_back(i3);
		aiIndices.push_back(i2);

		aiIndices.push_back(i1);
		aiIndices.push_back(i4);
		aiIndices.push_back(i3);

		i1 = iSkirt4Start + x;
		i2 = iSkirt4Start + (x+1);
		i3 = (x+1)*iRows + (iRows-1);
		i4 = x*iRows + (iRows-1);

		aiIndices.push_back(i1);
		aiIndices.push_back(i2);
		aiIndices.push_back(i3);

		aiIndices.push_back(i1);
		aiIndices.push_back(i3);
		aiIndices.push_back(i4);
	}
}

size_t CPlanetTerrain::BuildIndexedVerts(tvector<float>& aflVerts, tvector<unsigned int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows, bool bSkirt)
{
	bSkirt = false;
	BuildMeshIndices(aiIndices, tvector<CTerrainCoordinate>(), iRows, bSkirt);

	return BuildVertsForIndex(aflVerts, avecTerrain, iLevels, iRows, bSkirt);
}

size_t CPlanetTerrain::BuildVertsForIndex(tvector<float>& aflVerts, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows, bool bSkirt)
{
	bSkirt = false;
	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	if (bSkirt)
		iTriangles += (int)pow(2.0f, (float)iLevels) * 4 * 2;

	aflVerts.reserve(iTriangles*3*iVertSize);

	size_t iTerrainSize = avecTerrain.size();
	for (size_t x = 0; x < iTerrainSize; x++)
		PushVert(aflVerts, avecTerrain[x]);

	return iTriangles;
}

void PushPhysVert(tvector<float>& aflVerts, const CTerrainPoint& vecPoint)
{
	aflVerts.push_back((float)vecPoint.vecPhys.x);
	aflVerts.push_back((float)vecPoint.vecPhys.y);
	aflVerts.push_back((float)vecPoint.vecPhys.z);
}

size_t CPlanetTerrain::BuildIndexedPhysVerts(tvector<float>& aflVerts, tvector<int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows)
{
	size_t iVertSize = 3;	// position only

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	aiIndices.reserve(iTriangles * 3);

	aflVerts.reserve(avecTerrain.size()*iVertSize);

	size_t iTerrainSize = avecTerrain.size();

	size_t iInputRows = iRows;

	size_t iInputLevels = (int)(log((float)iInputRows-1)/log(2.0f));
	TAssert(iInputLevels >= iLevels && iInputLevels < 100);
	size_t iRowStride = (int)pow(2.0f, (float)(iInputLevels-iLevels));

	size_t iOutputRows = (int)pow(2.0f, (float)iLevels)+1;

	for (size_t x = 0; x < iInputRows; x += iRowStride)
	{
		for (size_t y = 0; y < iInputRows; y += iRowStride)
			PushPhysVert(aflVerts, avecTerrain[x*iInputRows + y]);
	}

	for (size_t x = 0; x < iOutputRows-1; x++)
	{
		for (size_t y = 0; y < iOutputRows-1; y++)
		{
			unsigned int i1 = y*iOutputRows + x;
			unsigned int i2 = y*iOutputRows + (x+1);
			unsigned int i3 = (y+1)*iOutputRows + (x+1);
			unsigned int i4 = (y+1)*iOutputRows + x;

			TAssert(i1 < iTerrainSize);
			TAssert(i2 < iTerrainSize);
			TAssert(i3 < iTerrainSize);
			TAssert(i4 < iTerrainSize);

			aiIndices.push_back(i1);
			aiIndices.push_back(i2);
			aiIndices.push_back(i3);

			aiIndices.push_back(i1);
			aiIndices.push_back(i3);
			aiIndices.push_back(i4);
		}
	}

	return iTriangles;
}

size_t CPlanetTerrain::BuildMeshIndices(tvector<unsigned int>& aiIndices, const tvector<CTerrainCoordinate>& aiExclude, size_t iRows, bool bSkirt)
{
	return BuildMeshIndices(aiIndices, aiExclude, 0, 0, 1, iRows-1, iRows, bSkirt);
}

size_t CPlanetTerrain::BuildMeshIndices(tvector<unsigned int>& aiIndices, const tvector<CTerrainCoordinate>& aiExclude, size_t iX, size_t iY, size_t iStep, size_t iRowsToIndex, size_t iRowsTotal, bool bSkirt)
{
	bSkirt = false;
	if ((iX || iY) && bSkirt)
		TUnimplemented();

	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = iRowsToIndex*iRowsToIndex*2;
	if (bSkirt)
		iTriangles += iRowsToIndex*2;

	aiIndices.reserve(iTriangles * 3);

	iTriangles = 0;

	for (size_t x = iX; x < iX+iRowsToIndex; x += iStep)
	{
		for (size_t y = iY; y < iY+iRowsToIndex; y += iStep)
		{
			if (iStep == 1)
			{
				bool bSkip = false;
				for (size_t i = 0; i < aiExclude.size(); i++)
				{
					if (x == aiExclude[i].x && y == aiExclude[i].y)
					{
						bSkip = true;
						break;
					}
				}

				if (bSkip)
					continue;
			}

			unsigned int i1 = y*iRowsTotal + x;
			unsigned int i2 = y*iRowsTotal + (x+iStep);
			unsigned int i3 = (y+iStep)*iRowsTotal + (x+iStep);
			unsigned int i4 = (y+iStep)*iRowsTotal + x;

			aiIndices.push_back(i1);
			aiIndices.push_back(i2);
			aiIndices.push_back(i3);

			aiIndices.push_back(i1);
			aiIndices.push_back(i3);
			aiIndices.push_back(i4);

			iTriangles += 2;
		}
	}

	if (bSkirt)
		BuildSkirt(aiIndices, iRowsTotal);

	aiIndices.set_capacity(aiIndices.size());

	return iTriangles;
}

class CCurrentNode
{
public:
	size_t iNode;
	int    iPointListFirst;
	int    iPointListSplit;
	int    iPointListLast;
};

void CPlanetTerrain::BuildKDTree(tvector<CKDPointTreeNode>& aNodes, tvector<CKDPointTreePoint>& aPoints, const tvector<CTerrainPoint>& avecTerrain, size_t iRows)
{
	aPoints.reserve(avecTerrain.size());
	aNodes.reserve(avecTerrain.size());

	CKDPointTreeNode& oTop = aNodes.push_back();

	oTop.oBounds.m_vecMins = avecTerrain[0].vecPhys;
	oTop.oBounds.m_vecMaxs = avecTerrain[0].vecPhys;

	for (size_t x = 0; x < iRows-1; x++)
	{
		for (size_t y = 0; y < iRows-1; y++)
		{
			CKDPointTreePoint& oPoint = aPoints.push_back();

			// Find the center of these four points. This center point will represent the entire area.
			oPoint.vec3DPosition = (avecTerrain[x*iRows+y].vecPhys + avecTerrain[(x+1)*iRows+y].vecPhys + avecTerrain[x*iRows+y+1].vecPhys + avecTerrain[(x+1)*iRows+y+1].vecPhys)/4;
			oPoint.vec2DMin = avecTerrain[x*iRows+y].vec2DPosition;
			oPoint.vec2DMax = avecTerrain[(x+1)*iRows+y+1].vec2DPosition;

			oTop.oBounds.Expand(oPoint.vec3DPosition);
		}
	}

	tvector<CCurrentNode> aiCurrentNode;
	aiCurrentNode.reserve(40);
	CCurrentNode& oFirst = aiCurrentNode.push_back();
	oFirst.iNode = 0;
	oFirst.iPointListFirst = 0;
	oFirst.iPointListLast = aPoints.size()-1;

	while (aiCurrentNode.size())
	{
		CCurrentNode& oCurrentIndex = aiCurrentNode.back();
		CKDPointTreeNode& oNode = aNodes[oCurrentIndex.iNode];

		if (oNode.iLeft == ~0 && oCurrentIndex.iPointListLast - oCurrentIndex.iPointListFirst > 5)
		{
			Vector vecBoundsSize = oNode.oBounds.Size();

			// Find the largest axis.
			if (vecBoundsSize.x > vecBoundsSize.y)
				oNode.iSplitAxis = 0;
			else
				oNode.iSplitAxis = 1;
			if (vecBoundsSize.z > vecBoundsSize[oNode.iSplitAxis])
				oNode.iSplitAxis = 2;

			// Use better algorithm!
			oNode.flSplitPos = oNode.oBounds.m_vecMins[oNode.iSplitAxis] + vecBoundsSize[oNode.iSplitAxis]/2;

			// Partially sort the points list using the quicksort method around the selected point.
			oCurrentIndex.iPointListSplit = oCurrentIndex.iPointListFirst;
			for (size_t i = oCurrentIndex.iPointListFirst; i <= (size_t)oCurrentIndex.iPointListLast; i++)
			{
				if (aPoints[i].vec3DPosition[oNode.iSplitAxis] < oNode.flSplitPos)
				{
					std::swap(aPoints[i], aPoints[oCurrentIndex.iPointListSplit]);
					oCurrentIndex.iPointListSplit++;
				}
			}

			// iSplitIndex is now index of the first point above the split axis. Points on each side of the split are not yet sorted.

			aNodes.push_back();
			CKDPointTreeNode& oRightChild = aNodes.push_back();
			CKDPointTreeNode& oLeftChild = *(aNodes.end()-2);

			// aNodes.push_back() may have invalidated old pointers so grab this again.
			CKDPointTreeNode& oNode = aNodes[oCurrentIndex.iNode];

			oLeftChild.iParent = oCurrentIndex.iNode;
			oRightChild.iParent = oCurrentIndex.iNode;

			oLeftChild.oBounds = oRightChild.oBounds = oNode.oBounds;

			oLeftChild.oBounds.m_vecMaxs[oNode.iSplitAxis] = oNode.flSplitPos;
			oRightChild.oBounds.m_vecMins[oNode.iSplitAxis] = oNode.flSplitPos;

			oNode.iLeft = aNodes.size()-2;
			oNode.iRight = aNodes.size()-1;

			// Stash there before aiCurrentNode.push_back() invalidates oCurrentIndex
			size_t iFirst = oCurrentIndex.iPointListFirst;
			size_t iSplit = oCurrentIndex.iPointListSplit;
			size_t iLast = oCurrentIndex.iPointListLast;

			aiCurrentNode.push_back();
			CCurrentNode& oRightChildIndex = aiCurrentNode.push_back();
			CCurrentNode& oLeftChildIndex = *(aiCurrentNode.end()-2);

			oLeftChildIndex.iNode = oNode.iLeft;
			oLeftChildIndex.iPointListFirst = iFirst;
			oLeftChildIndex.iPointListLast = iSplit-1;

			oRightChildIndex.iNode = oNode.iRight;
			oRightChildIndex.iPointListFirst = iSplit;
			oRightChildIndex.iPointListLast = iLast;
		}
		else
		{
			oNode.iNumPoints = oCurrentIndex.iPointListLast>=oCurrentIndex.iPointListFirst?(oCurrentIndex.iPointListLast - oCurrentIndex.iPointListFirst + 1):0;
			oNode.iFirstPoint = oCurrentIndex.iPointListFirst;

			aiCurrentNode.pop_back();
		}
	}

	aNodes.set_capacity(aNodes.size());
}

void CPlanetTerrain::CreateShell1VBO()
{
	if (m_iShell1VBO)
		CRenderer::UnloadVertexDataFromGL(m_iShell1VBO);

	m_iShell1VBO = 0;

	// Wouldn't it be cool if some planets were like, cubeworld?
	//int iHighLevels = 0;

	int iHighLevels = 3;

	tvector<CTerrainPoint> avecTerrain;
	DoubleMatrix4x4 mChunkToPlanet;
	size_t iRows = BuildTerrainArray(avecTerrain, mChunkToPlanet, iHighLevels, Vector2D(0, 0), Vector2D(1, 1), Vector(0, 0, 0));

	tvector<float> aflVerts;
	size_t iTriangles = BuildVerts(aflVerts, avecTerrain, iHighLevels, iRows);

	m_iShell1VBO = CRenderer::LoadVertexDataIntoGL(aflVerts.size() * sizeof(float), aflVerts.data());
	m_iShell1VBOSize = aflVerts.size();
}

void CPlanetTerrain::CreateShell2VBO()
{
	if (m_iShell2VBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iShell2VBO);
		CRenderer::UnloadVertexDataFromGL(m_iShell2IBO);
	}

	m_iShell2VBO = 0;

	int iHighLevels = m_pPlanet->LumpDepth();

	tvector<CTerrainPoint> avecTerrain;
	DoubleMatrix4x4 mChunkToPlanet;
	size_t iRows = BuildTerrainArray(avecTerrain, mChunkToPlanet, iHighLevels, Vector2D(0, 0), Vector2D(1, 1), Vector(0, 0, 0));

	tvector<float> aflVerts;
	tvector<unsigned int> aiVerts;
	size_t iTriangles = BuildIndexedVerts(aflVerts, aiVerts, avecTerrain, iHighLevels, iRows);

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	CMutexLocker oLock = m_pPlanet->s_pShell2Generator->GetLock();
	oLock.Lock();
	TAssert(!m_aflShell2Drop.size());
	swap(m_aflShell2Drop, aflVerts);

	// If there's already a shell 2 IBO then we don't need this one.
	if (!m_iShell2IBO)
		swap(m_aiShell2Drop, aiVerts);
}

// This isn't multithreaded but I'll leave the locks in there in case I want to do so later.
void CPlanetTerrain::RebuildShell2Indices()
{
	int iHighLevels = m_pPlanet->LumpDepth();

	tvector<CTerrainCoordinate> aiLumpCoordinates;
	for (size_t i = 0; i < m_pPlanet->m_pLumpManager->GetNumLumps(); i++)
	{
		CTerrainLump* pLump = m_pPlanet->m_pLumpManager->GetLump(i);
		if (!pLump)
			continue;

		if (pLump->IsGeneratingLowRes())
			continue;

		if (m_pPlanet->m_apTerrain[pLump->GetTerrain()] != this)
			continue;

		CTerrainCoordinate& oCoord = aiLumpCoordinates.push_back();
		pLump->GetCoordinates(oCoord.x, oCoord.y);
	}

	size_t iQuadsPerRow = (size_t)pow(2.0f, (float)iHighLevels);

	tvector<unsigned int> aiVerts;
	size_t iTriangles = BuildMeshIndices(aiVerts, aiLumpCoordinates, iQuadsPerRow+1);

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	CMutexLocker oLock = m_pPlanet->s_pShell2Generator->GetLock();
	oLock.Lock();
	swap(m_aiShell2Drop, aiVerts);
}

void CPlanetTerrain::Render(class CRenderingContext* c) const
{
	double flDistance = m_pPlanet->m_vecCharacterLocalOrigin.Length();
	Vector vecPlayerDirection = m_pPlanet->m_vecCharacterLocalOrigin / flDistance;

	float flPlayerDot = vecPlayerDirection.Dot(GetDirection());
	if (flPlayerDot < -0.60f)
		return;

	scale_t eRenderScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eRenderScale < SCALE_KILOMETER)
		return;

	if (eRenderScale > SCALE_GIGAMETER)
		return;

	bool bLowRes = flDistance > 300;

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetNearestPlanet())
	{
		if (flPlayerDot < 0.45f)
			return;
	}

	if (pCharacter->GetApproximateElevation() < 0.0001f)
		return;

	if (!m_bGeneratingShell2 && m_iShell2VBO && !bLowRes)
	{
		c->BeginRenderVertexArray(m_iShell2VBO);
		c->SetPositionBuffer(0u, 10*sizeof(float));
		c->SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
		c->SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
		c->SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
		c->EndRenderVertexArrayIndexed(m_iShell2IBO, m_iShell2IBOSize);
	}
	else
	{
		c->BeginRenderVertexArray(m_iShell1VBO);
		c->SetPositionBuffer(0u, 10*sizeof(float));
		c->SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
		c->SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
		c->SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
		c->EndRenderVertexArray(m_iShell1VBOSize);
	}
}

DoubleVector CPlanetTerrain::GenerateOffset(const DoubleVector2D& vecCoordinate)
{
	DoubleVector2D vecAdjustedCoordinate = vecCoordinate;
	Vector vecDirection = GetDirection();
	if (vecDirection[0] > 0)
		vecAdjustedCoordinate += DoubleVector2D(0, 0);
	else if (vecDirection[0] < 0)
		vecAdjustedCoordinate += DoubleVector2D(2, 0);
	else if (vecDirection[1] > 0)
		vecAdjustedCoordinate += DoubleVector2D(0, 1);
	else if (vecDirection[1] < 0)
		vecAdjustedCoordinate += DoubleVector2D(0, -1);
	else if (vecDirection[2] > 0)
		vecAdjustedCoordinate += DoubleVector2D(3, 0);
	else
		vecAdjustedCoordinate += DoubleVector2D(1, 0);

	DoubleVector vecOffset;

	static double aflFreqFactors[TERRAIN_NOISE_ARRAY_SIZE] =
	{
		15,      // Continents
		1000,
		3000,   // Mountains
		10000,
		30000,  // Large hills
		100000,
		400000,
		1500000, // Rough terrain
	};

	static double aflHeightFactors[TERRAIN_NOISE_ARRAY_SIZE] =
	{
		0.01,
		0.005,
		0.002,
		0.0005,
		0.0002,
		0.00005,
		0.00001,
		0.000002,
	};

	static double aflAlphaFreqFactors[TERRAIN_NOISE_ARRAY_SIZE] =
	{
		0,
		100,
		300,
		1000,
		3000,
		10000,
		30000,
		50000,
	};

	// This algorithm was designed on an Earth-radius planet so it should be scaled for whatever radius the target planet is.
	double flRadiusScalar = (m_pPlanet->GetRadius()/CScalableFloat(6.3781f, SCALE_MEGAMETER)).GetUnits(SCALE_METER);

	for (size_t i = 0; i < TERRAIN_NOISE_ARRAY_SIZE; i++)
	{
		double flXFactor = vecAdjustedCoordinate.x * aflFreqFactors[i];
		double flYFactor = vecAdjustedCoordinate.y * aflFreqFactors[i];
		double x = m_pPlanet->m_aNoiseArray[i][0].Noise(flXFactor, flYFactor) * aflHeightFactors[i] * flRadiusScalar;
		double y = m_pPlanet->m_aNoiseArray[i][1].Noise(flXFactor, flYFactor) * aflHeightFactors[i] * flRadiusScalar;
		double z = m_pPlanet->m_aNoiseArray[i][2].Noise(flXFactor, flYFactor) * aflHeightFactors[i] * flRadiusScalar;

		double a = m_pPlanet->m_aNoiseArray[i][3].Noise(vecAdjustedCoordinate.x * aflAlphaFreqFactors[i], vecAdjustedCoordinate.y * aflAlphaFreqFactors[i]);

		double flScale = RemapValClamped(a, -0.2, 0.4, 0.0, 1.0);
		double flOverdrive = RemapValClamped(a, 0.8, 1.0, 1.0, 1.5);

		vecOffset += DoubleVector(x, y, z) * (flScale*flOverdrive);
	}

	return vecOffset;
}

tvector<CTerrainArea> CPlanetTerrain::FindNearbyAreas(size_t iMaxDepth, size_t iStartDepth, const DoubleVector2D& vecSearchMin, const DoubleVector2D& vecSearchMax, const DoubleVector& vecSearch, double flMaxDistance)
{
	tvector<CTerrainArea> avecAreas;
	avecAreas.reserve(50);

	// Make sure we don't throw out a quad that would have been valid if it the offsets were generated properly.
	// This is compensated for by dividing by two in each recursion of SearchAreas().
	flMaxDistance *= pow(2.0, (double)iMaxDepth - iStartDepth);

	SearchAreas(avecAreas, iMaxDepth, iStartDepth, vecSearchMin, vecSearchMax, m_pPlanet->m_vecCharacterLocalOrigin, flMaxDistance);

	sort_heap(avecAreas.begin(), avecAreas.end(), TerrainAreaCompare);

	return avecAreas;
}

void CPlanetTerrain::SearchAreas(tvector<CTerrainArea>& avecAreas, size_t iMaxDepth, size_t iCurrentDepth, const DoubleVector2D& vecSearchMin, const DoubleVector2D& vecSearchMax, const DoubleVector& vecSearch, double flMaxDistance)
{
	if (iCurrentDepth < 4 && iMaxDepth != iCurrentDepth)
	{
		// Use a cheaper search method that works great for sphere sections.
		DoubleVector2D vecQuad1 = vecSearchMin;
		DoubleVector2D vecQuad2 = DoubleVector2D(vecSearchMin.x, vecSearchMax.y);
		DoubleVector2D vecQuad3 = vecSearchMax;
		DoubleVector2D vecQuad4 = DoubleVector2D(vecSearchMax.x, vecSearchMin.y);

		DoubleVector vecWorld1 = CoordToWorld(vecQuad1);
		DoubleVector vecWorld2 = CoordToWorld(vecQuad2);
		DoubleVector vecWorld3 = CoordToWorld(vecQuad3);
		DoubleVector vecWorld4 = CoordToWorld(vecQuad4);

		DoubleVector vecCenter = ((vecWorld1 + vecWorld2 + vecWorld3 + vecWorld4)/4).Normalized();

		DoubleVector vecSearchNormalized = vecSearch.Normalized();
		DoubleVector vecCornerNormalized = vecWorld1.Normalized();

		double flSearchDot = vecCenter.Dot(vecSearchNormalized);
		double flCornerDot = vecCenter.Dot(vecCornerNormalized) - 0.1; // Nudge a little to accept near cases

		if (flSearchDot < flCornerDot)
			return;
	}
	else
	{
		DoubleVector2D vecQuad1 = vecSearchMin;
		DoubleVector2D vecQuad2 = DoubleVector2D(vecSearchMin.x, vecSearchMax.y);
		DoubleVector2D vecQuad3 = vecSearchMax;
		DoubleVector2D vecQuad4 = DoubleVector2D(vecSearchMax.x, vecSearchMin.y);

		DoubleVector vecWorld1 = CoordToWorld(vecQuad1) + GenerateOffset(vecQuad1);
		DoubleVector vecWorld2 = CoordToWorld(vecQuad2) + GenerateOffset(vecQuad2);
		DoubleVector vecWorld3 = CoordToWorld(vecQuad3) + GenerateOffset(vecQuad3);
		DoubleVector vecWorld4 = CoordToWorld(vecQuad4) + GenerateOffset(vecQuad4);

		DoubleVector vecNormal = (vecWorld2-vecWorld1).Cross(vecWorld3-vecWorld1).Normalized();

		double flDistance = DistanceToQuad(vecSearch, vecWorld1, vecWorld2, vecWorld3, vecWorld4, vecNormal);

		if (flDistance > flMaxDistance)
			return;

		if (iMaxDepth == iCurrentDepth)
		{
			CTerrainArea& vecArea = avecAreas.push_back();
			vecArea.flDistanceToPlayer = flDistance;
			vecArea.vecMin = vecSearchMin;
			vecArea.vecMax = vecSearchMax;
			push_heap(avecAreas.begin(), avecAreas.end(), TerrainAreaCompare);
			return;
		}
	}

	DoubleVector2D vecSearchCenter = (vecSearchMin + vecSearchMax)/2;

	SearchAreas(avecAreas, iMaxDepth, iCurrentDepth+1, vecSearchMin, vecSearchCenter, vecSearch, flMaxDistance/2);
	SearchAreas(avecAreas, iMaxDepth, iCurrentDepth+1, DoubleVector2D(vecSearchMin.x, vecSearchCenter.y), DoubleVector2D(vecSearchCenter.x, vecSearchMax.y), vecSearch, flMaxDistance/2);
	SearchAreas(avecAreas, iMaxDepth, iCurrentDepth+1, vecSearchCenter, vecSearchMax, vecSearch, flMaxDistance/2);
	SearchAreas(avecAreas, iMaxDepth, iCurrentDepth+1, DoubleVector2D(vecSearchCenter.x, vecSearchMin.y), DoubleVector2D(vecSearchMax.x, vecSearchCenter.y), vecSearch, flMaxDistance/2);
}

const DoubleVector CPlanetTerrain::CoordToWorld(const DoubleVector2D& v) const
{
	DoubleVector vecDirection = GetDirection();

	DoubleVector vecCubePoint;

	double x = RemapVal(v.x, 0, 1, -1, 1);
	double y = RemapVal(v.y, 0, 1, -1, 1);

	if (vecDirection[0] > 0)
		vecCubePoint = DoubleVector(vecDirection[0], y, -x);
	else if (vecDirection[0] < 0)
		vecCubePoint = DoubleVector(vecDirection[0], y, x);
	else if (vecDirection[1] > 0)
		vecCubePoint = DoubleVector(-y, vecDirection[1], -x);
	else if (vecDirection[1] < 0)
		vecCubePoint = DoubleVector(y, vecDirection[1], -x);
	else if (vecDirection[2] > 0)
		vecCubePoint = DoubleVector(x, y, vecDirection[2]);
	else
		vecCubePoint = DoubleVector(-x, y, vecDirection[2]);

	return vecCubePoint.Normalized() * m_pPlanet->GetRadius().GetUnits(m_pPlanet->GetScale());
}

CTerrainLOD::~CTerrainLOD()
{
	UnloadAll();
}

void CTerrainLOD::UnloadAll()
{
	for (size_t i = 0; i < LOD_TOTAL; i++)
		CRenderer::UnloadVertexDataFromGL(iLODIBO[i]);

	memset(this, 0, sizeof(CTerrainLOD));
}
