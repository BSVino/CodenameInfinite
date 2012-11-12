#ifndef SP_PLAYER_H
#define SP_PLAYER_H

#include <tengine/game/entities/player.h>

#include <voxel/ivector.h>
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
	bool                    FindBlockPlacePoint(CScalableVector& vecLocal, CBaseEntity** pGiveTo = nullptr) const;
	void                    FinishBlockPlace();
	bool                    IsInBlockPlaceMode() const { return !!m_eBlockPlaceMode; }

	void                    EnterBlockDesignateMode(item_t eBlock);
	void                    FinishBlockDesignate();
	bool                    IsInBlockDesignateMode() const { return !!m_eBlockDesignateMode; }

	void                    CommandMenuOpened(CCommandMenu* pMenu);
	void                    CommandMenuClosed(CCommandMenu* pMenu);
	CCommandMenu*           GetActiveCommandMenu() { return m_pActiveCommandMenu; }

private:
	structure_type      m_eConstructionMode;
	item_t              m_eBlockPlaceMode;
	item_t              m_eBlockDesignateMode;
	int                 m_iBlockDesignateDimension;
	IVector             m_vecBlockDesignateMin;
	IVector             m_vecBlockDesignateMax;

	CCommandMenu*       m_pActiveCommandMenu;
};

#endif
