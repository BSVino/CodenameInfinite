#ifndef SP_PLANET_H
#define SP_PLANET_H

#include <tengine/game/baseentity.h>

class CPlanet : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPlanet, CBaseEntity);

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				Think();

	virtual float				GetRenderRadius() const { return m_flRadius; };
	virtual bool				ShouldRender() const { return true; };
	virtual void				ModifyContext(class CRenderingContext* pContext, bool bTransparent) const;
	virtual void				PostRender(bool bTransparent) const;

	void						SetRadius(float flRadius) { m_flRadius = flRadius; }
	float						GetRadius() { return m_flRadius; }

protected:
	float						m_flRadius;
};

#endif
