#ifndef SP_ENTITY_DATA_H
#define SP_ENTITY_DATA_H

#include <tengine/game/baseentitydata.h>

#include "sp_common.h"

class CSPEntityData : public CBaseEntityData
{
	DECLARE_CLASS(CSPEntityData, CBaseEntityData);

public:
	// For purposes of rendering and physics, units are assumed to be in this scale.
	virtual scale_t					GetScale() const { return SCALE_METER; }
	virtual bool					ShouldRenderAtScale(scale_t eScale) const;

	virtual CScalableMatrix			GetScalableRenderTransform() const;
	virtual CScalableVector			GetScalableRenderOrigin() const;

	virtual Matrix4x4				GetRenderTransform() const;
	virtual Vector					GetRenderOrigin() const;

	virtual bool					CollideLocal(const class CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);
	virtual bool					Collide(const class CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr);
};

#endif
