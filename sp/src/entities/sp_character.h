#ifndef SP_CHARACTER_H
#define SP_CHARACTER_H

#include <tengine/game/entities/character.h>

#include "planet/planet.h"

#include "sp_common.h"
#include "sp_entity.h"
#include "sp_player.h"

class CPlanet;

typedef enum {
	FINDPLANET_ANY,
	FINDPLANET_CLOSEORBIT,
	FINDPLANET_INATMOSPHERE,
} findplanet_t;

class CSPCharacter : public CCharacter
{
	REGISTER_ENTITY_CLASS(CSPCharacter, CCharacter);

public:
								CSPCharacter();

public:
	virtual void				Spawn();
	virtual void				Think();

	virtual const CScalableMatrix GetScalableRenderTransform() const;
	virtual const CScalableVector GetScalableRenderOrigin() const;

	virtual bool                ShouldRender() const;
	virtual const Matrix4x4     GetRenderTransform() const;
	virtual const Vector        GetRenderOrigin() const;

	virtual const TMatrix       GetMovementVelocityTransform() const;
	virtual void                CharacterMovement(class btCollisionWorld*, float flDelta);
	virtual const Matrix4x4     GetPhysicsTransform() const;
	virtual void                SetPhysicsTransform(const Matrix4x4& m);
	virtual void                PostSetLocalTransform(const TMatrix& m);
	const Vector                TransformPointPhysicsToLocal(const Vector& v);
	const Vector                TransformVectorLocalToPhysics(const Vector& v);
	virtual void                SetGroundEntity(CBaseEntity* pEntity);
	virtual void                SetGroundEntityExtra(size_t iExtra);

	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE) const;
	CPlanet*					FindNearestPlanet() const;

	CSpire*                     GetNearestSpire() const;
	CSpire*                     FindNearestSpire() const;
	void                        ClearNearestSpire();

	virtual const Vector        GetUpVector() const;
	virtual const Vector        GetLocalUpVector() const;

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual void                EnteredAtmosphere() {};

	void                        SetGroupTransform(const Matrix4x4& m) { m_mGroupTransform = m; }

	virtual const TFloat        GetBoundingRadius() const { return 2.0f; };
	virtual CScalableFloat		EyeHeight() const;
	virtual TFloat				JumpStrength();
	virtual CScalableFloat		CharacterSpeed();
	virtual double              GetLastEnteredAtmosphere() const { return m_flLastEnteredAtmosphere; }
	virtual float               GetAtmosphereLerpTime() const { return 1; }
	virtual float               GetAtmosphereLerp() const { return 0.3f; }
	virtual bool                ApplyGravity() const { return true; }

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
	double                      m_flNextPlanetCheck;

	mutable CNetworkedHandle<CSpire>    m_hNearestSpire;
	mutable double                      m_flNextSpireCheck;

	double                      m_flLastEnteredAtmosphere;
	float						m_flRollFromSpace;

	// Transform from the center of the nearest chunk group.
	// Use a persistent transform to avoid floating point problems converting back to double all the time.
	Matrix4x4                   m_mGroupTransform;
};

#endif
