#include "disassembler.h"

#include <game/gameserver.h>
#include <renderer/game_renderer.h>
#include <renderer/game_renderingcontext.h>
#include <game/entities/character.h>
#include <renderer/particles.h>

#include "entities/sp_player.h"

REGISTER_ENTITY(CDisassembler);

NETVAR_TABLE_BEGIN(CDisassembler);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CDisassembler);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CDisassembler);
INPUTS_TABLE_END();

void CDisassembler::Precache()
{
	PrecacheParticleSystem("disassembler-attack");
	PrecacheMaterial("textures/disassembler.mat");
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
	}

	Vector vecForward, vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, &vecRight, &vecUp);

	pContext->Translate(vecForward * 0.1f - vecUp * 0.025f + vecRight * 0.025f);

	if (pOwner->IsAttacking())
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

void CDisassembler::MeleeAttack()
{
	BaseClass::MeleeAttack();

	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, nullptr, nullptr);

	if (GetOwner())
		CParticleSystemLibrary::AddInstance("disassembler-attack", GameServer()->GetRenderer()->GetCameraPosition() + vecForward);
}
