#ifndef SP_ENTITY_H
#define SP_ENTITY_H

#include <game/entities/baseentity.h>

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

	virtual const Matrix4x4         GetRenderTransform() const;
	virtual const Vector            GetRenderOrigin() const;

	virtual bool					ShouldCollide() const { return true; }
};

#endif
