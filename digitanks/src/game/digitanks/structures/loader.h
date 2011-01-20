#ifndef DT_LOADER_H
#define DT_LOADER_H

#include "structure.h"

class CLoader : public CStructure
{
	REGISTER_ENTITY_CLASS(CLoader, CStructure);

public:
	virtual void				Precache();
	virtual void				Spawn();

	virtual void				OnTeamChange();

	virtual void				StartTurn();

	virtual void				PostRender(bool bTransparent);

	virtual void				SetupMenu(menumode_t eMenuMode);

	virtual bool				NeedsOrders();

	virtual void				UpdateInfo(eastl::string16& sInfo);

	void						BeginProduction();
	void						BeginProduction(class CNetworkParameters* p);
	void						CancelProduction();
	void						CancelProduction(class CNetworkParameters* p);
	bool						IsProducing() { return m_bProducing; };
	void						AddProduction(size_t iProduction) { m_iProductionStored += iProduction; }
	size_t						GetUnitProductionCost();

	virtual void				InstallUpdate(size_t x, size_t y);

	size_t						GetFleetPointsRequired();
	bool						HasEnoughFleetPoints();

	size_t						GetTurnsToProduce();

	void						SetBuildUnit(unittype_t eBuildUnit);
	unittype_t					GetBuildUnit() const { return m_eBuildUnit.Get(); };

	virtual eastl::string16		GetName();
	virtual unittype_t			GetUnitType() const;
	virtual float				TotalHealth() const { return 70; };

protected:
	CNetworkedVariable<unittype_t> m_eBuildUnit;
	CNetworkedVariable<size_t>	m_iBuildUnitModel;

	CNetworkedVariable<bool>	m_bProducing;
	CNetworkedVariable<size_t>	m_iProductionStored;

	CNetworkedVariable<size_t>	m_iTankAttack;
	CNetworkedVariable<size_t>	m_iTankDefense;
	CNetworkedVariable<size_t>	m_iTankMovement;
	CNetworkedVariable<size_t>	m_iTankHealth;
	CNetworkedVariable<size_t>	m_iTankRange;

	static size_t				s_iCancelIcon;
	static size_t				s_iInstallIcon;
	static size_t				s_iInstallAttackIcon;
	static size_t				s_iInstallDefenseIcon;
	static size_t				s_iInstallMovementIcon;
	static size_t				s_iInstallRangeIcon;
	static size_t				s_iInstallHealthIcon;
	static size_t				s_iBuildInfantryIcon;
	static size_t				s_iBuildTankIcon;
	static size_t				s_iBuildArtilleryIcon;
};

#endif
