#ifndef SP_ENTITY_H
#define SP_ENTITY_H

#include <tengine/game/baseentity.h>

#include "sp_common.h"

class CSPEntity : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CSPEntity, CBaseEntity);

public:
	virtual bool				ShouldRender() const;

	// For purposes of rendering and physics, units are assumed to be in this scale.
	virtual scale_t				GetScale() const { return SCALE_METER; }
	virtual bool				ShouldRenderAtScale(scale_t eScale) const { return (eScale == GetScale()); };
};

#endif
