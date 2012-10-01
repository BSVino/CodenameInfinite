#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/keys.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "sp_window.h"
#include "ui/hud.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "structures/structure.h"
#include "structures/spire.h"

REGISTER_ENTITY(CSPPlayer);

NETVAR_TABLE_BEGIN(CSPPlayer);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPPlayer);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, structure_type, m_eConstructionMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYARRAY, size_t, m_aiStructures);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPPlayer);
INPUTS_TABLE_END();

CSPPlayer::CSPPlayer()
{
	m_eConstructionMode = STRUCTURE_NONE;
	memset(m_aiStructures, 0, sizeof(m_aiStructures));
}

void CSPPlayer::MouseMotion(int x, int y)
{
	if (m_hCharacter == NULL)
		return;

	float flSensitivity = CVar::GetCVarFloat("m_sensitivity");
	float flYaw = (x*flSensitivity/20);
	float flPitch = (y*flSensitivity/20);

	if (GetPlayerCharacter()->GetNearestPlanet())
	{
		Matrix4x4 mPlanetLocalRotation(GetPlayerCharacter()->GetViewAngles());

		// Construct a "local space" for the surface
		Vector vecSurfaceUp = GetPlayerCharacter()->GetLocalUpVector();
		Vector vecSurfaceForward(1, 0, 0);
		Vector vecSurfaceRight = vecSurfaceForward.Cross(vecSurfaceUp).Normalized();
		vecSurfaceForward = vecSurfaceUp.Cross(vecSurfaceRight).Normalized();

		Matrix4x4 mSurface(vecSurfaceForward, vecSurfaceUp, vecSurfaceRight);
		Matrix4x4 mSurfaceInverse = mSurface;
		mSurfaceInverse.InvertRT();

		// Bring our current view angles into that local space
		Matrix4x4 mLocalRotation = mSurfaceInverse * mPlanetLocalRotation;
		EAngle angLocalRotation = mLocalRotation.GetAngles();

		angLocalRotation.y += flYaw;
		angLocalRotation.p -= flPitch;
		// Don't lock this here since it will snap as we're rolling in from space. LockViewToPlanet() keeps us level.
		//angLocalRotation.r = 0;

		if (angLocalRotation.p > 89)
			angLocalRotation.p = 89;

		if (angLocalRotation.p < -89)
			angLocalRotation.p = -89;

		while (angLocalRotation.y > 180)
			angLocalRotation.y -= 360;

		while (angLocalRotation.y < -180)
			angLocalRotation.y += 360;

		Matrix4x4 mNewLocalRotation;
		mNewLocalRotation.SetAngles(angLocalRotation);

		Matrix4x4 mNewGlobalRotation = mSurface * mNewLocalRotation;

		GetPlayerCharacter()->SetViewAngles(mNewGlobalRotation.GetAngles());
	}
	else
	{
		EAngle angMouse;
		angMouse.p = -flPitch;
		angMouse.y = flYaw;
		angMouse.r = 0;

		CScalableMatrix mRotate;
		mRotate.SetAngles(angMouse);

		CScalableMatrix mTransform(GetPlayerCharacter()->GetViewAngles());

		CScalableMatrix mNewTransform = mTransform * mRotate;

		GetPlayerCharacter()->SetViewAngles(mNewTransform.GetAngles());
	}
}

void CSPPlayer::MouseInput(int iButton, int iState)
{
	if (m_eConstructionMode)
	{
		FinishConstruction();
		return;
	}

	BaseClass::MouseInput(iButton, iState);
}

void CSPPlayer::KeyPress(int c)
{
	BaseClass::KeyPress(c);

	if (GetPlayerCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetPlayerCharacter()->EngageHyperdrive();

	if (c == TINKER_KEY_LCTRL)
		GetPlayerCharacter()->SetWalkSpeedOverride(true);

	if (c == 'F')
		GetPlayerCharacter()->ToggleFlying();

	if (c == TINKER_KEY_ESCAPE)
		m_eConstructionMode = STRUCTURE_NONE;
}

void CSPPlayer::KeyRelease(int c)
{
	BaseClass::KeyRelease(c);

	if (GetPlayerCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetPlayerCharacter()->DisengageHyperdrive();

	if (c == TINKER_KEY_LCTRL)
		GetPlayerCharacter()->SetWalkSpeedOverride(false);
}

CPlayerCharacter* CSPPlayer::GetPlayerCharacter() const
{
	return static_cast<CPlayerCharacter*>(m_hCharacter.GetPointer());
}

bool CSPPlayer::ShouldRender() const
{
	return m_eConstructionMode && SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CSPPlayer::PostRender() const
{
	BaseClass::PostRender();

	if (m_eConstructionMode && GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		float flRadius = GetPhysBoundingBox().Size().Length()/2;

		CScalableVector vecLocal;
		if (FindConstructionPoint(vecLocal))
		{
			Vector vecUp, vecRight;
			GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
			vecUp = GetPlayerCharacter()->GetLocalTransform().GetUpVector();

			Vector vecPosition = vecLocal - GetPlayerCharacter()->GetLocalOrigin();

			CGameRenderingContext c(GameServer()->GetRenderer(), true);

			c.ResetTransformations();
			c.Translate(vecPosition);

			c.SetBlend(BLEND_ADDITIVE);

			if (m_eConstructionMode == STRUCTURE_SPIRE)
			{
				c.UseMaterial("textures/spire.mat");

				c.SetUniform("flAlpha", 0.5f);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(-vecRight + vecUp * 15);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(-vecRight - vecUp);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight - vecUp);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp * 15);
				c.EndRender();
			}
			else if (m_eConstructionMode == STRUCTURE_MINE)
			{
				c.UseMaterial("textures/mine.mat");

				c.SetUniform("flAlpha", 0.5f);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(-vecRight + vecUp * 1.8f);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(-vecRight - vecUp * 0.2f);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight - vecUp * 0.2f);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp * 1.8f);
				c.EndRender();
			}
		}
	}
}

void CSPPlayer::EnterConstructionMode(structure_type eStructure)
{
	if (eStructure == STRUCTURE_SPIRE && !m_aiStructures[eStructure])
		return;

	m_eConstructionMode = eStructure;
}

bool CSPPlayer::FindConstructionPoint(CScalableVector& vecLocal) const
{
	Matrix4x4 mTransform = GetPlayerCharacter()->GetPhysicsTransform();

	Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*GetPlayerCharacter()->EyeHeight();
	Vector vecDirection = GetPlayerCharacter()->TransformVectorLocalToPhysics(AngleVector(GetPlayerCharacter()->GetViewAngles()));

	CTraceResult tr;
	GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*8, GetPlayerCharacter());

	if (tr.m_flFraction == 1)
		return false;

	vecLocal = GetPlayerCharacter()->TransformPointPhysicsToLocal(tr.m_vecHit);

	return true;
}

void CSPPlayer::FinishConstruction()
{
	if (!m_eConstructionMode)
		return;

	CScalableVector vecPoint;
	if (!FindConstructionPoint(vecPoint))
		return;

	CStructure* pStructure = CStructure::CreateStructure(m_eConstructionMode, this, GetPlayerCharacter()->GetNearestSpire(), vecPoint);

	if (m_eConstructionMode == STRUCTURE_SPIRE)
	{
		TAssert(m_aiStructures[m_eConstructionMode]);
		m_aiStructures[m_eConstructionMode]--;

		GetPlayerCharacter()->ClearNearestSpire();

		CSpire* pNewSpire = static_cast<CSpire*>(pStructure);
		size_t iSpires = 0;
		for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
		{
			CBaseEntity* pEnt = CBaseEntity::GetEntity(i);
			if (!pEnt)
				continue;

			CSpire* pSpire = dynamic_cast<CSpire*>(pEnt);
			if (!pSpire)
				continue;

			if (pSpire->GetMoveParent() != pNewSpire->GetMoveParent())
				continue;

			if (pSpire == pNewSpire)
				continue;

			iSpires++;
		}

		// Let's find a good name.
		tstring asDefaultNames[] =
		{
			"Alpha",
			"Beta",
			"Gamma",
			"Delta",
			"Epsilon",
			"Zeta",
			"Iota",
			"Lambda",
			"Omicron",
			"Sigma",
			"Tau",
			"Psi",
			"Omega",
		};

		pNewSpire->SetBaseName(asDefaultNames[iSpires%(sizeof(asDefaultNames)/sizeof(asDefaultNames[0]))]);
	}

	m_eConstructionMode = STRUCTURE_NONE;

	SPWindow()->GetHUD()->BuildMenus();
}

void CSPPlayer::AddSpires(size_t iSpires)
{
	m_aiStructures[STRUCTURE_SPIRE] += iSpires;

	SPWindow()->GetHUD()->BuildMenus();
}
