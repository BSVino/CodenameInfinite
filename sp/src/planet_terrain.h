#ifndef SP_PLANET_TERRAIN_H
#define SP_PLANET_TERRAIN_H

#include <tengine/game/entities/baseentity.h>

#include "sp_common.h"

class CTerrainPoint
{
public:
	DoubleVector		vec3DPosition;
	DoubleVector2D		vec2DPosition;
	Vector				vecNormal;
	DoubleVector2D		vecDetail;
	DoubleVector		vecOffset;
};

class CPlanetTerrain
{
	friend class CPlanet;

public:
								CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection);

public:
	void						Think();
	size_t						BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, size_t iDepth, const Vector2D& vecMin, const Vector2D vecMax);
	void						CreateHighLevelsVBO();

	void						Render(class CRenderingContext* c) const;

	DoubleVector				GenerateOffset(const DoubleVector2D& vecCoordinate);

	virtual DoubleVector2D		WorldToCoord(const DoubleVector& vecWorld) const;
	virtual DoubleVector		CoordToWorld(const TemplateVector2D<double>& vecTree) const;

	Vector						GetDirection() const { return m_vecDirection; }
	class CPlanet*				GetPlanet() const { return m_pPlanet; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;

	size_t                      m_iShell1VBO;
	size_t                      m_iShell1VBOSize;
	size_t                      m_iShell2VBO;
	size_t                      m_iShell2VBOSize;
};

#endif
