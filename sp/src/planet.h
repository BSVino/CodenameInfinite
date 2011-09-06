#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <simplex.h>

#include "sp_entity.h"

#define TERRAIN_NOISE_ARRAY_SIZE 15

class CPlanetTerrain;

class CPlanet : public CSPEntity
{
	friend class CPlanetTerrain;

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

	virtual CScalableFloat		GetRenderRadius() const;
	virtual void				PostRender(bool bTransparent) const;

	virtual TFloat				GetBoundingRadius() const { return GetRadius(); };

	void						SetRandomSeed(size_t iSeed);

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

	const DoubleVector&			GetCharacterLocalOrigin() { return s_vecCharacterLocalOrigin; }

	virtual scale_t				GetScale() const { return SCALE_MEGAMETER; }

protected:
	size_t						m_iRandomSeed;

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

	static DoubleVector			s_vecCharacterLocalOrigin;

	// 10 levels deep, 3 channels (x, y, z)
	CSimplexNoise<double>		m_aNoiseArray[TERRAIN_NOISE_ARRAY_SIZE][3];
};

#endif
