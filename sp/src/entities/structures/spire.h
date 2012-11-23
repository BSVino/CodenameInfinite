#pragma once

#include "structure.h"

class CMine;
class CBot;

class CSpire : public CStructure
{
	REGISTER_ENTITY_CLASS(CSpire, CStructure);

public:
	void        Precache();
	void        Spawn();

	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        PerformStructureTask(CSPCharacter* pUser);
	bool        IsOccupied() const;
	void        SetupMenuButtons();
	void        MenuCommand(const tstring& sCommand);

	void        PostConstruction();
	void        PostConstructionFinished();

	void        StartBuildWorker();
	void        EndBuild();

	void        AddUnit(CBaseEntity* pEntity);
	void        OnDeleted(const CBaseEntity* pEntity);

	const tvector<CEntityHandle<CStructure>>&    GetStructures() const { return m_hStructures; }
	const tvector<CEntityHandle<CMine>>&         GetMines() const { return m_hMines; }
	const tvector<CEntityHandle<CBot>>&          GetBots() const { return m_hBots; }

	void        SetBaseName(const tstring& sName) { m_sBaseName = sName; }
	tstring     GetBaseName() { return m_sBaseName; }

	structure_type    StructureType() const { return STRUCTURE_SPIRE; }

private:
	tstring     m_sBaseName;

	double      m_flBuildStart;

	double      m_flNextMonster;

	tvector<CEntityHandle<CStructure>> m_hStructures;
	tvector<CEntityHandle<CMine>>      m_hMines;
	tvector<CEntityHandle<CBot>>       m_hBots;
};
