#ifndef SP_PLAYER_H
#define SP_PLAYER_H

#include <tengine/game/entities/player.h>

#include "structures/structure.h"

class CSPPlayer : public CPlayer
{
	REGISTER_ENTITY_CLASS(CSPPlayer, CPlayer);

public:
	CSPPlayer();

public:
	virtual void					MouseMotion(int x, int y);
	virtual void                    MouseInput(int iButton, int iState);
	virtual void					KeyPress(int c);
	virtual void					KeyRelease(int c);

	class CPlayerCharacter*			GetPlayerCharacter() const;

	bool                ShouldRender() const;
	void                PostRender() const;

	void                    EnterConstructionMode(structure_type eStructure);
	bool                    FindConstructionPoint(CScalableVector& vecLocal) const;
	void                    FinishConstruction();

	void                AddSpires(size_t iSpires);
	size_t              GetNumSpires() { return m_aiStructures[STRUCTURE_SPIRE]; }

private:
	structure_type      m_eConstructionMode;
	size_t              m_aiStructures[STRUCTURE_TOTAL];
};

#endif
