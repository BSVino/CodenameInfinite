#include "sp_entity.h"

#include "sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CSPEntity);

NETVAR_TABLE_BEGIN(CSPEntity);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPEntity);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPEntity);
INPUTS_TABLE_END();

bool CSPEntity::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == GetScale();
}
