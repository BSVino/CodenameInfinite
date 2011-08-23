#ifndef SP_PLANET_H
#define SP_PLANET_H

#include "sp_entity.h"
#include <quadtree.h>

class CBranchData
{
public:
	CBranchData()
	{
		flHeight = 0;
		bRender = false;
		flScreenSize = 1;
		flLastScreenUpdate = -1;
		iShouldRenderLastFrame = ~0;
		iRenderVectorsLastFrame = ~0;
		flRadiusMeters = 0;
	}

public:
	float				flHeight;
	bool				bRender;
	float				flScreenSize;
	CScalableFloat		flGlobalRadius;
	float				flRadiusMeters;
	float				flLastScreenUpdate;
	size_t				iShouldRenderLastFrame;
	bool				bShouldRender;

	size_t				iRenderVectorsLastFrame;
	CScalableVector		vecGlobalQuadCenter;
	DoubleVector		vec1;
	DoubleVector		vec2;
	DoubleVector		vec3;
	DoubleVector		vec4;
	Vector				vec1n;
	Vector				vec2n;
	Vector				vec3n;
	Vector				vec4n;
};

typedef CQuadTree<CBranchData, double> CTerrainQuadTree;
typedef CQuadTreeBranch<CBranchData, double> CTerrainQuadTreeBranch;
typedef CQuadTreeDataSource<CBranchData, double> CTerrainQuadTreeDataSource;

class CPlanetTerrain : public CTerrainQuadTree, public CTerrainQuadTreeDataSource
{
	friend class CPlanet;

public:
	CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection)
		: CTerrainQuadTree()
	{
		m_pPlanet = pPlanet;
		m_vecDirection = vecDirection;
	};

public:
	void						Init();

	void						Think();
	void						ThinkBranch(CTerrainQuadTreeBranch* pBranch);
	void						ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch);

	void						Render(class CRenderingContext* c) const;
	void						RenderBranch(const CTerrainQuadTreeBranch* pBranch, class CRenderingContext* c) const;

	void						UpdateScreenSize(CTerrainQuadTreeBranch* pBranch);
	bool						ShouldRenderBranch(CTerrainQuadTreeBranch* pBranch);
	void						CalcRenderVectors(CTerrainQuadTreeBranch* pBranch);

	virtual TVector2D<double>	WorldToQuadTree(const CTerrainQuadTree* pTree, const DoubleVector& vecWorld) const;
	virtual DoubleVector		QuadTreeToWorld(const CTerrainQuadTree* pTree, const TVector2D<double>& vecTree) const;
	virtual TVector2D<double>	WorldToQuadTree(CTerrainQuadTree* pTree, const DoubleVector& vecWorld);
	virtual DoubleVector		QuadTreeToWorld(CTerrainQuadTree* pTree, const TVector2D<double>& vecTree);
	virtual bool				ShouldBuildBranch(CTerrainQuadTreeBranch* pBranch, bool& bDelete);

	Vector						GetDirection() const { return m_vecDirection; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;
	int							m_iBuildsThisFrame;
	eastl::map<scale_t, eastl::vector<CTerrainQuadTreeBranch*> >	m_apRenderBranches;
	bool						m_bOneQuad;
};

class CPlanet : public CSPEntity
{
	REGISTER_ENTITY_CLASS(CPlanet, CSPEntity);

public:
								CPlanet();
	virtual						~CPlanet();

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();
	virtual void				RenderUpdate();

	virtual bool				ShouldRenderAtScale(scale_t eScale) const;

	virtual CScalableFloat		GetScalableRenderRadius() const;
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(const CScalableFloat& flRadius) { m_flRadius = flRadius; }
	CScalableFloat				GetRadius() const { return m_flRadius; }

	void						SetAtmosphereThickness(const CScalableFloat& flAtmosphereThickness) { m_flAtmosphereThickness = flAtmosphereThickness; }
	const CScalableFloat&		GetAtmosphereThickness() { return m_flAtmosphereThickness; }

	void						SetPlanetName(const tstring& sName) { m_sPlanetName = sName; }
	tstring						GetPlanetName() const { return m_sPlanetName; }

	void						SetAtmosphereColor(const Color& clrAtmosphere) { m_clrAtmosphere = clrAtmosphere; }
	Color						GetAtmosphereColor() const { return m_clrAtmosphere; }

	int							GetMinQuadRenderDepth() { return m_iMinQuadRenderDepth; };

	CScalableFloat				GetCloseOrbit();

	void						SetMinutesPerRevolution(float f) { m_flMinutesPerRevolution = f; }

	virtual scale_t				GetScale() const { return SCALE_MEGAMETER; }

protected:
	CScalableFloat				m_flRadius;
	CScalableFloat				m_flAtmosphereThickness;
	float						m_flMinutesPerRevolution;
	int							m_iMinQuadRenderDepth;

	bool						m_bOneSurface;

	tstring						m_sPlanetName;
	Color						m_clrAtmosphere;

	union
	{
		struct
		{
			CPlanetTerrain*		m_pTerrainFd;
			CPlanetTerrain*		m_pTerrainBk;
			CPlanetTerrain*		m_pTerrainRt;
			CPlanetTerrain*		m_pTerrainLf;
			CPlanetTerrain*		m_pTerrainUp;
			CPlanetTerrain*		m_pTerrainDn;
		};
		CPlanetTerrain*			m_pTerrain[6];
	};
};

#endif
