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

	virtual float				GetRenderRadius() const { return m_flRadius; };
	virtual bool				ShouldRender() const { return true; };
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(float flRadius) { m_flRadius = flRadius; }
	float						GetRadius() const { return m_flRadius; }

	float						GetCloseOrbit();

protected:
	float						m_flRadius;
};

#endif
