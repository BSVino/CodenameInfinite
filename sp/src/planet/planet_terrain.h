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

class CTerrainArea
{
public:
	DoubleVector2D    vecMin;
	DoubleVector2D    vecMax;
	double            flDistanceToSearchPoint;
};

inline bool TerrainAreaCompare(const CTerrainArea& l, const CTerrainArea& r)
{
	return l.flDistanceToSearchPoint < r.flDistanceToSearchPoint;
}

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

class CKDPointTreePoint
{
public:
	DoubleVector        vec3DPosition;
	DoubleVector2D      vec2DMin;
	DoubleVector2D      vec2DMax;
};

class CKDPointTreeNode
{
public:
	CKDPointTreeNode()
	{
		iParent = iLeft = iRight = ~0;
		iNumPoints = 0;
	}

public:
	size_t              iParent;    // Indices into node vector
	size_t              iLeft;
	size_t              iRight;

	size_t              iFirstPoint;    // Index into point vector
	size_t              iNumPoints;

	AABB                oBounds;

	size_t              iSplitAxis;
	float               flSplitPos;
};

extern tvector<CTerrainArea> SearchKDTree(const tvector<CKDPointTreeNode>& aNodes, const tvector<CKDPointTreePoint>& aPoints, const DoubleVector& vecNearestToPoint, double flDistanceSqr, float flScaleResult);

typedef enum
{
	LOD_HIGH,
	LOD_MED,
	LOD_LOW,
	LOD_TOTAL,
} lod_type;

class CTerrainLOD
{
public:
	CTerrainLOD()
	{
		memset(this, 0, sizeof(CTerrainLOD));
	}

	~CTerrainLOD();

public:
	void         UnloadAll();

public:
	DoubleVector vecCenter;
	size_t       iLODIBO[LOD_TOTAL], iLODIBOSize[LOD_TOTAL];
};

class CTerrainLODDrop
{
public:
	size_t                 x, y;
	lod_type               m_eType;

	tvector<unsigned int>  m_aiDrop;
	size_t                 m_iSize;
};

class CPlanetTerrain
{
	friend class CPlanet;

public:
								CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection);

public:
	void						Think();
	static size_t               BuildIndexedVerts(tvector<float>& aflVerts, tvector<unsigned int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows, bool bSkirt = false);
	static size_t               BuildVertsForIndex(tvector<float>& aflVerts, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows, bool bSkirt = false);
	static size_t               BuildIndexedPhysVerts(tvector<float>& aflVerts, tvector<int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows);
	static size_t               BuildMeshIndices(tvector<unsigned int>& aiIndices, const tvector<CTerrainCoordinate>& aiExclude, size_t iRows, bool bSkirt = false);
	static size_t               BuildMeshIndices(tvector<unsigned int>& aiIndices, const tvector<CTerrainCoordinate>& aiExclude, size_t iX, size_t iY, size_t iStep, size_t iRowsToIndex, size_t iRowsTotal, bool bSkirt = false);
	static void                 BuildKDTree(tvector<CKDPointTreeNode>& aNodes, tvector<CKDPointTreePoint>& aPoints, const tvector<CTerrainPoint>& avecTerrain, size_t iRows, bool bPhysicsCenter, float flScale3DPosition = 1);
	size_t                      BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, DoubleMatrix4x4& mPlanetToChunk, size_t iDepth, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax, const DoubleVector& vecCenter, bool bSkirt = false);
	static Vector               GetTerrainPointNormal(const tvector<CTerrainPoint>& avecTerrain, size_t x, size_t y, size_t iVertsPerRow);
	void						CreateShell1VBO();
	void						CreateShell2VBO();
	void						RebuildShell2Indices();

	void						Render(class CRenderingContext* c) const;

	DoubleVector				GenerateOffset(const DoubleVector2D& vecCoordinate);
	float                       GenerateTrees(const DoubleVector2D& vecCoordinate);

	tvector<CTerrainArea>       FindNearbyAreas(size_t iMaxDepth, size_t iStartDepth, const DoubleVector2D& vecSearchMin, const DoubleVector2D& vecSearchMax, const DoubleVector& vecSearch, double flMaxDistance);
	void                        SearchAreas(tvector<CTerrainArea>& avecAreas, size_t iMaxDepth, size_t iCurrentDepth, const DoubleVector2D& vecSearchMin, const DoubleVector2D& vecSearchMax, const DoubleVector& vecSearch, double flMaxDistance);

	const DoubleVector          CoordToWorld(const DoubleVector2D& vecTree) const;

	Vector						GetDirection() const { return m_vecDirection; }
	class CPlanet*				GetPlanet() const { return m_pPlanet; }

	bool                        IsShell2Done() const { return !!m_iShell2IBO; }

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

	bool                        m_bKDTreeAvailable;
	tvector<CKDPointTreeNode>   m_aKDNodes;
	tvector<CKDPointTreePoint>  m_aKDPoints;
};

#endif
