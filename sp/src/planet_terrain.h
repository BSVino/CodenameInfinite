#ifndef SP_PLANET_TERRAIN_H
#define SP_PLANET_TERRAIN_H

#include <EASTL/map.h>

#include <quadtree.h>

#include <tengine/game/baseentity.h>

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
	float				flLastScreenUpdate;
	float				flLastPushPull;
	size_t				iShouldRenderLastFrame;
	bool				bShouldRender;
	size_t				iLocalCharacterDotLastFrame;
	float				flLocalCharacterDot;
	bool				bCompletelyInsideFrustum;
	char				iSplitSides;

	size_t				iRenderVectorsLastFrame;
	CScalableVector		vecGlobalQuadCenter;
	DoubleVector		vecCenter;
	DoubleVector		vec1;
	DoubleVector		vec2;
	DoubleVector		vec3;
	DoubleVector		vec4;
	Vector				vecCenterNormal;
	Vector				vec1n;
	Vector				vec2n;
	Vector				vec3n;
	Vector				vec4n;
	DoubleVector2D		vecDetailMin;
	DoubleVector2D		vecDetailMax;
	DoubleVector		vecOffsetCenter;
	DoubleVector		vecOffset1;
	DoubleVector		vecOffset2;
	DoubleVector		vecOffset3;
	DoubleVector		vecOffset4;
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
	void						PushBranch(CTerrainQuadTreeBranch* pBranch);
	void						PullBranch(CTerrainQuadTreeBranch* pBranch);
	void						ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch);

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

	virtual bool				CollideLocal(const CBaseEntity* pWith, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr);
	virtual bool				CollideLocalBranch(CTerrainQuadTreeBranch* pBranch, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr);

	virtual bool				Collide(const CBaseEntity* pWith, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr);
	virtual bool				CollideBranch(CTerrainQuadTreeBranch* pBranch, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr);

	Vector						GetDirection() const { return m_vecDirection; }
	class CPlanet*				GetPlanet() const { return m_pPlanet; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;
	int							m_iBuildsThisFrame;
	eastl::map<scale_t, eastl::vector<CTerrainQuadTreeBranch*> >	m_apRenderBranches;
	bool						m_bOneQuad;
};

#endif
