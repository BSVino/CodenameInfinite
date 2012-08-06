#pragma once

#include <tstring.h>
#include <tinker_memory.h>

#include <glgui/panel.h>
#include <glgui/movablepanel.h>
#include <game/cameramanager.h>
#include <game/level.h>

#include "tool.h"
#include "manipulator.h"

class CEntityPropertiesPanel : public glgui::CPanel, public glgui::IEventListener
{
	DECLARE_CLASS(CEntityPropertiesPanel, glgui::CPanel);

public:
							CEntityPropertiesPanel(bool bCommon);

public:
	void					Layout();

	EVENT_CALLBACK(CEntityPropertiesPanel, ModelChanged);
	EVENT_CALLBACK(CEntityPropertiesPanel, TargetChanged);
	EVENT_CALLBACK(CEntityPropertiesPanel, PropertyChanged);

	void					SetPropertyChangedListener(glgui::IEventListener* pListener, glgui::IEventListener::Callback pfnCallback);

	void					SetClass(const tstring& sClass) { m_sClass = sClass; }
	void					SetEntity(class CLevelEntity* pEntity);

public:
	tstring								m_sClass;
	float								m_bCommonProperties;
	class CLevelEntity*					m_pEntity;

	tvector<glgui::CControl<glgui::CLabel>>				m_ahPropertyLabels;
	tvector<glgui::CControl<glgui::CBaseControl>>		m_ahPropertyOptions;
	tvector<tstring>					m_asPropertyHandle;

	glgui::IEventListener::Callback		m_pfnPropertyChangedCallback;
	glgui::IEventListener*				m_pPropertyChangedListener;
};

class CCreateEntityPanel : public glgui::CMovablePanel
{
	DECLARE_CLASS(CCreateEntityPanel, glgui::CMovablePanel);

public:
							CCreateEntityPanel();

public:
	void					Layout();

	EVENT_CALLBACK(CCreateEntityPanel, ChooseClass);
	EVENT_CALLBACK(CCreateEntityPanel, ModelChanged);

public:
	bool					m_bReadyToCreate;

	glgui::CControl<glgui::CMenu>			m_hClass;

	glgui::CControl<glgui::CLabel>			m_hNameLabel;
	glgui::CControl<glgui::CTextField>		m_hNameText;

	glgui::CControl<glgui::CLabel>			m_hModelLabel;
	glgui::CControl<glgui::CTextField>		m_hModelText;

	glgui::CControl<CEntityPropertiesPanel>	m_hPropertiesPanel;
};

class CEditorPanel : public glgui::CPanel, public glgui::IEventListener
{
	DECLARE_CLASS(CEditorPanel, glgui::CPanel);

public:
							CEditorPanel();

public:
	void					Layout();
	void					LayoutEntity();
	void					LayoutOutput();
	void					LayoutInput();

	void                    Paint(float x, float y, float w, float h);

	CLevelEntity*			GetCurrentEntity();
	CLevelEntity::CLevelEntityOutput* GetCurrentOutput();

	EVENT_CALLBACK(CEditorPanel, EntitySelected);
	EVENT_CALLBACK(CEditorPanel, PropertyChanged);
	EVENT_CALLBACK(CEditorPanel, OutputSelected);
	EVENT_CALLBACK(CEditorPanel, AddOutput);
	EVENT_CALLBACK(CEditorPanel, RemoveOutput);
	EVENT_CALLBACK(CEditorPanel, ChooseOutput);
	EVENT_CALLBACK(CEditorPanel, TargetEntityChanged);
	EVENT_CALLBACK(CEditorPanel, ChooseInput);
	EVENT_CALLBACK(CEditorPanel, ArgumentsChanged);

public:
	glgui::CControl<glgui::CTree>			m_hEntities;
	glgui::CControl<glgui::CLabel>			m_hObjectTitle;

	glgui::CControl<glgui::CSlidingContainer>	m_hSlider;
	glgui::CControl<glgui::CSlidingPanel>	m_hPropertiesSlider;
	glgui::CControl<glgui::CSlidingPanel>	m_hOutputsSlider;

	glgui::CControl<CEntityPropertiesPanel>	m_hPropertiesPanel;

	glgui::CControl<glgui::CTree>			m_hOutputs;
	glgui::CControl<glgui::CButton>			m_hAddOutput;
	glgui::CControl<glgui::CButton>			m_hRemoveOutput;

	glgui::CControl<glgui::CMenu>			m_hOutput;

	glgui::CControl<glgui::CLabel>			m_hOutputEntityNameLabel;
	glgui::CControl<glgui::CTextField>		m_hOutputEntityNameText;

	glgui::CControl<glgui::CMenu>			m_hInput;

	glgui::CControl<glgui::CLabel>			m_hOutputArgsLabel;
	glgui::CControl<glgui::CTextField>		m_hOutputArgsText;
};

class CLevelEditor : public CWorkbenchTool, public IManipulatorListener
{
	DECLARE_CLASS(CLevelEditor, CWorkbenchTool);

public:
							CLevelEditor();
	virtual					~CLevelEditor();

public:
	virtual void            Think();

	void					RenderEntity(size_t i);
	void					RenderEntity(class CLevelEntity* pEntity, bool bSelected=false, bool bHover=false);
	void					RenderCreateEntityPreview();

	Vector					PositionFromMouse();
	size_t                  TraceLine(const Ray& vecTrace);
	void					EntitySelected();
	void					CreateEntityFromPanel(const Vector& vecPosition);
	static void				PopulateLevelEntityFromPanel(class CLevelEntity* pEntity, CEntityPropertiesPanel* pPanel);
	void					DuplicateSelectedEntity();

	class CLevel*			GetLevel() { return m_pLevel; }

	EVENT_CALLBACK(CLevelEditor, CreateEntity);
	EVENT_CALLBACK(CLevelEditor, SaveLevel);

	bool					KeyPress(int c);
	bool					MouseInput(int iButton, tinker_mouse_state_t iState);

	virtual void			Activate();
	virtual void			Deactivate();

	virtual void			RenderScene();

	virtual void			ManipulatorUpdated(const tstring& sArguments);
	virtual void            DuplicateMove(const tstring& sArguments);

	virtual bool			ShowCameraControls() { return true; }

	virtual tstring			GetToolName() { return "Level Editor"; }

	static CLevelEditor*	Get() { return s_pLevelEditor; }

protected:
	CHandle<class CLevel>	m_pLevel;

	glgui::CControl<CEditorPanel>			m_hEditorPanel;

	glgui::CControl<glgui::CPictureButton>	m_hCreateEntityButton;
	glgui::CControl<CCreateEntityPanel>		m_hCreateEntityPanel;

	float					m_flCreateObjectDistance;

	size_t					m_iHoverEntity;

private:
	static CLevelEditor*	s_pLevelEditor;
};

inline CLevelEditor* LevelEditor()
{
	return CLevelEditor::Get();
}
