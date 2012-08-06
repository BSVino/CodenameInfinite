#ifndef LW_LEVEL_H
#define LW_LEVEL_H

#include <tmap.h>
#include <tstring.h>
#include <cachedvalue.h>
#include <matrix.h>
#include <geometry.h>
#include <trs.h>

#include <textures/materialhandle.h>

// A description of an entity for use in the level editor
class CLevelEntity
{
public:
	class CLevelEntityOutput
	{
	public:
		CLevelEntityOutput()
		{
			m_bKill = false;
		}

	public:
		tstring					m_sOutput;
		tstring					m_sTargetName;
		tstring					m_sInput;
		tstring					m_sArgs;
		bool					m_bKill;
	};

public:
	CLevelEntity(const tstring& sClass)
	{
		InitializeCallbacks();
		m_sClass = sClass;
	}

	CLevelEntity(const CLevelEntity& o)
	{
		(*this) = o;

		InitializeCallbacks();
	}

	CLevelEntity()
	{
		InitializeCallbacks();
	}

public:
	void								InitializeCallbacks()
	{
		m_mGlobalTransform.SetCallbacks(&CalculateGlobalTransform, this);
		m_trsGlobalTRS.SetCallbacks(&CalculateGlobalTRS, this);
		m_bVisible.SetCallbacks(&CalculateVisible, this);
		m_bRenderInverted.SetCallbacks(&CalculateRenderInverted, this);
		m_bDisableBackCulling.SetCallbacks(&CalculateDisableBackCulling, this);
		m_iModel.SetCallbacks(&CalculateModelID, this);
		m_vecScale.SetCallbacks(&CalculateScale, this);
		m_aabbBounds.SetCallbacks(&CalculateBoundingBox, this);
		m_sName.SetCallbacks(&CalculateName, this);
	}

	void								Dirtify()
	{
		m_mGlobalTransform.Dirtify();
		m_trsGlobalTRS.Dirtify();
		m_bVisible.Dirtify();
		m_bRenderInverted.Dirtify();
		m_bDisableBackCulling.Dirtify();
		m_iModel.Dirtify();
		m_vecScale.Dirtify();
		m_aabbBounds.Dirtify();
		m_sName.Dirtify();
	}

	tstring								GetClass() { return m_sClass; }
	void								SetClass(const tstring& sClass) { m_sClass = sClass; }

	const CMaterialHandle&				GetMaterialModel() { return m_hMaterialModel; }

	tvector<CLevelEntityOutput>&		GetOutputs() { return m_aOutputs; }

	const tstring&						GetParameterValue(const tstring& sKey) const;
	void								SetParameterValue(const tstring& sKey, const tstring& sValue);
	void								RemoveParameter(const tstring& sKey);
	bool								HasParameterValue(const tstring& sKey) const;
	tmap<tstring, tstring>&				GetParameters() { return m_asParameters; }

	Matrix4x4							GetGlobalTransform() { return m_mGlobalTransform; }
	void								SetGlobalTransform(const Matrix4x4& m) { m_mGlobalTransform = m; }
	TRS									GetGlobalTRS() { return m_trsGlobalTRS; }
	bool								IsVisible() { return m_bVisible; }
	bool								ShouldRenderInverted() { return m_bRenderInverted; }
	bool								ShouldDisableBackCulling() { return m_bDisableBackCulling; }
	size_t								GetModelID() { return m_iModel; }
	Vector								GetScale() { return m_vecScale; }
	AABB								GetBoundingBox() { return m_aabbBounds; }
	tstring								GetName() { return m_sName; }

public:
	static Matrix4x4					CalculateGlobalTransform(CLevelEntity* pThis);
	static TRS							CalculateGlobalTRS(CLevelEntity* pThis);
	static bool							CalculateVisible(CLevelEntity* pThis);
	static bool							CalculateRenderInverted(CLevelEntity* pThis);
	static bool							CalculateDisableBackCulling(CLevelEntity* pThis);
	static size_t						CalculateModelID(CLevelEntity* pThis);
	static Vector						CalculateScale(CLevelEntity* pThis);
	static AABB							CalculateBoundingBox(CLevelEntity* pThis);
	static tstring						CalculateName(CLevelEntity* pThis);

private:
	tstring								m_sClass;
	tmap<tstring, tstring>				m_asParameters;
	CMaterialHandle						m_hMaterialModel;

	CCachedValue<Matrix4x4, CLevelEntity>	m_mGlobalTransform;
	CCachedValue<TRS, CLevelEntity>			m_trsGlobalTRS;
	CCachedValue<bool, CLevelEntity>		m_bVisible;
	CCachedValue<bool, CLevelEntity>		m_bRenderInverted;
	CCachedValue<bool, CLevelEntity>		m_bDisableBackCulling;
	CCachedValue<size_t, CLevelEntity>		m_iModel;
	CCachedValue<Vector, CLevelEntity>		m_vecScale;
	CCachedValue<AABB, CLevelEntity>		m_aabbBounds;
	CCachedValue<tstring, CLevelEntity>		m_sName;

	tvector<CLevelEntityOutput>			m_aOutputs;
};

class CLevel
{
public:
	virtual					~CLevel() {};

public:
	void					ReadInfoFromData(const class CData* pData);
	virtual void			OnReadInfo(const class CData* pData);

	void					SaveToFile();

	const tstring&			GetName() { return m_sName; }
	const tstring&			GetFile() { return m_sFile; }

	void					SetFile(const tstring& sFile) { m_sFile = sFile; }

	const tstring&			GetGameMode() { return m_sGameMode; }

	void					CreateEntitiesFromData(const CData* pData);
	tvector<CLevelEntity>&	GetEntityData() { return m_aLevelEntities; }
	const tvector<CLevelEntity>& GetEntityData() const { return m_aLevelEntities; }

protected:
	tstring					m_sName;
	tstring					m_sFile;

	tstring					m_sGameMode;

	tvector<CLevelEntity>	m_aLevelEntities;
};

#endif
