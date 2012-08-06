#ifndef SP_PLANET_H
#define SP_PLANET_H

#include "sp_entity.h"

class CPlanetTerrain;

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
	void                        RenderUpdate();

	virtual bool				ShouldRenderAtScale(scale_t eScale) const;

	virtual const CScalableFloat GetRenderRadius() const;
	virtual void				PostRender() const;

	virtual const TFloat        GetBoundingRadius() const { return GetRadius(); };

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
};

#endif
