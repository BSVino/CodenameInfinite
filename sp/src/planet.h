#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <simplex.h>
#include <geometry.h>

#include <tengine/game/baseentity.h>

#define TERRAIN_NOISE_ARRAY_SIZE 15

class CPlanetTerrain;

template <class T, class F>
class COctree;
template <class T, class F>
class COctreeBranch;
template <class T, class F>
class CQuadTreeBranch;
class CBranchData;
class CTerrainChunk;

class CChunkOrQuad
{
public:
	CChunkOrQuad()
	{
		m_pPlanet = NULL;
		m_iChunk = ~0;
		m_pQuad = NULL;
	}

	CChunkOrQuad(class CPlanet* pPlanet, size_t iChunk)
	{
		m_pPlanet = pPlanet;
		m_iChunk = iChunk;
		m_pQuad = NULL;
	}

	CChunkOrQuad(class CPlanet* pPlanet, CQuadTreeBranch<CBranchData, double>* pQuad)
	{
		m_pPlanet = pPlanet;
		m_iChunk = ~0;
		m_pQuad = pQuad;
	}

public:
	bool Raytrace(const DoubleVector& vecStart, const DoubleVector& vecEnd, CCollisionResult& tr);

	bool operator == (const CChunkOrQuad& r)
	{
		if (m_iChunk != ~0 && m_iChunk == r.m_iChunk)
			return true;

		if (m_pQuad && m_pQuad == r.m_pQuad)
			return true;

		return false;
	}

public:
	class CPlanet*							m_pPlanet;
	size_t									m_iChunk;
	CQuadTreeBranch<CBranchData, double>*	m_pQuad;
};

inline bool operator < (const CChunkOrQuad& l, const CChunkOrQuad& r)
{
	if (l.m_iChunk != ~0 && r.m_iChunk != ~0)
		return l.m_iChunk < r.m_iChunk;

	if (l.m_pQuad && r.m_pQuad)
		return l.m_pQuad < r.m_pQuad;

	else if (l.m_iChunk != ~0 && r.m_pQuad)
		return true;

	else if (l.m_pQuad && r.m_iChunk != ~0)
		return false;

	TAssert(false);
	return false;
}

class CPlanet : public CBaseEntity
{
	friend class CPlanetTerrain;
	friend class CTerrainChunkManager;
	friend class CChunkOrQuad;
	REGISTER_ENTITY_CLASS(CPlanet, CBaseEntity);

public:
								CPlanet();
	virtual						~CPlanet();

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();
	virtual void				RenderUpdate();

	virtual bool				ShouldRender() const { return true; };
	virtual bool				ShouldRenderAtScale(scale_t eScale) const;

	virtual CScalableFloat		GetRenderRadius() const;
	virtual void				PostRender(bool bTransparent) const;

	virtual bool				CollideLocal(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);
	virtual bool				CollideLocalAccurate(const CBaseEntity* pWith, bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr);

	virtual bool				Collide(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);
	virtual bool				CollideAccurate(const CBaseEntity* pWith, bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr);

	virtual bool				ShouldCollide() const { return true; }

	virtual TFloat				GetBoundingRadius() const { return GetRadius(); };

	void						SetRandomSeed(size_t iSeed);

	void						SetRadius(const CScalableFloat& flRadius);
	CScalableFloat				GetRadius() const { return m_flRadius; }

	void						SetAtmosphereThickness(const CScalableFloat& flAtmosphereThickness) { m_flAtmosphereThickness = flAtmosphereThickness; }
	const CScalableFloat&		GetAtmosphereThickness() const { return m_flAtmosphereThickness; }

	void						SetPlanetName(const tstring& sName) { m_sPlanetName = sName; }
	tstring						GetPlanetName() const { return m_sPlanetName; }

	void						SetAtmosphereColor(const Color& clrAtmosphere) { m_clrAtmosphere = clrAtmosphere; }
	Color						GetAtmosphereColor() const { return m_clrAtmosphere; }

	int							GetMinQuadRenderDepth() { return m_iMinQuadRenderDepth; };

	CScalableFloat				GetCloseOrbit();

	void						SetMinutesPerRevolution(float f) { m_flMinutesPerRevolution = f; }

	const DoubleVector&			GetCharacterLocalOrigin() { return s_vecCharacterLocalOrigin; }

	virtual scale_t				GetScale() const { return SCALE_MEGAMETER; }

	void						Debug_RebuildTerrain();
	void						Debug_RenderOctree(const COctreeBranch<CChunkOrQuad, double>* pBranch) const;
	void						Debug_RenderCollision(const COctreeBranch<CChunkOrQuad, double>* pBranch) const;

protected:
	size_t						m_iRandomSeed;

	CScalableFloat				m_flRadius;
	CScalableFloat				m_flAtmosphereThickness;
	float						m_flMinutesPerRevolution;
	int							m_iMinQuadRenderDepth;
	int							m_iChunkDepth;

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

	class CTerrainChunkManager*	m_pTerrainChunkManager;
	COctree<CChunkOrQuad, double>* m_pOctree;

	// 10 levels deep, 3 channels (x, y, z)
	CSimplexNoise<double>		m_aNoiseArray[TERRAIN_NOISE_ARRAY_SIZE][3];

	static DoubleVector			s_vecCharacterLocalOrigin;
};

#endif
