#ifndef SP_STAR_H
#define SP_STAR_H

#include "sp_entity.h"

class CStar : public CSPEntity
{
	REGISTER_ENTITY_CLASS(CStar, CSPEntity);

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();

	virtual CScalableFloat		GetRenderScalableRadius() const { return m_flRadius; };
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(const CScalableFloat& flRadius) { m_flRadius = flRadius; }
	CScalableFloat				GetRadius() const { return m_flRadius; }

	CScalableFloat				GetCloseOrbit();

	virtual scale_t				GetScale() const { return SCALE_GIGAMETER; }

protected:
	CScalableFloat				m_flRadius;
};

#endif
