#ifndef SP_ENTITY_H
#define SP_ENTITY_H

#include <tengine/game/baseentity.h>

#include "sp_common.h"

class CSPEntity : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CSPEntity, CBaseEntity);

public:
	// For purposes of rendering and physics, units are assumed to be in this scale.
	virtual scale_t					GetScale() const { return SCALE_METER; }
	virtual bool					ShouldRenderAtScale(scale_t eScale) const { return (eScale == GetScale()); };

	virtual CScalableMatrix			GetScalableRenderTransform() const;
	virtual CScalableVector			GetScalableRenderOrigin() const;

	virtual Matrix4x4				GetRenderTransform() const;
	virtual Vector					GetRenderOrigin() const;

	virtual bool					CollideLocal(const TVector& v1, const TVector& v2, CTraceResult& tr);
	virtual bool					Collide(const TVector& v1, const TVector& v2, CTraceResult& tr);

	virtual bool					ShouldCollide() const { return true; }
};

#endif
