#pragma once

#include "structure.h"

#include "voxel/voxel_tree.h"

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
	void        SetupMenuButtons();
	void        MenuCommand(const tstring& sCommand);

	void        PostConstruction();
	void        PostConstructionFinished();

	void        StartBuildWorker();
	void        EndBuild();

	void        SetBaseName(const tstring& sName) { m_sBaseName = sName; }
	tstring     GetBaseName() { return m_sBaseName; }

	CVoxelTree*       GetVoxelTree() { return &m_oVoxelTree; }
	const CVoxelTree* GetVoxelTree() const { return &m_oVoxelTree; }

private:
	tstring     m_sBaseName;

	CVoxelTree  m_oVoxelTree;

	double      m_flBuildStart;

	double      m_flNextMonster;
};
