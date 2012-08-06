#pragma once

#include "trs.h"
#include "tstring.h"

#include <glgui/glgui.h>
#include <tinker/keys.h>

class IManipulatorListener
{
public:
	virtual void		ManipulatorUpdated(const tstring& sArguments)=0;
	virtual void		DuplicateMove(const tstring& sArguments)=0;
};

class CManipulatorTool : public glgui::IEventListener
{
public:
	typedef enum
	{
		MT_TRANSLATE,
		MT_ROTATE,
		MT_SCALE,
	} TransformType;

public:
	CManipulatorTool();

public:
	void				Activate(IManipulatorListener* pListener, const TRS& trs=TRS(), const tstring& sArguments="");
	void				Deactivate();
	bool				IsActive() { return m_bActive; }
	bool				IsTransforming() { return m_bTransforming; }

	void				SetTransfromType(TransformType eTransform) { m_eTransform = eTransform; }
	TransformType		GetTransfromType() { return m_eTransform; }

	bool				MouseInput(int iButton, tinker_mouse_state_t iState);

	void				Render();

	Matrix4x4			GetTransform(bool bRotation = true, bool bScaling = true);
	TRS					GetTRS() { return m_trsTransform; }
	void				SetTRS(const TRS& trs) { m_trsTransform = trs; }

	TRS					GetNewTRS();

	EVENT_CALLBACK(CManipulatorTool, TranslateMode);
	EVENT_CALLBACK(CManipulatorTool, RotateMode);
	EVENT_CALLBACK(CManipulatorTool, ScaleMode);

protected:
	bool				m_bActive;
	bool				m_bTransforming;
	TransformType		m_eTransform;

	char				m_iLockedAxis;
	float				m_flStartX;
	float				m_flStartY;
	float				m_flOriginalDistance;

	TRS					m_trsTransform;

	IManipulatorListener*	m_pListener;
	tstring				m_sListenerArguments;

	glgui::CPictureButton*	m_pTranslateButton;
	glgui::CPictureButton*	m_pRotateButton;
	glgui::CPictureButton*	m_pScaleButton;

public:
	static CManipulatorTool*	Get();

protected:
	static CManipulatorTool*	s_pManipulatorTool;
};

inline CManipulatorTool* Manipulator()
{
	return CManipulatorTool::Get();
}
