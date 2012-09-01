#ifndef SP_PLANET_TERRAIN_H
#define SP_PLANET_TERRAIN_H

#include <EASTL/map.h>

#include <quadtree.h>

#include <tengine/game/entities/baseentity.h>

#include "sp_common.h"

class CBranchData
{
public:
	CBranchData()
	{
		bRender = false;
		flScreenSize = 1;
		flLastScreenUpdate = -1;
		flLastPushPull = -1;
		iShouldRenderLastFrame = ~0;
		iLocalCharacterDotLastFrame = ~0;
		iRenderVectorsLastFrame = ~0;
		flRadiusMeters = 0;
		bCompletelyInsideFrustum = false;
		iSplitSides = 0;
	}

public:
	bool				bRender;
	float				flScreenSize;
	CScalableFloat		flGlobalRadius;
	float				flRadiusMeters;
	double              flLastScreenUpdate;
	double              flLastPushPull;
	size_t				iShouldRenderLastFrame;
	bool				bShouldRender;
	size_t				iLocalCharacterDotLastFrame;
	float				flLocalCharacterDot;
	bool				bCompletelyInsideFrustum;
	char				iSplitSides;

	size_t				iRenderVectorsLastFrame;
	CScalableVector		vecGlobalQuadCenter;
	DoubleVector		avecVerts[4];
	Vector				avecNormals[4];
	DoubleVector2D		avecDetails[4];
	DoubleVector		avecOffsets[4];
};

class CTerrainPoint
{
public:
	DoubleVector		vec3DPosition;
	DoubleVector2D		vec2DPosition;
	Vector				vecNormal;
	DoubleVector2D		vecDetail;
	DoubleVector		vecOffset;
};

typedef CQuadTree<CBranchData, double> CTerrainQuadTree;
typedef CQuadTreeBranch<CBranchData, double> CTerrainQuadTreeBranch;
typedef CQuadTreeDataSource<CBranchData, double> CTerrainQuadTreeDataSource;

class CPlanetTerrain : public CTerrainQuadTree, public CTerrainQuadTreeDataSource
{
	friend class CPlanet;

public:
								CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection);

public:
	void						Init();

	void						Think();
	void						ThinkBranch(CTerrainQuadTreeBranch* pBranch);
	bool						ShouldPush(CTerrainQuadTreeBranch* pBranch);
	bool						ShouldPull(CTerrainQuadTreeBranch* pBranch);
	void						BuildBranch(CTerrainQuadTreeBranch* pBranch, bool bForce = false);
	void						BuildBranchToDepth(CTerrainQuadTreeBranch* pBranch, size_t iDepth);
	void						PushBranch(CTerrainQuadTreeBranch* pBranch);
	void						PullBranch(CTerrainQuadTreeBranch* pBranch);
	void						ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch);

	size_t						BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, size_t iDepth, const Vector2D& vecMin, const Vector2D vecMax);
	void						CreateHighLevelsVBO();

	void						Render(class CRenderingContext* c) const;
	void						RenderBranch(const CTerrainQuadTreeBranch* pBranch, class CRenderingContext* c) const;

	void						UpdateScreenSize(CTerrainQuadTreeBranch* pBranch);
	bool						ShouldRenderBranch(CTerrainQuadTreeBranch* pBranch);
	void						InitRenderVectors(CTerrainQuadTreeBranch* pBranch);
	void						CalcRenderVectors(CTerrainQuadTreeBranch* pBranch);
	float						GetLocalCharacterDot(CTerrainQuadTreeBranch* pBranch);

	DoubleVector				GenerateOffset(const DoubleVector2D& vecCoordinate);

	virtual DoubleVector2D		WorldToQuadTree(const CTerrainQuadTree* pTree, const DoubleVector& vecWorld) const;
	virtual DoubleVector		QuadTreeToWorld(const CTerrainQuadTree* pTree, const TemplateVector2D<double>& vecTree) const;
	virtual DoubleVector2D		WorldToQuadTree(CTerrainQuadTree* pTree, const DoubleVector& vecWorld);
	virtual DoubleVector		QuadTreeToWorld(CTerrainQuadTree* pTree, const TemplateVector2D<double>& vecTree);
	virtual DoubleVector		GetBranchCenter(CTerrainQuadTreeBranch* pBranch);
	virtual bool				ShouldBuildBranch(CTerrainQuadTreeBranch* pBranch, bool& bDelete);
	virtual bool				IsLeaf(CTerrainQuadTreeBranch* pBranch);

	Vector						GetDirection() const { return m_vecDirection; }
	class CPlanet*				GetPlanet() const { return m_pPlanet; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;
	int							m_iBuildsThisFrame;
	tmap<scale_t, tvector<CTerrainQuadTreeBranch*> >	m_apRenderBranches;
	bool						m_bOneQuad;

	size_t                      m_iShell1VBO;
	size_t                      m_iShell1VBOSize;
	size_t                      m_iShell2VBO;
	size_t                      m_iShell2VBOSize;
};

#endif
