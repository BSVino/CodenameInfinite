#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <simplex.h>
#include <geometry.h>

#include <tengine/game/entities/baseentity.h>

#define TERRAIN_NOISE_ARRAY_SIZE 15

class CPlanetTerrain;
class CTerrainLump;

class CPlanet : public CBaseEntity
{
	friend class CPlanetTerrain;
	friend class CTerrainLumpManager;
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

	const DoubleVector&			GetCharacterLocalOrigin() { return m_vecCharacterLocalOrigin; }

	virtual scale_t				GetScale() const { return SCALE_MEGAMETER; }

	const class CTerrainLumpManager* GetTerrainLumpManager() { return m_pTerrainLumpManager; }
	size_t						LumpSize() { return m_iMeterDepth-m_iLumpDepth; };
	size_t						LumpDepth() { return m_iLumpDepth; };
	size_t						PhysicsDepth() { return m_iMeterDepth-7; };

	class CPlanetTerrain*       GetTerrain(size_t iTerrain) { return m_apTerrain[iTerrain]; }

	void						Debug_RebuildTerrain();

protected:
	size_t						m_iRandomSeed;

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

	class CTerrainLumpManager*	m_pTerrainLumpManager;

	// 10 levels deep, 3 channels (x, y, z)
	CSimplexNoise<double>		m_aNoiseArray[TERRAIN_NOISE_ARRAY_SIZE][3];

	static class CParallelizer*	s_pShell2Generator;
};

#endif
