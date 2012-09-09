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
	Vector              vecPhys;
};

class CShell2GenerationJob
{
public:
	class CPlanetTerrain*	pTerrain;
};

class CTerrainCoordinate
{
public:
	unsigned short  x;
	unsigned short  y;
};

class CPlanetTerrain
{
	friend class CPlanet;

public:
								CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection);

public:
	void						Think();
	static size_t               BuildIndexedVerts(tvector<float>& aflVerts, tvector<unsigned int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows);
	static size_t               BuildIndexedPhysVerts(tvector<float>& aflVerts, tvector<int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows);
	static size_t               BuildMeshIndices(tvector<unsigned int>& aiIndices, const tvector<CTerrainCoordinate>& aiExclude, size_t iLevels, size_t iRows);
	size_t						BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, size_t iDepth, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax, const DoubleVector& vecCenter);
	void						CreateShell1VBO();
	void						CreateShell2VBO();
	void						RebuildShell2Indices();

	void						Render(class CRenderingContext* c) const;

	DoubleVector				GenerateOffset(const DoubleVector2D& vecCoordinate);

	bool                        FindAreaNearestToPlayer(size_t iAreaDepth, const DoubleVector2D& vecSearchMin, const DoubleVector2D& vecSearchMax, DoubleVector2D& vecLumpMin, DoubleVector2D& vecLumpMax);

	virtual DoubleVector		CoordToWorld(const TemplateVector2D<double>& vecTree) const;

	Vector						GetDirection() const { return m_vecDirection; }
	class CPlanet*				GetPlanet() const { return m_pPlanet; }

protected:
	class CPlanet*				m_pPlanet;
	Vector						m_vecDirection;

	size_t                      m_iShell1VBO;
	size_t                      m_iShell1VBOSize;
	size_t                      m_iShell2VBO;
	size_t                      m_iShell2IBO;
	size_t                      m_iShell2IBOSize;

	bool						m_bGeneratingShell2;
	tvector<float>				m_aflShell2Drop;
	tvector<unsigned int>       m_aiShell2Drop;
};

#endif
