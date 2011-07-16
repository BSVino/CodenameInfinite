#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <tengine/game/baseentity.h>
#include <quadtree.h>

class CBranchData
{
public:
	CBranchData()
	{
		flHeight = 0;
		bRender = false;
		flScreenSize = 1;
		flLastScreenSizeUpdate = -1;
	}

public:
	float flHeight;
	bool bRender;
	float flScreenSize;
	float flLastScreenSizeUpdate;
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

	void						Think();
	void						ThinkBranch(CQuadTreeBranch<CBranchData>* pBranch);

	void						Render(class CRenderingContext* c) const;
	void						RenderBranch(const CQuadTreeBranch<CBranchData>* pBranch, class CRenderingContext* c) const;

	void						UpdateScreenSize(CQuadTreeBranch<CBranchData>* pBranch);
	bool						ShouldRenderBranch(CQuadTreeBranch<CBranchData>* pBranch);

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
};

class CPlanet : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPlanet, CBaseEntity);

public:
								CPlanet();
	virtual						~CPlanet();

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();
	virtual void				RenderUpdate();

	virtual float				GetRenderRadius() const { return m_flRadius; };
	virtual bool				ShouldRender() const { return true; };
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(float flRadius) { m_flRadius = flRadius; }
	float						GetRadius() { return m_flRadius; }

	void						SetAtmosphereThickness(float flAtmosphereThickness) { m_flAtmosphereThickness = flAtmosphereThickness; }
	float						GetAtmosphereThickness() { return m_flAtmosphereThickness; }

	float						GetCloseOrbit();

	void						SetMinutesPerRevolution(float f) { m_flMinutesPerRevolution = f; }

protected:
	float						m_flRadius;
	float						m_flAtmosphereThickness;
	float						m_flMinutesPerRevolution;

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
