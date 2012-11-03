#pragma once

#include "structure.h"

#include "voxel/voxel_tree.h"

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
	void        OnDeleted(CBaseEntity* pEntity);

	const tvector<CEntityHandle<CStructure>>&    GetStructures() const { return m_hStructures; }
	const tvector<CEntityHandle<CMine>>&         GetMines() const { return m_hMines; }
	const tvector<CEntityHandle<CBot>>&          GetBots() const { return m_hBots; }

	void        SetBaseName(const tstring& sName) { m_sBaseName = sName; }
	tstring     GetBaseName() { return m_sBaseName; }

	CVoxelTree*       GetVoxelTree() { return &m_oVoxelTree; }
	const CVoxelTree* GetVoxelTree() const { return &m_oVoxelTree; }

private:
	tstring     m_sBaseName;

	CVoxelTree  m_oVoxelTree;

	double      m_flBuildStart;

	double      m_flNextMonster;

	tvector<CEntityHandle<CStructure>> m_hStructures;
	tvector<CEntityHandle<CMine>>      m_hMines;
	tvector<CEntityHandle<CBot>>       m_hBots;
};
