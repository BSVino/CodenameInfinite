#ifndef SP_ENTITY_DATA_H
#define SP_ENTITY_DATA_H

#include <game/entities/baseentitydata.h>
#include <game/entityhandle.h>

#include "../sp_common.h"

class CPlanet;
class CSPPlayer;

class CSPEntityData : public CBaseEntityData
{
	DECLARE_CLASS(CSPEntityData, CBaseEntityData);

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
	const Vector                TransformVectorLocalToPhysics(const Vector& v) const;

	void                        SetPlanet(CPlanet* pPlanet);
	CPlanet*                    GetPlanet() const;

	void                        SetPlayerOwner(CSPPlayer* pPlanet);
	CSPPlayer*                  GetPlayerOwner() const;

	void                        SetGroupTransform(const Matrix4x4& m) { m_mGroupTransform = m; }
	const Matrix4x4&            GetGroupTransform() const { return m_mGroupTransform; }

private:
	CEntityHandle<CSPPlayer>    m_hPlayerOwner;
	CEntityHandle<CPlanet>      m_hPlanet;

	// Transform from the center of the nearest chunk group.
	// Use a persistent transform to avoid floating point problems converting back to double all the time.
	Matrix4x4                   m_mGroupTransform;
};

#endif
