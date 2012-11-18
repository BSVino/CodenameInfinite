#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <simplex.h>
#include <geometry.h>

#include <tengine/game/entities/baseentity.h>

#define TERRAIN_NOISE_ARRAY_SIZE 8

class CPlanetTerrain;
class CTerrainLump;
class CStar;

class CPlanet : public CBaseEntity
{
	friend class CPlanetTerrain;
	friend class CTerrainLumpManager;
	friend class CTerrainLump;
	friend class CTerrainChunkManager;
	REGISTER_ENTITY_CLASS(CPlanet, CBaseEntity);

public:
								CPlanet();
	virtual						~CPlanet();

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();
	void                        RenderUpdate();

	virtual bool                ShouldRender() const { return true; }
	virtual bool				ShouldRenderAtScale(scale_t eScale) const;

	virtual const CScalableFloat GetRenderRadius() const;
	virtual void				PostRender() const;

	virtual const TFloat        GetBoundingRadius() const { return GetRadius(); };

	float                       GetSunHeight(const TVector& vecGlobal) const; // 1 for noon, -1 for midnight, 0 for dawn/dusk
	float                       GetTimeOfDay(const TVector& vecGlobal) const; // In radians, negative answers are night: 0 for dawn, pi/2 for noon, +/-pi for dusk, -pi/2 for midnight

	void						SetRandomSeed(size_t iSeed);

	void                        GetApprox2DPosition(const DoubleVector& vec3DLocal, size_t& iTerrain, DoubleVector2D& vec2DCoord);
	bool                        FindApproximateElevation(const DoubleVector& vec3DLocal, float& flElevation) const;

	bool                        IsExtraPhysicsEntGround(size_t iEnt) const;

	void                        SetStar(class CStar* pStar);
	class CStar*                GetStar() const;

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

	const DoubleVector&			GetCharacterLocalOrigin() { return m_vecCharacterLocalOrigin; }

	virtual scale_t				GetScale() const { return SCALE_MEGAMETER; }

	const class CTerrainLumpManager*  GetLumpManager() const { return m_pLumpManager; }
	const class CTerrainChunkManager* GetChunkManager() const { return m_pChunkManager; }
	class CTreeManager*               GetTreeManager() { return m_pTreeManager; }

	size_t						LumpDepth() { return m_iLumpDepth; };      // The depth at which lumps first appear.
	size_t						ChunkDepth() { return m_iMeterDepth-7; };  // The depth at which chunks first appear.
	size_t						MeterDepth() { return m_iMeterDepth; };    // The deepest depth.

	class CPlanetTerrain*       GetTerrain(size_t iTerrain) { return m_apTerrain[iTerrain]; }

	void						Debug_RebuildTerrain();

protected:
	size_t						m_iRandomSeed;

	CEntityHandle<CStar>        m_hStar;

	CScalableFloat				m_flRadius;
	CScalableFloat				m_flAtmosphereThickness;
	float						m_flMinutesPerRevolution;
	int							m_iMinQuadRenderDepth;
	int							m_iLumpDepth;
	int							m_iMeterDepth;

	DoubleVector				m_vecCharacterLocalOrigin;

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
		CPlanetTerrain*         m_apTerrain[6];
	};

	class CTerrainLumpManager*  m_pLumpManager;
	class CTerrainChunkManager* m_pChunkManager;
	class CTreeManager*         m_pTreeManager;

	// 3 channels (x, y, z), one alpha for each channel for variation
	CSimplexNoise<double>       m_aNoiseArray[TERRAIN_NOISE_ARRAY_SIZE][4];
	CSimplexNoise<double>       m_aTreeDensity;

	static class CParallelizer*	s_pShell2Generator;
};

#endif
