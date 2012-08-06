#pragma once

#include <trs.h>

#include <glgui/movablepanel.h>

#include "tool.h"
#include "manipulator.h"

class CCreateToySourcePanel : public glgui::CMovablePanel
{
	DECLARE_CLASS(CCreateToySourcePanel, glgui::CMovablePanel);

public:
							CCreateToySourcePanel();

public:
	void					Layout();

	EVENT_CALLBACK(CCreateToySourcePanel, ToyChanged);
	EVENT_CALLBACK(CCreateToySourcePanel, SourceChanged);
	EVENT_CALLBACK(CCreateToySourcePanel, Create);

	tstring					GetToyFileName();
	tstring					GetSourceFileName();

	void					FileNamesChanged();

public:
	glgui::CLabel*			m_pToyFileLabel;
	glgui::CTextField*		m_pToyFileText;

	glgui::CLabel*			m_pSourceFileLabel;
	glgui::CTextField*		m_pSourceFileText;

	glgui::CLabel*			m_pWarnings;

	glgui::CButton*			m_pCreate;
};

class CToySource
{
public:
	void					Save() const;
	void					Build() const;
	void					Open(const tstring& sFile);

public:
	tstring					m_sFilename;
	tstring					m_sToyFile;

	tstring					m_sMesh;
	tstring					m_sPhys;

	class CPhysicsShape
	{
	public:
		TRS					m_trsTransform;
	};

	tvector<CPhysicsShape>	m_aShapes;
};

class CSourcePanel : public glgui::CPanel, public glgui::IEventListener
{
	DECLARE_CLASS(CSourcePanel, glgui::CPanel);

public:
							CSourcePanel();

public:
	virtual void			SetVisible(bool bVis);

	void					Layout();
	void					LayoutFilename();
	void					UpdateFields();

	void					SetModelSourcesAutoComplete(glgui::CTextField* pField);

	EVENT_CALLBACK(CSourcePanel, ToyFileChanged);
	EVENT_CALLBACK(CSourcePanel, ModelChanged);
	EVENT_CALLBACK(CSourcePanel, PhysicsChanged);
	EVENT_CALLBACK(CSourcePanel, PhysicsAreaSelected);
	EVENT_CALLBACK(CSourcePanel, NewPhysicsShape);
	EVENT_CALLBACK(CSourcePanel, DeletePhysicsShape);
	EVENT_CALLBACK(CSourcePanel, Save);
	EVENT_CALLBACK(CSourcePanel, Build);
	EVENT_CALLBACK(CSourcePanel, MeshSource);
	EVENT_CALLBACK(CSourcePanel, MaterialSource);

public:
	glgui::CLabel*			m_pFilename;

	glgui::CLabel*			m_pToyFileLabel;
	glgui::CTextField*		m_pToyFileText;

	glgui::CMenu*			m_pMeshMenu;
	bool					m_bMesh;
	glgui::CTextField*		m_pMeshText;

	glgui::CLabel*			m_pPhysLabel;
	glgui::CTextField*		m_pPhysText;

	glgui::CLabel*			m_pPhysShapesLabel;
	glgui::CTree*			m_pPhysicsShapes;
	glgui::CButton*			m_pNewPhysicsShape;
	glgui::CButton*			m_pDeletePhysicsShape;

	glgui::CButton*			m_pSave;
	glgui::CButton*			m_pBuild;
};

class CToyEditor : public CWorkbenchTool, public IManipulatorListener
{
	DECLARE_CLASS(CToyEditor, CWorkbenchTool);

public:
							CToyEditor();
	virtual					~CToyEditor();

public:
	virtual void			Activate();
	virtual void			Deactivate();

	void					Layout();
	void					SetupMenu();

	virtual void			RenderScene();

	void					NewToy();
	CToySource&				GetToyToModify();
	const CToySource&		GetToy() { return m_oToySource; }
	void					MarkUnsaved();
	void					MarkSaved();
	bool					IsSaved() { return m_bSaved; }

	EVENT_CALLBACK(CToyEditor, NewToy);
	EVENT_CALLBACK(CToyEditor, SaveToy);
	EVENT_CALLBACK(CToyEditor, ChooseToy);
	EVENT_CALLBACK(CToyEditor, OpenToy);
	EVENT_CALLBACK(CToyEditor, BuildToy);

	bool					KeyPress(int c);
	bool					MouseInput(int iButton, tinker_mouse_state_t iState);
	void					MouseMotion(int x, int y);
	void					MouseWheel(int x, int y);

	virtual TVector			GetCameraPosition();
	virtual Vector          GetCameraDirection();

	virtual void			ManipulatorUpdated(const tstring& sArguments);
	virtual void            DuplicateMove(const tstring& sArguments);

	virtual tstring			GetToolName() { return "Toy Editor"; }

public:
	static CToyEditor*		Get() { return s_pToyEditor; }

protected:
	CCreateToySourcePanel*	m_pCreateToySourcePanel;

	CSourcePanel*			m_pSourcePanel;

	CToySource				m_oToySource;

	size_t					m_iMeshPreview;
	size_t					m_iPhysPreview;
	CMaterialHandle			m_hMaterialPreview;

	bool					m_bRotatingPreview;
	EAngle					m_angPreview;
	float					m_flPreviewDistance;

	bool					m_bSaved;

private:
	static CToyEditor*		s_pToyEditor;
};

inline CToyEditor* ToyEditor()
{
	return CToyEditor::Get();
}
