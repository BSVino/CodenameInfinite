#include <renderer/particles.h>

void CParticleSystemLibrary::InitSystems()
{
	CParticleSystemLibrary* pPSL = CParticleSystemLibrary::Get();

	{
		CParticleSystem* pSystem = pPSL->GetParticleSystem(pPSL->AddParticleSystem("disassembler-attack"));
		pSystem->SetMaterialName("textures/disassembler-juice.mat");
		pSystem->SetLifeTime(0.2f);
		pSystem->SetEmissionRate(0.001f);
		pSystem->SetEmissionMax(150);
		pSystem->SetEmissionMaxDistance(0.1f);
		pSystem->SetAlpha(0.9f);
		pSystem->SetStartRadius(0.01f);
		pSystem->SetEndRadius(0.02f);
		pSystem->SetFadeOut(1.0f);
		pSystem->SetInheritedVelocity(0.0f);
		pSystem->SetRandomVelocity(AABB(Vector(-0.05f, -0.05f, -0.05f), Vector(0.05f, 0.05f, 0.05f)));
		pSystem->SetGravity(Vector(0, 0, 0));
		pSystem->SetDrag(0.95f);
		pSystem->SetRandomBillboardYaw(true);
	}
}
