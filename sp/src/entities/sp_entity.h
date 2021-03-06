#ifndef SP_ENTITY_DATA_H
#define SP_ENTITY_DATA_H

#include <game/entities/baseentitydata.h>
#include <game/entityhandle.h>

#include "../sp_common.h"
#include "../planet/lumpaddress.h"

class CPlanet;
class CSPPlayer;
class CCommandMenu;

class CSPEntityData : public CBaseEntityData
{
	DECLARE_CLASS(CSPEntityData, CBaseEntityData);

public:
	CSPEntityData();

public:
	// For purposes of rendering and physics, units are assumed to be in this scale.
	virtual scale_t					GetScale() const { return SCALE_METER; }
	virtual bool					ShouldRenderAtScale(scale_t eScale) const;

	virtual CScalableMatrix			GetScalableRenderTransform() const;
	virtual CScalableVector			GetScalableRenderOrigin() const;

	virtual const Matrix4x4         GetRenderTransform() const;
	virtual const Vector            GetRenderOrigin() const;

	const DoubleVector          TransformPointPhysicsToLocal(const Vector& v) const;
	const Vector                TransformPointLocalToPhysics(const DoubleVector& v) const;
	const DoubleVector          TransformVectorPhysicsToLocal(const Vector& v) const;
	const Vector                TransformVectorLocalToPhysics(const Vector& v) const;

	void                        SetPlanet(CPlanet* pPlanet);
	CPlanet*                    GetPlanet() const;

	class CTerrainLump*         GetLump() const;
	void                        SetLump(const CLumpAddress& oAddress) { m_oLump = oAddress; }

	class CVoxelTree*           GetVoxelTree() const;

	void                        SetPlayerOwner(CSPPlayer* pPlanet);
	CSPPlayer*                  GetPlayerOwner() const;

	void                        SetGroupTransform(const Matrix4x4& m) { m_mGroupTransform = m; }
	const Matrix4x4&            GetGroupTransform() const { return m_mGroupTransform; }

	class CCommandMenu*         CreateCommandMenu(class CPlayerCharacter* pRequester);
	void                        CloseCommandMenu();
	class CCommandMenu*         GetCommandMenu() const;
	void                        SetCommandMenuParameters(const Vector& vecRenderOffset, float flProjectionDistance, float flProjectionRadius);
	const Vector&               GetCommandMenuRenderOffset() const { return m_vecCommandMenuRenderOffset; }
	float                       GetCommandMenuProjectionDistance() const { return m_flCommandMenuProjectionDistance; }
	float                       GetCommandMenuProjectionRadius() const { return m_flCommandMenuProjectionRadius; }

private:
	CEntityHandle<CSPPlayer>    m_hPlayerOwner;
	CEntityHandle<CPlanet>      m_hPlanet;

	CLumpAddress                m_oLump;

	// Transform from the center of the nearest chunk group.
	// Use a persistent transform to avoid floating point problems converting back to double all the time.
	Matrix4x4                   m_mGroupTransform;

	CCommandMenu*     m_pCommandMenu;

	Vector            m_vecCommandMenuRenderOffset;
	float             m_flCommandMenuProjectionDistance;
	float             m_flCommandMenuProjectionRadius;
};

#endif
