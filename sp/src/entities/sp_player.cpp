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
#include "items/disassembler.h"
#include "planet/terrain_lumps.h"
#include "voxel/voxel_tree.h"

REGISTER_ENTITY(CSPPlayer);

NETVAR_TABLE_BEGIN(CSPPlayer);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPPlayer);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, structure_type, m_eConstructionMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, item_t, m_eBlockPlaceMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, item_t, m_eBlockDesignateMode);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, int, m_iBlockDesignateDimension);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, IVector, m_vecBlockDesignateMin);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, IVector, m_vecBlockDesignateMax);
	SAVEDATA_OMIT(m_pActiveCommandMenu);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPPlayer);
INPUTS_TABLE_END();

CSPPlayer::CSPPlayer()
{
	m_eConstructionMode = STRUCTURE_NONE;
	m_eBlockPlaceMode = ITEM_NONE;
	m_eBlockDesignateMode = ITEM_NONE;
	m_pActiveCommandMenu = nullptr;
}

void CSPPlayer::Precache()
{
	PrecacheMaterial("textures/commandmenubutton.mat");
	PrecacheMaterial("textures/commandmenunotice.mat");
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
	if (iButton == TINKER_KEY_MOUSE_LEFT && iState == 1 && m_pActiveCommandMenu)
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

	if (m_eBlockDesignateMode && iState == 1)
	{
		FinishBlockDesignate();
		return;
	}

	if (iButton == TINKER_KEY_MOUSE_LEFT)
	{
		CBaseWeapon* pWeapon = GetPlayerCharacter()->GetEquippedWeapon();
		if (pWeapon)
		{
			pWeapon->OwnerMouseInput(iButton, iState);
		}
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
	{
		GetPlayerCharacter()->EngageHyperdrive();
		if (!GetPlayerCharacter()->GameData().GetPlanet())
			Instructor_LessonLearned("shift-hyperspace");
	}

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

	if (c == TINKER_KEY_ESCAPE)
		m_eBlockDesignateMode = ITEM_NONE;

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
	return (m_eConstructionMode || m_eBlockPlaceMode || m_eBlockDesignateMode) && SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
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
					c.Vertex(-vecRight + vecUp * 15/4);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(-vecRight - vecUp/4);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight - vecUp/4);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp * 15/4);
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
			else if (m_eConstructionMode == STRUCTURE_STOVE)
			{
				c.UseMaterial("textures/stove.mat");

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
			else
				TAssert(false);
		}
	}

	if (m_eBlockPlaceMode && GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		CScalableVector vecLocal;
		CBaseEntity* pGiveTo;
		if (FindBlockPlacePoint(vecLocal, &pGiveTo))
		{
			CVoxelTree* pTree = GetPlayerCharacter()->GameData().GetVoxelTree();

			if (pGiveTo)
			{
				Vector vecPosition = pGiveTo->GetLocalOrigin() - GetPlayerCharacter()->GetLocalOrigin();
				vecPosition -= Vector(0.25f, 0.25f, 0.25f);

				Vector vecForward = pGiveTo->GetLocalTransform().GetForwardVector()/2;
				Vector vecUp = pGiveTo->GetLocalTransform().GetUpVector()/2;
				Vector vecRight = pGiveTo->GetLocalTransform().GetRightVector()/2;

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.ResetTransformations();
				c.Translate(vecPosition);

				c.UseMaterial(GetItemMaterial(m_eBlockPlaceMode));

				c.SetUniform("flAlpha", 0.7f);

				c.SetBlend(BLEND_ADDITIVE);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(Vector(0, 0, 0));
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecUp);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecForward);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight);
				c.EndRender();

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight + vecUp + vecForward);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecUp);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecForward);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecForward + vecUp);
				c.EndRender();
			}
			else if (pTree)
			{
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

					c.UseMaterial(GetItemMaterial(m_eBlockPlaceMode));

					c.SetBlend(BLEND_ADDITIVE);

					c.SetUniform("flAlpha", 0.95f);

					c.BeginRenderTriFan();
						c.TexCoord(0.0f, 1.0f);
						c.Vertex(-vecRight + vecUp);
						c.TexCoord(0.0f, 0.0f);
						c.Vertex(-vecRight - vecUp);
						c.TexCoord(1.0f, 0.0f);
						c.Vertex(vecRight - vecUp);
						c.TexCoord(1.0f, 1.0f);
						c.Vertex(vecRight + vecUp);
					c.EndRender();
				}
#endif

				Vector vecForward = pTree->GetTreeToPlanet().GetForwardVector();
				Vector vecUp = pTree->GetTreeToPlanet().GetUpVector();
				Vector vecRight = pTree->GetTreeToPlanet().GetRightVector();

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.ResetTransformations();
				c.Translate(vecPosition);

				c.UseMaterial(GetItemMaterial(m_eBlockPlaceMode));

				c.SetUniform("flAlpha", 0.7f);

				c.SetBlend(BLEND_ADDITIVE);

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(Vector(0, 0, 0));
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecUp);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecForward);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight);
				c.EndRender();

				c.BeginRenderTriFan();
					c.TexCoord(0.0f, 1.0f);
					c.Vertex(vecRight + vecUp + vecForward);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(vecUp + vecForward);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecUp);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecRight + vecUp);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecRight);
					c.TexCoord(0.0f, 0.0f);
					c.Vertex(vecForward + vecRight);
					c.TexCoord(1.0f, 0.0f);
					c.Vertex(vecForward);
					c.TexCoord(1.0f, 1.0f);
					c.Vertex(vecForward + vecUp);
				c.EndRender();
			}
		}
	}

	if (m_eBlockDesignateMode && GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		CVoxelTree* pTree = GetPlayerCharacter()->GameData().GetVoxelTree();

		if (pTree)
		{
			CScalableVector vecLocal;
			bool bFoundPlacePoint = FindBlockPlacePoint(vecLocal);

			if (m_iBlockDesignateDimension < 0 || !bFoundPlacePoint)
			{
				IVector vecBlock = pTree->ToVoxelCoordinates(vecLocal);
				if (m_iBlockDesignateDimension >= 0)
					vecBlock = m_vecBlockDesignateMin;

				CScalableVector vecSBlock = pTree->ToLocalCoordinates(vecBlock);

				Vector vecPosition = vecSBlock - GetPlayerCharacter()->GetLocalOrigin();

				Vector vecForward = pTree->GetTreeToPlanet().GetForwardVector();
				Vector vecUp = pTree->GetTreeToPlanet().GetUpVector();
				Vector vecRight = pTree->GetTreeToPlanet().GetRightVector();

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.ResetTransformations();
				c.Translate(vecPosition);

				c.UseMaterial(GetItemMaterial(m_eBlockDesignateMode));

				c.SetUniform("flAlpha", 0.5f);

				c.SetBlend(BLEND_ADDITIVE);

				c.BeginRenderTriFan();
				c.TexCoord(0.0f, 0.0f);
				c.Vertex(Vector(0, 0, 0));
				c.TexCoord(1.0f, 0.0f);
				c.Vertex(vecRight);
				c.TexCoord(1.0f, 1.0f);
				c.Vertex(vecRight + vecUp);
				c.TexCoord(0.0f, 1.0f);
				c.Vertex(vecUp);
				c.TexCoord(1.0f, 1.0f);
				c.Vertex(vecUp + vecForward);
				c.TexCoord(1.0f, 0.0f);
				c.Vertex(vecForward);
				c.TexCoord(1.0f, 1.0f);
				c.Vertex(vecForward + vecRight);
				c.TexCoord(0.0f, 1.0f);
				c.Vertex(vecRight);
				c.EndRender();

				c.BeginRenderTriFan();
				c.TexCoord(0.0f, 1.0f);
				c.Vertex(vecRight + vecUp + vecForward);
				c.TexCoord(0.0f, 0.0f);
				c.Vertex(vecUp + vecForward);
				c.TexCoord(1.0f, 0.0f);
				c.Vertex(vecUp);
				c.TexCoord(1.0f, 1.0f);
				c.Vertex(vecRight + vecUp);
				c.TexCoord(1.0f, 0.0f);
				c.Vertex(vecRight);
				c.TexCoord(0.0f, 0.0f);
				c.Vertex(vecForward + vecRight);
				c.TexCoord(1.0f, 0.0f);
				c.Vertex(vecForward);
				c.TexCoord(1.0f, 1.0f);
				c.Vertex(vecForward + vecUp);
				c.EndRender();
			}
			else
			{
				IVector vecNew = pTree->ToVoxelCoordinates(vecLocal);

				IVector vecMin = m_vecBlockDesignateMin;
				IVector vecMax = m_vecBlockDesignateMax;

				if (vecNew.x < vecMin.x)
					vecMin.x = vecNew.x;
				if (vecNew.y < vecMin.y)
					vecMin.y = vecNew.y;
				if (vecNew.z < vecMin.z)
					vecMin.z = vecNew.z;
				if (vecNew.x > vecMax.x)
					vecMax.x = vecNew.x;
				if (vecNew.y > vecMax.y)
					vecMax.y = vecNew.y;
				if (vecNew.z > vecMax.z)
					vecMax.z = vecNew.z;

				Vector vecForward = pTree->GetTreeToPlanet().GetForwardVector();
				Vector vecUp = pTree->GetTreeToPlanet().GetUpVector();
				Vector vecRight = pTree->GetTreeToPlanet().GetRightVector();

				CGameRenderingContext c(GameServer()->GetRenderer(), true);

				c.UseMaterial(GetItemMaterial(m_eBlockDesignateMode));

				c.SetUniform("flAlpha", 0.5f);

				c.SetBlend(BLEND_ADDITIVE);

				for (int x = 0; x <= vecMax.x - vecMin.x; x++)
				{
					for (int y = 0; y <= vecMax.y - vecMin.y; y++)
					{
						for (int z = 0; z <= vecMax.z - vecMin.z; z++)
						{
							c.ResetTransformations();

							IVector vecBlock = vecMin + IVector(x, y, z);
							c.Translate(pTree->ToLocalCoordinates(vecBlock) - GetPlayerCharacter()->GetLocalOrigin());

							c.BeginRenderTriFan();
							c.TexCoord(0.0f, 0.0f);
							c.Vertex(Vector(0, 0, 0));
							c.TexCoord(1.0f, 0.0f);
							c.Vertex(vecRight);
							c.TexCoord(1.0f, 1.0f);
							c.Vertex(vecRight + vecUp);
							c.TexCoord(0.0f, 1.0f);
							c.Vertex(vecUp);
							c.TexCoord(1.0f, 1.0f);
							c.Vertex(vecUp + vecForward);
							c.TexCoord(1.0f, 0.0f);
							c.Vertex(vecForward);
							c.TexCoord(1.0f, 1.0f);
							c.Vertex(vecForward + vecRight);
							c.TexCoord(0.0f, 1.0f);
							c.Vertex(vecRight);
							c.EndRender();

							c.BeginRenderTriFan();
							c.TexCoord(0.0f, 1.0f);
							c.Vertex(vecRight + vecUp + vecForward);
							c.TexCoord(0.0f, 0.0f);
							c.Vertex(vecUp + vecForward);
							c.TexCoord(1.0f, 0.0f);
							c.Vertex(vecUp);
							c.TexCoord(1.0f, 1.0f);
							c.Vertex(vecRight + vecUp);
							c.TexCoord(1.0f, 0.0f);
							c.Vertex(vecRight);
							c.TexCoord(0.0f, 0.0f);
							c.Vertex(vecForward + vecRight);
							c.TexCoord(1.0f, 0.0f);
							c.Vertex(vecForward);
							c.TexCoord(1.0f, 1.0f);
							c.Vertex(vecForward + vecUp);
							c.EndRender();
						}
					}
				}
			}
		}
	}
}

void CSPPlayer::EnterConstructionMode(structure_type eStructure)
{
	if (!GetPlayerCharacter()->CanBuildStructure(eStructure))
		return;

	m_eBlockPlaceMode = ITEM_NONE;
	m_eBlockDesignateMode = ITEM_NONE;
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

	if (!GetPlayerCharacter()->CanBuildStructure(m_eConstructionMode))
		return;

	CStructure* pStructure = CStructure::CreateStructure(m_eConstructionMode, this, GetPlayerCharacter()->GetNearestSpire(), vecPoint);

	if (!pStructure)
		return;

	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
		GetPlayerCharacter()->RemoveBlocks((item_t)i, SPGame()->StructureCost(m_eConstructionMode, (item_t)i));

	if (m_eConstructionMode == STRUCTURE_SPIRE)
	{
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

	m_eConstructionMode = STRUCTURE_NONE;
	m_eBlockDesignateMode = ITEM_NONE;
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

	if (pGiveTo)
		*pGiveTo = nullptr;

	CStructure* pStructure = dynamic_cast<CStructure*>(tr.m_pHit);
	if (pStructure && pStructure->TakesBlocks())
	{
		if (pStructure->IsUnderConstruction())
			return false;

		if (pGiveTo)
			*pGiveTo = pStructure;
	}
	else
	{
		CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(tr.m_pHit);
		if (pGiveTo && pCharacter && pCharacter->TakesBlocks())
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

void CSPPlayer::EnterBlockDesignateMode(item_t eBlock)
{
	m_eConstructionMode = STRUCTURE_NONE;
	m_eBlockPlaceMode = ITEM_NONE;
	m_eBlockDesignateMode = eBlock;
	m_iBlockDesignateDimension = -1;
	m_vecBlockDesignateMin = IVector(0, 0, 0);
	m_vecBlockDesignateMax = IVector(0, 0, 0);
}

void CSPPlayer::FinishBlockDesignate()
{
	if (!m_eBlockDesignateMode)
		return;

	CVoxelTree* pTree = GetPlayerCharacter()->GameData().GetVoxelTree();

	if (!pTree)
		return;

	if (m_iBlockDesignateDimension == -1)
	{
		CScalableVector vecPoint;
		if (!FindBlockPlacePoint(vecPoint))
			return;

		m_vecBlockDesignateMin = m_vecBlockDesignateMax = pTree->ToVoxelCoordinates(vecPoint);

		m_iBlockDesignateDimension = 0;
	}
	else
	{
		CScalableVector vecPoint;
		if (!FindBlockPlacePoint(vecPoint))
			return;

		IVector vecNew = pTree->ToVoxelCoordinates(vecPoint);

		if (vecNew.x < m_vecBlockDesignateMin.x)
			m_vecBlockDesignateMin.x = vecNew.x;
		if (vecNew.y < m_vecBlockDesignateMin.y)
			m_vecBlockDesignateMin.y = vecNew.y;
		if (vecNew.z < m_vecBlockDesignateMin.z)
			m_vecBlockDesignateMin.z = vecNew.z;
		if (vecNew.x > m_vecBlockDesignateMax.x)
			m_vecBlockDesignateMax.x = vecNew.x;
		if (vecNew.y > m_vecBlockDesignateMax.y)
			m_vecBlockDesignateMax.y = vecNew.y;
		if (vecNew.z > m_vecBlockDesignateMax.z)
			m_vecBlockDesignateMax.z = vecNew.z;

		pTree->AddBlockDesignation(m_eBlockDesignateMode, m_vecBlockDesignateMin, m_vecBlockDesignateMax);

		m_eBlockDesignateMode = ITEM_NONE;
	}
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
