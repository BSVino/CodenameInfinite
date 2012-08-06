#include "static.h"

#include <physics/physics.h>

REGISTER_ENTITY(CStatic);

NETVAR_TABLE_BEGIN(CStatic);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN_EDITOR(CStatic);
	SAVEDATA_EDITOR_VARIABLE("Model");
	SAVEDATA_EDITOR_VARIABLE("DisableBackCulling");
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStatic);
INPUTS_TABLE_END();

void CStatic::OnSetModel()
{
	BaseClass::OnSetModel();

	// In case the model has changed.
	if (IsInPhysics())
		RemoveFromPhysics();

	if (GetModelID() == ~0)
		return;

	AddToPhysics(CT_STATIC_MESH);
}
