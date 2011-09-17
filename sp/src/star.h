#ifndef SP_STAR_H
#define SP_STAR_H

#include <tengine/game/baseentity.h>

class CStar : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CStar, CBaseEntity);

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();

	virtual bool				ShouldRender() const { return true; };
	virtual void				PostRender(bool bTransparent) const;

	virtual TFloat				GetBoundingRadius() const { return GetRadius(); };

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
