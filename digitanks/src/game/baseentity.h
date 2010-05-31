#ifndef DT_BASEENTITY_H
#define DT_BASEENTITY_H

#include <map>
#include <vector>
#include <vector.h>
#include <assert.h>

#include "entityhandle.h"

typedef void (*EntityRegisterCallback)();

class CBaseEntity
{
	friend class CGame;

public:
											CBaseEntity();
											~CBaseEntity();

public:
	virtual void							Precache() {};

	void									SetModel(const wchar_t* pszModel);
	size_t									GetModel() { return m_iModel; };

	virtual Vector							GetRenderOrigin() const { return GetOrigin(); };
	virtual EAngle							GetRenderAngles() const { return GetAngles(); };

	Vector									GetOrigin() const { return m_vecOrigin; };
	void									SetOrigin(const Vector& vecOrigin) { m_vecOrigin = vecOrigin; };

	Vector									GetLastOrigin() const { return m_vecLastOrigin; };
	void									SetLastOrigin(const Vector& vecOrigin) { m_vecLastOrigin = vecOrigin; };

	Vector									GetVelocity() const { return m_vecVelocity; };
	void									SetVelocity(const Vector& vecVelocity) { m_vecVelocity = vecVelocity; };

	EAngle									GetAngles() const { return m_angAngles; };
	void									SetAngles(const EAngle& angAngles) { m_angAngles = angAngles; };

	Vector									GetGravity() const { return m_vecGravity; };
	void									SetGravity(Vector vecGravity) { m_vecGravity = vecGravity; };

	bool									GetSimulated() { return m_bSimulated; };
	void									SetSimulated(bool bSimulated) { m_bSimulated = bSimulated; };

	size_t									GetHandle() const { return m_iHandle; }

	virtual float							GetTotalHealth() { return m_flTotalHealth; }
	virtual float							GetHealth() { return m_flHealth; }
	virtual bool							IsAlive() { return m_flHealth > 0; }

	virtual void							TakeDamage(CBaseEntity* pAttacker, CBaseEntity* pInflictor, float flDamage);
	void									Killed();
	virtual void							OnKilled() {};

	virtual void							PreRender() {};
	virtual void							ModifyContext(class CRenderingContext* pContext) {};
	void									Render();
	virtual void							OnRender() {};
	virtual void							PostRender() {};

	void									Delete();
	virtual void							OnDeleted() {};
	bool									IsDeleted() { return m_bDeleted; }
	void									SetDeleted() { m_bDeleted = true; }

	virtual void							Think() {};

	virtual bool							ShouldTouch(CBaseEntity* pOther) const { return false; };
	virtual bool							IsTouching(CBaseEntity* pOther, Vector& vecPoint) const { return false; };
	virtual void							Touching(CBaseEntity* pOther) {};

	virtual int								GetCollisionGroup() { return m_iCollisionGroup; }
	virtual void							SetCollisionGroup(int iCollisionGroup) { m_iCollisionGroup = iCollisionGroup; }

	static CBaseEntity*						GetEntity(size_t iHandle);
	static size_t							GetEntityHandle(size_t i);
	static CBaseEntity*						GetEntityNumber(size_t i);
	static size_t							GetNumEntities();

	static void								PrecacheModel(const wchar_t* pszModel, bool bStatic = true);
	static void								PrecacheParticleSystem(const wchar_t* pszSystem);

	static void								RegisterEntity(EntityRegisterCallback pfnCallback);
	static void								RegisterEntity(CBaseEntity* pEntity);

protected:
	Vector									m_vecOrigin;
	Vector									m_vecLastOrigin;
	EAngle									m_angAngles;
	Vector									m_vecVelocity;
	Vector									m_vecGravity;

	bool									m_bSimulated;

	size_t									m_iHandle;

	bool									m_bTakeDamage;
	float									m_flTotalHealth;
	float									m_flHealth;

	bool									m_bDeleted;

	std::vector<CEntityHandle<CBaseEntity> >	m_ahTouching;

	int										m_iCollisionGroup;

	size_t									m_iModel;

private:
	static std::map<size_t, CBaseEntity*>	s_apEntityList;
	static size_t							s_iNextEntityListIndex;

	static std::vector<EntityRegisterCallback>	s_apfnEntityRegisterCallbacks;
};

#define REGISTER_ENTITY(entity) \
void RegisterCallback##entity() \
{ \
	entity* pEntity = new entity(); \
	CBaseEntity::RegisterEntity(pEntity); \
	delete pEntity; \
} \
 \
class CRegister##entity \
{ \
public: \
	CRegister##entity() \
	{ \
		CBaseEntity::RegisterEntity(&RegisterCallback##entity); \
	} \
} g_Register##entity = CRegister##entity(); \

#endif