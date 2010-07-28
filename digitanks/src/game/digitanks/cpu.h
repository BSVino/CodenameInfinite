#ifndef DT_CPU_H
#define DT_CPU_H

#include "structure.h"

class CCPU : public CSupplier
{
	REGISTER_ENTITY_CLASS(CCPU, CSupplier);

public:
	virtual void				Spawn();
	virtual void				Precache();

	virtual size_t				InitialDataStrength() { return 3200; };
	virtual size_t				BaseDataFlowPerTurn() { return 50; };
	virtual float				TotalHealth() const { return 100; };
	virtual size_t				FleetPoints() const { return 10; };

	virtual float				GetBoundingRadius() const { return 8; };

	virtual bool				AllowControlMode(controlmode_t eMode);

	virtual void				SetupMenu(menumode_t eMenuMode);

	bool						IsPreviewBuildValid() const;

	Vector						GetPreviewBuild() const { return m_vecPreviewBuild; };
	virtual void				SetPreviewBuild(Vector vecPreviewBuild);
	virtual void				SetPreviewStructure(structure_t ePreviewStructure) { m_ePreviewStructure = ePreviewStructure; };
	void						ClearPreviewBuild();

	void						BeginConstruction();
	void						CancelConstruction();
	bool						HasConstruction() { return m_hConstructing != NULL; }
	CStructure*					GetConstructing() { return m_hConstructing; }

	virtual void				StartTurn();

	virtual void				OnRender();
	virtual void				PostRender();

	virtual void				UpdateInfo(std::string& sInfo);

	virtual const char*			GetName() { return "Central Processing Unit"; };

protected:
	Vector						m_vecPreviewBuild;

	structure_t					m_ePreviewStructure;
	CEntityHandle<CStructure>	m_hConstructing;

	size_t						m_iFanModel;
	float						m_flFanRotationSpeed;
	float						m_flFanRotation;
};

#endif