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

	virtual void				PostRender() const;

	virtual const TFloat        GetBoundingRadius() const { return GetRadius(); };

	void						SetRadius(const CScalableFloat& flRadius) { m_flRadius = flRadius; }
	CScalableFloat				GetRadius() const { return m_flRadius; }

	void						SetLightColor(const Color& clrLight) { m_clrLight = clrLight; }
	Color						GetLightColor() const { return m_clrLight; }

	CScalableFloat				GetCloseOrbit();

	virtual scale_t				GetScale() const { return SCALE_GIGAMETER; }

protected:
	CScalableFloat				m_flRadius;
	Color						m_clrLight;
};

#endif
