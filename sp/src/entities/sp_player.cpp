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
#include "ui/command_menu.h"

REGISTER_ENTITY(CSPPlayer);

NETVAR_TABLE_BEGIN(CSPPlayer);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPPlayer);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, structure_type, m_eConstructionMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, item_t, m_eBlockPlaceMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYARRAY, size_t, m_aiStructures);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPPlayer);
INPUTS_TABLE_END();

CSPPlayer::CSPPlayer()
{
	m_eConstructionMode = STRUCTURE_NONE;
	m_eBlockPlaceMode = ITEM_NONE;
	memset(m_aiStructures, 0, sizeof(m_aiStructures));
	m_pActiveCommandMenu = nullptr;
}

void CSPPlayer::Precache()
{
	PrecacheMaterial("textures/commandmenubutton.mat");
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
		if (GetPlayerCharacter()->GetMoveParent() != GetPlayerCharacter()->GetNearestPlanet())
		{
			TAssert(GetPlayerCharacter()->GetMoveParent()->GetMoveParent() == GetPlayerCharacter()->GetNearestPlanet());
			mPlanetLocalRotation.SetAngles(VectorAngles(GetPlayerCharacter()->GetMoveParent()->GetLocalTransform().TransformVector(AngleVector(GetPlayerCharacter()->GetViewAngles()))));
		}

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

		if (GetPlayerCharacter()->GetMoveParent() == GetPlayerCharacter()->GetNearestPlanet())
			GetPlayerCharacter()->SetViewAngles(mNewGlobalRotation.GetAngles());
		else
			GetPlayerCharacter()->SetViewAngles((Matrix4x4(GetPlayerCharacter()->GetMoveParent()->GetLocalTransform().InvertedRT()) * mNewGlobalRotation).GetAngles());
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
	if (m_pActiveCommandMenu)
	{
		if (m_pActiveCommandMenu->MouseInput(iButton, iState))
			return;
	}

	if (m_eConstructionMode)
	{
		FinishConstruction();
		return;
	}

	if (m_eBlockPlaceMode && iState == 1)
	{
		FinishBlockPlace();
		return;
	}

	if (iButton == TINKER_KEY_MOUSE_LEFT && iState == 1)
	{
		GetPlayerCharacter()->MeleeAttack();
	}

	if (iButton == TINKER_KEY_MOUSE_RIGHT && iState == 1)
	{
		Matrix4x4 mTransform = GetPlayerCharacter()->GetPhysicsTransform();

		Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*GetPlayerCharacter()->EyeHeight();
		Vector vecDirection = GetPlayerCharacter()->GameData().TransformVectorLocalToPhysics(AngleVector(GetPlayerCharacter()->GetViewAngles()));

		CTraceResult tr;
		GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*4, GetPlayerCharacter());

		if (tr.m_flFraction < 1)
		{
			if (tr.m_pHit)
				tr.m_pHit->Use(GetPlayerCharacter());
		}
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
	{
		if (GetPlayerCharacter()->IsFlying())
			Instructor_LessonLearned("f-to-stop-flying");
		else
			Instructor_LessonLearned("f-to-fly");

		GetPlayerCharacter()->ToggleFlying();
	}

	if (c == TINKER_KEY_ESCAPE)
		m_eConstructionMode = STRUCTURE_NONE;

	if (c == TINKER_KEY_ESCAPE)
		m_eBlockPlaceMode = ITEM_NONE;

	if (c == 'W' || c == 'A' || c == 'S' || c == 'D')
		Instructor_LessonLearned("wasd");
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
	return (m_eConstructionMode || m_eBlockPlaceMode) && SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
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
			else if (m_eConstructionMode == STRUCTURE_PALLET)
			{
				c.UseMaterial("textures/pallet.mat");

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

	if (m_eBlockPlaceMode && GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		CScalableVector vecLocal;
		CBaseEntity* pGiveTo;
		if (FindBlockPlacePoint(vecLocal, &pGiveTo))
		{
			CSpire* pSpire = GetPlayerCharacter()->GetNearestSpire();
			if (pGiveTo)
			{
				Vector vecPosition = pGiveTo->GetLocalOrigin() - GetPlayerCharacter()->GetLocalOrigin();
				vecPosition -= Vector(0.25f, 0.25f, 0.25f);

				Vector vecForward = pSpire->GetLocalTransform().GetForwardVector()/2;
				Vector vecUp = pSpire->GetLocalTransform().GetUpVector()/2;
				Vector vecRight = pSpire->GetLocalTransform().GetRightVector()/2;

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.ResetTransformations();
				c.Translate(vecPosition);

				c.UseMaterial("textures/items1.mat");

				c.SetUniform("flAlpha", 0.7f);

				c.SetBlend(BLEND_ADDITIVE);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(Vector(0, 0, 0));
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecRight);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecUp);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecForward);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight);
				c.EndRender();

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight + vecUp + vecForward);
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecUp);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecRight);
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecForward);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecForward + vecUp);
				c.EndRender();
			}
			else if (pSpire)
			{
				CScalableVector vecSpire = pSpire->GetLocalOrigin();

				CVoxelTree* pTree = pSpire->GetVoxelTree();

				IVector vecBlock = pTree->ToVoxelCoordinates(vecLocal);
				CScalableVector vecSBlock = pTree->ToLocalCoordinates(vecBlock);

				Vector vecPosition = vecSBlock - GetPlayerCharacter()->GetLocalOrigin();

#if 0
				{
					Vector vecUp, vecRight;
					GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, &vecUp);
					vecUp *= 0.1f;
					vecRight *= 0.1f;

					CGameRenderingContext c(GameServer()->GetRenderer(), true);

					c.ResetTransformations();
					c.Translate(vecLocal - GetPlayerCharacter()->GetLocalOrigin());

					c.UseMaterial("textures/items1.mat");

					c.SetBlend(BLEND_ADDITIVE);

					c.SetUniform("flAlpha", 0.95f);

					c.BeginRenderTriFan();
						c.TexCoord(0.0f, 1.0f);
						c.Vertex(-vecRight + vecUp);
						c.TexCoord(0.0f, 0.75f);
						c.Vertex(-vecRight - vecUp);
						c.TexCoord(0.25f, 0.75f);
						c.Vertex(vecRight - vecUp);
						c.TexCoord(0.25f, 1.0f);
						c.Vertex(vecRight + vecUp);
					c.EndRender();
				}
#endif

				Vector vecForward = pSpire->GetLocalTransform().GetForwardVector();
				Vector vecUp = pSpire->GetLocalTransform().GetUpVector();
				Vector vecRight = pSpire->GetLocalTransform().GetRightVector();

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.ResetTransformations();
				c.Translate(vecPosition);

				c.UseMaterial("textures/items1.mat");

				c.SetUniform("flAlpha", 0.7f);

				c.SetBlend(BLEND_ADDITIVE);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(Vector(0, 0, 0));
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecRight);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecUp);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecForward);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight);
				c.EndRender();

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight + vecUp + vecForward);
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecUp);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecRight);
					c.TexCoord(0.0f, 0.75f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.25f, 0.75f);
					c.Vertex(vecForward);
					c.TexCoord(0.25f, 1.0f);
					c.Vertex(vecForward + vecUp);
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
	Vector vecDirection = GetPlayerCharacter()->GameData().TransformVectorLocalToPhysics(AngleVector(GetPlayerCharacter()->GetViewAngles()));

	CTraceResult tr;
	GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*8, GetPlayerCharacter());

	if (tr.m_flFraction == 1)
		return false;

	vecLocal = CScalableVector(GetPlayerCharacter()->GameData().TransformPointPhysicsToLocal(tr.m_vecHit), SCALE_METER);

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

			if (pSpire->GameData().GetPlanet() != pNewSpire->GameData().GetPlanet())
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

void CSPPlayer::EnterBlockPlaceMode(item_t eBlock)
{
	if (!GetPlayerCharacter()->GetInventory(eBlock))
		return;

	m_eBlockPlaceMode = eBlock;
}

bool CSPPlayer::FindBlockPlacePoint(CScalableVector& vecLocal, CBaseEntity** pGiveTo) const
{
	Matrix4x4 mTransform = GetPlayerCharacter()->GetPhysicsTransform();

	Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*GetPlayerCharacter()->EyeHeight();
	Vector vecDirection = GetPlayerCharacter()->GameData().TransformVectorLocalToPhysics(AngleVector(GetPlayerCharacter()->GetViewAngles()));
	Vector vecEndpoint = vecEye + vecDirection*6;

	CTraceResult tr;
	GamePhysics()->TraceLine(tr, vecEye, vecEndpoint, GetPlayerCharacter());

	if (tr.m_flFraction == 1)
		return false;

	*pGiveTo = nullptr;

	CStructure* pStructure = dynamic_cast<CStructure*>(tr.m_pHit);
	if (pStructure && pStructure->TakesBlocks())
	{
		if (pStructure->IsUnderConstruction())
			return false;

		*pGiveTo = pStructure;
	}
	else
	{
		CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(tr.m_pHit);
		if (pCharacter && pCharacter->TakesBlocks())
			*pGiveTo = pCharacter;
	}

	Vector vecAdjustedEndpoint = tr.m_vecHit - vecDirection * 0.01f;
	vecLocal = CScalableVector(GetPlayerCharacter()->GameData().TransformPointPhysicsToLocal(vecAdjustedEndpoint), SCALE_METER);

	return true;
}

void CSPPlayer::FinishBlockPlace()
{
	if (!m_eBlockPlaceMode)
		return;

	CScalableVector vecPoint;
	CBaseEntity* pGiveTo;
	if (!FindBlockPlacePoint(vecPoint, &pGiveTo))
		return;

	if (!GetPlayerCharacter()->GetInventory(m_eBlockPlaceMode))
		return;

	if (pGiveTo)
	{
		if (!GetPlayerCharacter()->GiveBlocks(m_eBlockPlaceMode, 1, pGiveTo))
			return;
	}
	else if (!GetPlayerCharacter()->PlaceBlock(m_eBlockPlaceMode, vecPoint))
		return;

	if (!GetPlayerCharacter()->GetInventory(m_eBlockPlaceMode))
		m_eBlockPlaceMode = ITEM_NONE;

	SPWindow()->GetHUD()->BuildMenus();
}

void CSPPlayer::CommandMenuOpened(CCommandMenu* pMenu)
{
	if (m_pActiveCommandMenu)
	{
		CBaseEntity* pOwner = m_pActiveCommandMenu->GetOwner();
		if (pOwner)
			pOwner->GameData().CloseCommandMenu();
	}

	m_pActiveCommandMenu = pMenu;
}

void CSPPlayer::CommandMenuClosed(CCommandMenu* pMenu)
{
	if (m_pActiveCommandMenu != pMenu)
		return;

	m_pActiveCommandMenu = nullptr;
}

void CSPPlayer::AddSpires(size_t iSpires)
{
	m_aiStructures[STRUCTURE_SPIRE] += iSpires;

	SPWindow()->GetHUD()->BuildMenus();
}
