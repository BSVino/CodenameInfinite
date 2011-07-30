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
		bRendered = false;
		flScreenSize = 1;
		flLastScreenUpdate = -1;
		bRenderVectorsDirty = true;
	}

public:
	float	flHeight;
	bool	bRender;
	bool	bRendered;
	float	flScreenSize;
	CScalableFloat	flQuadDistance;
	CScalableFloat	flQuadRadius;
	float	flLastScreenUpdate;

	bool	bRenderVectorsDirty;
	Vector	vec1;
	Vector	vec2;
	Vector	vec3;
	Vector	vec4;
	Vector	vec1n;
	Vector	vec2n;
	Vector	vec3n;
	Vector	vec4n;
};

class CPlanetTerrain : public CQuadTree<CBranchData>, public CQuadTreeDataSource<CBranchData>
{
public:
	CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection)
		: CQuadTree<CBranchData>()
	{
		m_pPlanet = pPlanet;
		m_vecDirection = vecDirection;
	};

public:
	void						Init();

	void						ResetRenderFlags(CQuadTreeBranch<CBranchData>* pBranch = NULL);

	void						Think();
	void						ThinkBranch(CQuadTreeBranch<CBranchData>* pBranch);
	void						ProcessBranchRendering(CQuadTreeBranch<CBranchData>* pBranch);

	void						Render(class CRenderingContext* c) const;
	void						RenderBranch(const CQuadTreeBranch<CBranchData>* pBranch, class CRenderingContext* c) const;

	void						UpdateScreenSize(CQuadTreeBranch<CBranchData>* pBranch);
	bool						ShouldRenderBranch(CQuadTreeBranch<CBranchData>* pBranch);
	void						CalcRenderVectors(CQuadTreeBranch<CBranchData>* pBranch);

	virtual Vector2D			WorldToQuadTree(const CQuadTree<CBranchData>* pTree, const Vector& vecWorld) const;
	virtual Vector				QuadTreeToWorld(const CQuadTree<CBranchData>* pTree, const Vector2D& vecTree) const;
	virtual Vector2D			WorldToQuadTree(CQuadTree<CBranchData>* pTree, const Vector& vecWorld);
	virtual Vector				QuadTreeToWorld(CQuadTree<CBranchData>* pTree, const Vector2D& vecTree);
	virtual bool				ShouldBuildBranch(CQuadTreeBranch<CBranchData>* pBranch, bool& bDelete);

	Vector						GetDirection() const { return m_vecDirection; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;
	int							m_iBuildsThisFrame;
	eastl::vector<CQuadTreeBranch<CBranchData>*>	m_apRenderBranches;
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

	virtual bool				ShouldRenderAtScale(scale_t eScale) const { return true; };

	virtual CScalableFloat		GetScalableRenderRadius() const;
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(const CScalableFloat& flRadius) { m_flRadius = flRadius; }
	CScalableFloat				GetRadius() const { return m_flRadius; }

	void						SetAtmosphereThickness(const CScalableFloat& flAtmosphereThickness) { m_flAtmosphereThickness = flAtmosphereThickness; }
	const CScalableFloat&		GetAtmosphereThickness() { return m_flAtmosphereThickness; }

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
