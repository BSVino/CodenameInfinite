#include "disassembler.h"

#include <tinker/keys.h>
#include <tinker/cvar.h>
#include <game/gameserver.h>
#include <renderer/game_renderer.h>
#include <renderer/game_renderingcontext.h>
#include <game/entities/character.h>
#include <renderer/particles.h>

#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "entities/structures/spire.h"
#include "entities/items/pickup.h"
#include "planet/trees.h"
#include "voxel/voxel_tree.h"

REGISTER_ENTITY(CDisassembler);

NETVAR_TABLE_BEGIN(CDisassembler);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CDisassembler);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bDisassembling);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CStructure>, m_hDisassemblingStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, IVector, m_vecDisassemblingBlock);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, TVector, m_vecDisassemblingGround);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CTreeAddress, m_oDisassemblingTree);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flDisassemblingStart);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CDisassembler);
INPUTS_TABLE_END();

void CDisassembler::Precache()
{
	PrecacheParticleSystem("disassembler-attack");
	PrecacheParticleSystem("disassembler-disassemble");
	PrecacheMaterial("textures/disassembler.mat");
}

void CDisassembler::Spawn()
{
	BaseClass::Spawn();

	m_bDisassembling = false;
	m_flDisassemblingStart = 0;
}

CVar disassemble_structure_time("disassemble_structure_time", "4.0");
CVar disassemble_block_time("disassemble_block_time", "2.0");
CVar disassemble_damage("disassemble_damage", "30"); // per second

void CDisassembler::Think()
{
	BaseClass::Think();

	CCharacter* pOwner = GetOwner();

	if (!pOwner)
		return;

	CPlayerCharacter* pPlayer = dynamic_cast<CPlayerCharacter*>(pOwner);
	if (!pPlayer)
		return;

	if (m_bDisassembling)
	{
		float flDisassembleTime = disassemble_block_time.GetFloat();
		if (m_hDisassemblingStructure)
			flDisassembleTime = disassemble_structure_time.GetFloat();

		if (m_flDisassemblingStart && GameServer()->GetGameTime() > m_flDisassemblingStart + flDisassembleTime)
		{
			m_flDisassemblingStart = 0;

			if (m_hDisassemblingStructure)
				m_hDisassemblingStructure->Delete();
			else if (m_vecDisassemblingBlock != IVector() && pPlayer->GameData().GetVoxelTree())
				pPlayer->GameData().GetVoxelTree()->RemoveBlock(m_vecDisassemblingBlock);
			else if (m_vecDisassemblingGround != TVector() && pPlayer->GameData().GetPlanet())
			{
				CPickup* pDisassembled = GameServer()->Create<CPickup>("CPickup");
				pDisassembled->GameData().SetPlayerOwner(pPlayer->GameData().GetPlayerOwner());
				pDisassembled->GameData().SetPlanet(pPlayer->GameData().GetPlanet());
				pDisassembled->SetGlobalTransform(pPlayer->GameData().GetPlanet()->GetGlobalTransform()); // Avoid floating point precision problems
				pDisassembled->SetMoveParent(pPlayer->GameData().GetPlanet());
				pDisassembled->SetLocalTransform(pPlayer->GetLocalTransform());
				pDisassembled->SetLocalOrigin(m_vecDisassemblingGround + pPlayer->GetLocalTransform().GetUpVector()*0.3f);
				pDisassembled->SetItem(ITEM_DIRT);
			}
			else if (m_oDisassemblingTree.IsValid() && pPlayer->GameData().GetPlanet())
			{
				double flScale = CScalableFloat::ConvertUnits(1, pPlayer->GameData().GetPlanet()->GetScale(), SCALE_METER);

				DoubleVector vecTree = pPlayer->GameData().GetPlanet()->GetTreeManager()->GetTreeOrigin(m_oDisassemblingTree) * flScale;
				DoubleVector vecUp = vecTree.Normalized();

				size_t iWoods = RandomInt(3, 5);

				for (size_t i = 0; i < iWoods; i++)
				{
					CPickup* pDisassembled = GameServer()->Create<CPickup>("CPickup");
					pDisassembled->GameData().SetPlayerOwner(pPlayer->GameData().GetPlayerOwner());
					pDisassembled->GameData().SetPlanet(pPlayer->GameData().GetPlanet());
					pDisassembled->SetGlobalTransform(pPlayer->GameData().GetPlanet()->GetGlobalTransform()); // Avoid floating point precision problems
					pDisassembled->SetMoveParent(pPlayer->GameData().GetPlanet());
					pDisassembled->SetLocalTransform(pPlayer->GetLocalTransform());
					pDisassembled->SetLocalOrigin(vecTree + vecUp*i);
					pDisassembled->SetItem(ITEM_WOOD);
				}

				pPlayer->GameData().GetPlanet()->GetTreeManager()->RemoveTree(m_oDisassemblingTree);
			}
			else
				TAssert(false);

			m_vecDisassemblingBlock = IVector();
			m_vecDisassemblingGround = TVector();
			m_hDisassemblingStructure = nullptr;
			m_oDisassemblingTree.Reset();
		}

		Matrix4x4 mTransform = pPlayer->GetPhysicsTransform();

		Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*pPlayer->EyeHeight();
		Vector vecDirection = pPlayer->GameData().TransformVectorLocalToPhysics(AngleVector(pPlayer->GetViewAngles()));

		CTraceResult tr;
		GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*Range(), pPlayer);

		if (tr.m_pHit)
			tr.m_pHit->TakeDamage(GetOwner(), this, DAMAGE_BURN, disassemble_damage.GetFloat()*(float)GameServer()->GetFrameTime());

		{
			CStructure* pStructure = dynamic_cast<CStructure*>(tr.m_pHit);

			if (m_hDisassemblingStructure && pStructure != m_hDisassemblingStructure)
			{
				m_flDisassemblingStart = 0;
				m_hDisassemblingStructure = nullptr;
				RestartParticles();
				return;
			}

			if (!m_hDisassemblingStructure && pStructure && !pStructure->IsDeleted())
			{
				m_hDisassemblingStructure = pStructure;
				m_flDisassemblingStart = GameServer()->GetGameTime();
				RestartParticles();
				return;
			}
		}

		if (pPlayer->GetNearestSpire() && !m_hDisassemblingStructure)
		{
			CScalableVector vecSpire = pPlayer->GetNearestSpire()->GetLocalOrigin();

			CVoxelTree* pTree = pPlayer->GameData().GetVoxelTree();

			TVector vecLocal = CScalableVector(pPlayer->GameData().TransformPointPhysicsToLocal(tr.m_vecHit + vecDirection * 0.01f), SCALE_METER);
			item_t eBlock = pTree->GetBlock(vecLocal);
			if (m_vecDisassemblingBlock != IVector() && (!eBlock || pTree->ToVoxelCoordinates(vecLocal) != m_vecDisassemblingBlock))
			{
				m_vecDisassemblingBlock = IVector();
				m_flDisassemblingStart = 0;
				RestartParticles();
				return;
			}
			else if (m_vecDisassemblingBlock == IVector() && eBlock)
			{
				m_vecDisassemblingBlock = pTree->ToVoxelCoordinates(vecLocal);
				m_flDisassemblingStart = GameServer()->GetGameTime();
				RestartParticles();
				return;
			}
		}

		if (pPlayer->GameData().GetPlanet() && !m_hDisassemblingStructure && m_vecDisassemblingBlock == IVector())
		{
			bool bHitGround = pPlayer->GameData().GetPlanet()->IsExtraPhysicsEntGround(tr.m_iHitExtra);
			if (tr.m_iHitExtra == ~0)
				bHitGround = false;
			if (tr.m_flFraction == 1)
				bHitGround = false;

			if (m_vecDisassemblingGround != TVector() && (!bHitGround || pPlayer->GameData().TransformPointPhysicsToLocal(tr.m_vecHit).DistanceSqr(m_vecDisassemblingGround) > 1))
			{
				m_vecDisassemblingGround = TVector();
				m_flDisassemblingStart = 0;
				RestartParticles();
				return;
			}
			else if (bHitGround && m_vecDisassemblingGround == TVector())
			{
				m_vecDisassemblingGround = pPlayer->GameData().TransformPointPhysicsToLocal(tr.m_vecHit);
				m_flDisassemblingStart = GameServer()->GetGameTime();
				RestartParticles();
				return;
			}
		}

		if (pPlayer->GameData().GetPlanet() && !m_hDisassemblingStructure && m_vecDisassemblingBlock == IVector() && m_vecDisassemblingGround == TVector())
		{
			CTreeAddress oHitTree;
			bool bHitTree = pPlayer->GameData().GetPlanet()->IsExtraPhysicsEntTree(tr.m_iHitExtra, oHitTree);
			if (tr.m_iHitExtra == ~0)
				bHitTree = false;
			if (tr.m_flFraction == 1)
				bHitTree = false;

			double flScale = CScalableFloat::ConvertUnits(1, pPlayer->GameData().GetPlanet()->GetScale(), SCALE_METER);

			if (m_oDisassemblingTree.IsValid() && (!bHitTree || oHitTree != m_oDisassemblingTree))
			{
				m_oDisassemblingTree.Reset();
				m_flDisassemblingStart = 0;
				RestartParticles();
				return;
			}
			else if (bHitTree && !m_oDisassemblingTree.IsValid())
			{
				m_oDisassemblingTree = oHitTree;
				m_flDisassemblingStart = GameServer()->GetGameTime();
				RestartParticles();
				return;
			}
		}
	}
}

void CDisassembler::DrawViewModel(CGameRenderingContext* pContext)
{
	CCharacter* pOwner = GetOwner();

	if (!pOwner)
		return;

	CSPPlayer* pPlayerOwner = dynamic_cast<CSPPlayer*>(pOwner->GetControllingPlayer());
	if (pPlayerOwner)
	{
		if (pPlayerOwner->IsInBlockPlaceMode())
			return;

		if (pPlayerOwner->IsInConstructionMode())
			return;

		if (pPlayerOwner->IsInBlockDesignateMode())
			return;

		CPlayerCharacter* pPlayerCharacterOwner = dynamic_cast<CPlayerCharacter*>(pOwner);
		if (pPlayerCharacterOwner && pPlayerCharacterOwner->IsHoldingPowerCord())
			return;
	}

	Vector vecForward, vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, &vecRight, &vecUp);

	pContext->Translate(vecForward * 0.1f - vecUp * 0.025f + vecRight * 0.025f);

	if (pOwner->IsAttacking() || m_bDisassembling)
	{
		pContext->Rotate(-45, vecRight);
		pContext->Translate(vecUp * -0.01f);
	}

	pContext->UseMaterial(CMaterialHandle("textures/disassembler.mat"));

	float flRadius = 0.02f;

	vecUp *= flRadius;
	vecRight *= flRadius;

	pContext->BeginRenderTriFan();
		pContext->TexCoord(0.0f, 1.0f);
		pContext->Vertex(-vecRight + vecUp);
		pContext->TexCoord(0.0f, 0.0f);
		pContext->Vertex(-vecRight - vecUp);
		pContext->TexCoord(1.0f, 0.0f);
		pContext->Vertex(vecRight - vecUp);
		pContext->TexCoord(1.0f, 1.0f);
		pContext->Vertex(vecRight + vecUp);
	pContext->EndRender();
}

void CDisassembler::OwnerMouseInput(int iButton, int iState)
{
	if (iButton == TINKER_KEY_MOUSE_LEFT && iState == 0)
	{
		if (IsDisassembling())
		{
			EndDisassembly();
			return;
		}
	}

	if (iButton == TINKER_KEY_MOUSE_LEFT && iState == 1)
	{
		CPlayerCharacter* pOwner = dynamic_cast<CPlayerCharacter*>(GetOwner());
		if (pOwner)
		{
			CPlanet* pPlanet = pOwner->GameData().GetPlanet();

			Matrix4x4 mTransform = pOwner->GetPhysicsTransform();

			Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*pOwner->EyeHeight();
			Vector vecDirection = pOwner->GameData().TransformVectorLocalToPhysics(AngleVector(pOwner->GetViewAngles()));

			CTraceResult tr;
			GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*Range(), pOwner);

			CStructure* pStructureHit = dynamic_cast<CStructure*>(tr.m_pHit);

			CVoxelTree* pTree = pOwner->GameData().GetVoxelTree();

			TVector vecLocal = CScalableVector(pOwner->GameData().TransformPointPhysicsToLocal(tr.m_vecHit + vecDirection * 0.01f), SCALE_METER);

			item_t eBlock = ITEM_NONE;
			if (pTree)
				eBlock = pTree->GetBlock(vecLocal);

			CTreeAddress oTreeAddress;
			if (tr.m_flFraction < 1 && pStructureHit && pStructureHit->GetOwner() == GetOwner()->GetControllingPlayer())
				BeginDisassembly(pStructureHit);
			else if (eBlock && pTree)
				BeginDisassembly(pTree->ToVoxelCoordinates(vecLocal));
			else if (pPlanet && pPlanet->IsExtraPhysicsEntGround(tr.m_iHitExtra) && tr.m_flFraction < 1)
				BeginDisassembly(TVector(pOwner->GameData().TransformPointPhysicsToLocal(tr.m_vecHit)));
			else if (pPlanet && tr.m_flFraction < 1 && pPlanet->IsExtraPhysicsEntTree(tr.m_iHitExtra, oTreeAddress))
				BeginDisassembly(oTreeAddress);
		}

		if (!m_bDisassembling)
			BeginDisassembly();
	}
}

void CDisassembler::BeginDisassembly(CStructure* pStructure)
{
	TAssert(!m_bDisassembling);
	if (m_bDisassembling)
		return;

	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_hDisassemblingStructure = pStructure;
	m_vecDisassemblingGround = TVector();
	m_vecDisassemblingBlock = IVector();
	m_oDisassemblingTree.Reset();

	RestartParticles();

	m_bDisassembling = true;
}

void CDisassembler::BeginDisassembly(const IVector& vecBlock)
{
	TAssert(!m_bDisassembling);
	if (m_bDisassembling)
		return;

	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_vecDisassemblingBlock = vecBlock;
	m_vecDisassemblingGround = TVector();
	m_hDisassemblingStructure = nullptr;
	m_oDisassemblingTree.Reset();

	RestartParticles();

	m_bDisassembling = true;
}

void CDisassembler::BeginDisassembly(const TVector& vecGround)
{
	TAssert(!m_bDisassembling);
	if (m_bDisassembling)
		return;

	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_vecDisassemblingGround = vecGround;
	m_hDisassemblingStructure = nullptr;
	m_vecDisassemblingBlock = IVector();
	m_oDisassemblingTree.Reset();

	RestartParticles();

	m_bDisassembling = true;
}

void CDisassembler::BeginDisassembly(const CTreeAddress& oTree)
{
	TAssert(!m_bDisassembling);
	if (m_bDisassembling)
		return;

	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_oDisassemblingTree = oTree;
	m_vecDisassemblingGround = TVector();
	m_hDisassemblingStructure = nullptr;
	m_vecDisassemblingBlock = IVector();

	RestartParticles();

	m_bDisassembling = true;
}

void CDisassembler::BeginDisassembly()
{
	TAssert(!m_bDisassembling);
	if (m_bDisassembling)
		return;

	m_flDisassemblingStart = 0;
	m_vecDisassemblingGround = TVector();
	m_hDisassemblingStructure = nullptr;
	m_vecDisassemblingBlock = IVector();
	m_oDisassemblingTree.Reset();

	RestartParticles();

	m_bDisassembling = true;
}

void CDisassembler::EndDisassembly()
{
	m_flDisassemblingStart = 0;
	m_hDisassemblingStructure = nullptr;
	m_vecDisassemblingBlock = IVector();
	m_vecDisassemblingGround = TVector();
	m_oDisassemblingTree.Reset();

	CParticleSystemLibrary::StopInstances("disassembler-disassemble");

	m_bDisassembling = false;
}

void CDisassembler::RestartParticles()
{
	CParticleSystemLibrary::StopInstances("disassembler-disassemble");

	CCharacter* pOwner = GetOwner();
	CPlayerCharacter* pPlayer = dynamic_cast<CPlayerCharacter*>(pOwner);

	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, nullptr, nullptr);

	if (pPlayer)
	{
		double flScale = 1;
		if (pPlayer->GetNearestPlanet())
			flScale = CScalableFloat::ConvertUnits(1, pPlayer->GetNearestPlanet()->GetScale(), SCALE_METER);

		if (m_hDisassemblingStructure)
			CParticleSystemLibrary::AddInstance("disassembler-disassemble", m_hDisassemblingStructure->GetLocalOrigin() - pPlayer->GetLocalOrigin());
		else if (m_vecDisassemblingBlock != IVector() && pPlayer->GameData().GetVoxelTree())
			CParticleSystemLibrary::AddInstance("disassembler-disassemble", pPlayer->GameData().GetVoxelTree()->ToLocalCoordinates(m_vecDisassemblingBlock) + pPlayer->GameData().GetVoxelTree()->GetTreeToPlanet().TransformVector(Vector(0.5f, 0.5f, 0.5f)) - pPlayer->GetLocalOrigin());
		else if (m_vecDisassemblingGround != TVector() && pPlayer->GameData().GetPlanet())
			CParticleSystemLibrary::AddInstance("disassembler-disassemble", m_vecDisassemblingGround - pPlayer->GetLocalOrigin());
		else if (m_oDisassemblingTree.IsValid() && pPlayer->GameData().GetPlanet())
			CParticleSystemLibrary::AddInstance("disassembler-disassemble", pPlayer->GameData().GetPlanet()->GetTreeManager()->GetTreeOrigin(m_oDisassemblingTree) * flScale - pPlayer->GetLocalOrigin());
		else
			CParticleSystemLibrary::AddInstance("disassembler-disassemble", GameServer()->GetRenderer()->GetCameraPosition() + vecForward*Range());
	}
	else
		CParticleSystemLibrary::AddInstance("disassembler-disassemble", GameServer()->GetRenderer()->GetCameraPosition() + vecForward*Range());
}

void CDisassembler::MeleeAttack()
{
	BaseClass::MeleeAttack();

	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, nullptr, nullptr);

	if (GetOwner())
		CParticleSystemLibrary::AddInstance("disassembler-attack", GameServer()->GetRenderer()->GetCameraPosition() + vecForward);
}
