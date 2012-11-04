#ifndef SP_PLAYER_H
#define SP_PLAYER_H

#include <tengine/game/entities/player.h>

#include "structures/structure.h"
#include "items/items.h"

class CCommandMenu;

class CSPPlayer : public CPlayer
{
	REGISTER_ENTITY_CLASS(CSPPlayer, CPlayer);

public:
	CSPPlayer();

public:
	void                    Precache();

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
	bool                    IsInConstructionMode() const { return !!m_eConstructionMode; }

	void                    EnterBlockPlaceMode(item_t eBlock);
	bool                    FindBlockPlacePoint(CScalableVector& vecLocal, CBaseEntity** pGiveTo) const;
	void                    FinishBlockPlace();
	bool                    IsInBlockPlaceMode() const { return !!m_eBlockPlaceMode; }

	void                    CommandMenuOpened(CCommandMenu* pMenu);
	void                    CommandMenuClosed(CCommandMenu* pMenu);

	void                AddSpires(size_t iSpires);
	size_t              GetNumSpires() { return m_aiStructures[STRUCTURE_SPIRE]; }

private:
	structure_type      m_eConstructionMode;
	item_t              m_eBlockPlaceMode;

	size_t              m_aiStructures[STRUCTURE_TOTAL];

	CCommandMenu*       m_pActiveCommandMenu;
};

#endif
