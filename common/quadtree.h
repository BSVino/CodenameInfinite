#ifndef LW_QUADTREE_H
#define LW_QUADTREE_H

#include <EASTL/vector.h>

#include <vector.h>

template <class T> class CQuadTree;
template <class T> class CQuadTreeBranch;

template <class T>
class CQuadTreeDataSource
{
public:
	// Quad trees aren't always flat, they're curvy and wonky sometimes. This function should translate a 3d coordinate to a quadtree coordinate
	// The output should be ([0, 1], [0, 1])
	// If the point is not in the quad tree then return a value less than 0 or greater than 1 for x or y
	virtual Vector2D			WorldToQuadTree(const CQuadTree<T>* pTree, const Vector& vecWorld) const=0;
	// This is the opposite conversion from the above.
	virtual Vector				QuadTreeToWorld(const CQuadTree<T>* pTree, const Vector2D& vecTree) const=0;

	virtual Vector2D			WorldToQuadTree(CQuadTree<T>* pTree, const Vector& vecWorld)=0;
	virtual Vector				QuadTreeToWorld(CQuadTree<T>* pTree, const Vector2D& vecTree)=0;

	virtual bool				ShouldBuildBranch(CQuadTreeBranch<T>* pBranch, bool& bDelete)=0;
};

template <class T>
class CQuadTreeBranch
{
public:
	CQuadTreeBranch(CQuadTreeDataSource<T>* pSource, CQuadTreeBranch<T>* pParent, CQuadTree<T>* pTree, Vector2D vecMin, Vector2D vecMax, unsigned short iDepth, const T& oData = T())
	{
		m_pDataSource = pSource;
		m_pParent = pParent;
		m_pTree = pTree;

		m_pBranches[0] = NULL;
		m_pBranches[1] = NULL;
		m_pBranches[2] = NULL;
		m_pBranches[3] = NULL;

		m_vecMin = vecMin;
		m_vecMax = vecMax;

		m_iDepth = iDepth;

		m_oData = oData;

		m_bCenterCalculated = false;
	}

public:
	void						BuildBranch(bool bAndChildren = true);

	void						InitPathfinding();
	void						FindNeighbors(const CQuadTreeBranch* pLeaf, eastl::vector<CQuadTreeBranch*>& apNeighbors);
	CQuadTreeBranch<T>*			FindLeaf(const Vector& vecPoint);
	void						SetGScore(float flGScore);
	float						GetFScore();
	Vector						GetCenter();
	Vector						GetCenter() const;

	void						DebugRender();

public:
	CQuadTreeDataSource<T>*		m_pDataSource;
	CQuadTreeBranch<T>*			m_pParent;
	CQuadTree<T>*				m_pTree;

	Vector2D					m_vecMin;
	Vector2D					m_vecMax;

	unsigned short				m_iDepth;

	union
	{
		struct
		{
			CQuadTreeBranch<T>*	m_pBranchxy;
			CQuadTreeBranch<T>*	m_pBranchxY;
			CQuadTreeBranch<T>*	m_pBranchXy;
			CQuadTreeBranch<T>*	m_pBranchXY;
		};
		CQuadTreeBranch<T>*		m_pBranches[4];
	};

	T							m_oData;

	// Pathfinding stuff. SO not thread-safe!
	bool						m_bClosed;
	bool						m_bOpen;
	bool						m_bHCalculated;
	bool						m_bFValid;
	float						m_flGScore;
	float						m_flHScore;
	float						m_flFScore;
	bool						m_bCenterCalculated;
	Vector						m_vecCenter;

	CQuadTreeBranch<T>*			m_pPathParent;
};

template <class T>
class CQuadTree
{
public:
	CQuadTree()
	{
		m_pDataSource = NULL;
		m_pQuadTreeHead = NULL;
	};
	
	CQuadTree(CQuadTreeDataSource<T>* pSource, const T& oData)
	{
		Init(pSource, oData);
	};

public:
	// NOTE: There's no pathfinding code because this code was ripped from a previous project and the next project didn't need pathfinding
	// If you want pathfinding code you'll need to port it from terrain.cpp in the digitanks project

	void						Init(CQuadTreeDataSource<T>* pSource, const T& oData);

	class CQuadTreeBranch<T>*	FindLeaf(const Vector& vecPoint);
	void						FindNeighbors(const CQuadTreeBranch<T>* pLeaf, eastl::vector<CQuadTreeBranch<T>*>& apNeighbors);

protected:
	CQuadTreeDataSource<T>*		m_pDataSource;
	class CQuadTreeBranch<T>*	m_pQuadTreeHead;
};

#include <EASTL/list.h>
#include <EASTL/heap.h>

template <class T>
class LowestF
{
public:
	bool operator() (const CQuadTreeBranch<T>* pLeft, const CQuadTreeBranch<T>* pRight)
	{
		return const_cast<CQuadTreeBranch<T>*>(pLeft)->GetFScore() > const_cast<CQuadTreeBranch<T>*>(pRight)->GetFScore();
	}
};

template <class T>
void CQuadTree<T>::Init(CQuadTreeDataSource<T>* pSource, const T& oData)
{
	m_pDataSource = pSource;
	m_pQuadTreeHead = new CQuadTreeBranch<T>(pSource, NULL, this, Vector2D(0, 0), Vector2D(1, 1), 0, oData);
}

template <class T>
CQuadTreeBranch<T>* CQuadTree<T>::FindLeaf(const Vector& vecPoint)
{
	if (!m_pQuadTreeHead)
		return NULL;

	if (!m_pDataSource)
		return NULL;

	Vector2D vecQuadTreePoint = m_pDataSource->WorldToQuadTree(vecPoint);

	if (vecQuadTreePoint.x < 0)
		return NULL;

	if (vecQuadTreePoint.y < 0)
		return NULL;

	if (vecQuadTreePoint.x > 1)
		return NULL;

	if (vecQuadTreePoint.y > 1)
		return NULL;

	CQuadTreeBranch<T>* pCurrent = m_pQuadTreeHead;
	while (pCurrent->m_pBranches[0])
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (vecQuadTreePoint.x < pCurrent->m_pBranches[i]->m_vecMin.x)
				continue;

			if (vecQuadTreePoint.y < pCurrent->m_pBranches[i]->m_vecMin.y)
				continue;

			if (vecQuadTreePoint.x > pCurrent->m_pBranches[i]->m_vecMax.x)
				continue;

			if (vecQuadTreePoint.y > pCurrent->m_pBranches[i]->m_vecMax.y)
				continue;

			pCurrent = pCurrent->m_pBranches[i];
			break;
		}
	}

	return pCurrent;
}

template <class T>
void CQuadTree<T>::FindNeighbors(const CQuadTreeBranch<T>* pLeaf, eastl::vector<CQuadTreeBranch<T>*>& apNeighbors)
{
	if (!pLeaf)
		return;

	apNeighbors.set_capacity(20);
	m_pQuadTreeHead->FindNeighbors(pLeaf, apNeighbors);
}

template <class T>
void CQuadTreeBranch<T>::BuildBranch(bool bAndChildren)
{
	bool bDelete;
	bool bBuildBranch = m_pDataSource->ShouldBuildBranch(this, bDelete);

	if (bBuildBranch)
	{
		if (!m_pBranches[0])
		{
			float flSize = (m_vecMax.x - m_vecMin.x)/2;
			m_pBranchxy = new CQuadTreeBranch<T>(m_pDataSource, this, m_pTree, m_vecMin + Vector2D(0, 0), m_vecMin + Vector2D(flSize, flSize), m_iDepth+1);
			m_pBranchxY = new CQuadTreeBranch<T>(m_pDataSource, this, m_pTree, m_vecMin + Vector2D(0, flSize), m_vecMin + Vector2D(flSize, flSize+flSize), m_iDepth+1);
			m_pBranchXy = new CQuadTreeBranch<T>(m_pDataSource, this, m_pTree, m_vecMin + Vector2D(flSize, 0), m_vecMin + Vector2D(flSize+flSize, flSize), m_iDepth+1);
			m_pBranchXY = new CQuadTreeBranch<T>(m_pDataSource, this, m_pTree, m_vecMin + Vector2D(flSize, flSize), m_vecMin + Vector2D(flSize+flSize, flSize+flSize), m_iDepth+1);
		}

		if (bAndChildren)
		{
			for (size_t i = 0; i < 4; i++)
				m_pBranches[i]->BuildBranch();
		}
	}
	else
	{
		if (m_pBranches[0] && bDelete)
		{
			for (size_t i = 0; i < 4; i++)
			{
				delete m_pBranches[i];
				m_pBranches[i] = NULL;
			}
		}
	}
}

template <class T>
void CQuadTreeBranch<T>::InitPathfinding()
{
	if (m_pBranches[0])
	{
		for (size_t i = 0; i < 4; i++)
			m_pBranches[i]->InitPathfinding();
	}
	else
	{
		m_bClosed = false;
		m_bOpen = false;
		m_bFValid = false;
		m_bHCalculated = false;
		m_flGScore = 0;
		m_pPathParent = NULL;
		m_bCenterCalculated = false;	// Re-calculated centers every pathfind in case of terrain height changes.
	}
}

template <class T>
void CQuadTreeBranch<T>::FindNeighbors(const CQuadTreeBranch<T>* pLeaf, eastl::vector<CQuadTreeBranch<T>*>& apNeighbors)
{
	if (!pLeaf)
		return;

	if (m_pBranches[0])
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (pLeaf->m_vecMin.x > m_pBranches[i]->m_vecMax.x)
				continue;

			if (pLeaf->m_vecMin.y > m_pBranches[i]->m_vecMax.y)
				continue;

			if (pLeaf->m_vecMax.x < m_pBranches[i]->m_vecMin.x)
				continue;

			if (pLeaf->m_vecMax.y < m_pBranches[i]->m_vecMin.y)
				continue;

			m_pBranches[i]->FindNeighbors(pLeaf, apNeighbors);
		}
	}
	else
	{
		if (pLeaf->m_vecMin.x > m_vecMax.x)
			return;

		if (pLeaf->m_vecMin.y > m_vecMax.y)
			return;

		if (pLeaf->m_vecMax.x < m_vecMin.x)
			return;

		if (pLeaf->m_vecMax.y < m_vecMin.y)
			return;

		apNeighbors.push_back(this);
	}
}

template <class T>
CQuadTreeBranch<T>* CQuadTreeBranch<T>::FindLeaf(const Vector& vecPoint)
{
	Vector2D vecQuadTreePoint = m_pDataSource->WorldToQuadTree(vecPoint);

	if (vecQuadTreePoint.x < m_vecMin.x)
		return NULL;

	if (vecQuadTreePoint.y < m_vecMin.y)
		return NULL;

	if (vecQuadTreePoint.x > m_vecMax.x)
		return NULL;

	if (vecQuadTreePoint.y > m_vecMax.y)
		return NULL;

	if (!m_pBranches[0])
		return this;

	for (size_t i = 0; i < 4; i++)
	{
		CQuadTreeBranch<T>* pResult = m_pBranches[i]->FindLeaf(vecPoint);
		if (pResult)
			return pResult;
	}

	TAssert(!"Should never get here.");
	return NULL;
}

template <class T>
void CQuadTreeBranch<T>::SetGScore(float flGScore)
{
	m_bFValid = false;
	m_flGScore = flGScore;
}

template <class T>
float CQuadTreeBranch<T>::GetFScore()
{
	if (m_bFValid)
		return m_flFScore;

	if (!m_bHCalculated)
	{
		m_flHScore = m_pTerrain->WeightedLeafDistance(this, m_pTerrain->m_pPathEnd, true);
		m_bHCalculated = true;
	}

	m_flFScore = m_flHScore + m_flGScore;
	m_bFValid = true;

	return m_flFScore;
}

template <class T>
Vector CQuadTreeBranch<T>::GetCenter()
{
	if (!m_bCenterCalculated)
	{
		m_vecCenter = (m_pDataSource->QuadTreeToWorld(m_pTree, m_vecMin) + m_pDataSource->QuadTreeToWorld(m_pTree, m_vecMax))/2;
		m_bCenterCalculated = true;
	}

	return m_vecCenter;
}

template <class T>
Vector CQuadTreeBranch<T>::GetCenter() const
{
	TAssert(m_bCenterCalculated);

	if (!m_bCenterCalculated)
		return (m_pDataSource->QuadTreeToWorld(m_pTree, m_vecMin) + m_pDataSource->QuadTreeToWorld(m_pTree, m_vecMax))/2;

	return m_vecCenter;
}

template <class T>
void CQuadTreeBranch<T>::DebugRender()
{
	if (m_pBranches[0])
	{
		for (size_t i = 0; i < 4; i++)
			m_pBranches[i]->DebugRender();
	}
	else
	{
		glBegin(GL_LINE_STRIP);
			glVertex3f(m_pTerrain->ArrayToWorldSpace(m_vecMin.x), m_pTerrain->GetRealHeight(m_vecMin.x, m_vecMin.y)+1, m_pTerrain->ArrayToWorldSpace(m_vecMin.y));
			glVertex3f(m_pTerrain->ArrayToWorldSpace(m_vecMax.x), m_pTerrain->GetRealHeight(m_vecMax.x, m_vecMin.y)+1, m_pTerrain->ArrayToWorldSpace(m_vecMin.y));
			glVertex3f(m_pTerrain->ArrayToWorldSpace(m_vecMax.x), m_pTerrain->GetRealHeight(m_vecMax.x, m_vecMax.y)+1, m_pTerrain->ArrayToWorldSpace(m_vecMax.y));
		glEnd();
	}
}

#endif
