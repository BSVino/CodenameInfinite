#include "manipulator.h"

#include <geometry.h>

#include <tinker/application.h>
#include <game/gameserver.h>
#include <renderer/game_renderer.h>
#include <glgui/rootpanel.h>
#include <glgui/picturebutton.h>
#include <textures/materiallibrary.h>
#include <renderer/game_renderingcontext.h>
#include <game/gameserver.h>
#include <game/cameramanager.h>

CManipulatorTool::CManipulatorTool()
{
	m_bActive = false;
	m_pListener = nullptr;
	m_bTransforming = false;
	m_eTransform = MT_TRANSLATE;

	m_pTranslateButton = new glgui::CPictureButton("Translate", CMaterialLibrary::AddMaterial("editor/translate.mat"));
	glgui::CRootPanel::Get()->AddControl(m_pTranslateButton, true);
	m_pTranslateButton->Layout_AlignBottom(nullptr, 20);
	m_pTranslateButton->Layout_ColumnFixed(3, 0, m_pTranslateButton->GetWidth());
	m_pTranslateButton->SetClickedListener(this, TranslateMode);
	m_pTranslateButton->SetTooltip("Translate");
	m_pTranslateButton->SetVisible(false);

	m_pRotateButton = new glgui::CPictureButton("Rotate", CMaterialLibrary::AddMaterial("editor/rotate.mat"));
	glgui::CRootPanel::Get()->AddControl(m_pRotateButton, true);
	m_pRotateButton->Layout_AlignBottom(nullptr, 20);
	m_pRotateButton->Layout_ColumnFixed(3, 1, m_pRotateButton->GetWidth());
	m_pRotateButton->SetClickedListener(this, RotateMode);
	m_pRotateButton->SetTooltip("Rotate");
	m_pRotateButton->SetVisible(false);

	m_pScaleButton = new glgui::CPictureButton("Scale", CMaterialLibrary::AddMaterial("editor/scale.mat"));
	glgui::CRootPanel::Get()->AddControl(m_pScaleButton, true);
	m_pScaleButton->Layout_AlignBottom(nullptr, 20);
	m_pScaleButton->Layout_ColumnFixed(3, 2, m_pScaleButton->GetWidth());
	m_pScaleButton->SetClickedListener(this, ScaleMode);
	m_pScaleButton->SetTooltip("Scale");
	m_pScaleButton->SetVisible(false);
}

void CManipulatorTool::Activate(IManipulatorListener* pListener, const TRS& trs, const tstring& sArguments)
{
	m_bActive = true;
	m_pListener = pListener;
	m_trsTransform = trs;
	m_sListenerArguments = sArguments;
	m_pTranslateButton->SetVisible(true);
	m_pRotateButton->SetVisible(true);
	m_pScaleButton->SetVisible(true);
}

void CManipulatorTool::Deactivate()
{
	m_bActive = false;
	m_pTranslateButton->SetVisible(false);
	m_pRotateButton->SetVisible(false);
	m_pScaleButton->SetVisible(false);
}

bool CManipulatorTool::MouseInput(int iButton, tinker_mouse_state_t iState)
{
	if (!IsActive())
		return false;

	if (!iState)
	{
		if (m_bTransforming)
		{
			m_bTransforming = false;

			m_trsTransform = GetNewTRS();

			m_pListener->ManipulatorUpdated(m_sListenerArguments);

			return true;
		}

		return false;
	}

	int x, y;
	Application()->GetMousePosition(x, y);

	Vector vecPosition = GameServer()->GetRenderer()->WorldPosition(Vector((float)x, (float)y, 1));
	Vector vecCamera = GameServer()->GetRenderer()->GetCameraPosition();

	float flClosest = -1;
	m_iLockedAxis = -1;

	Matrix4x4 mTransform = m_trsTransform.GetMatrix4x4(false, false);

	float flScale = (float)(Vector(GameServer()->GetCameraManager()->GetCameraPosition()) - mTransform.GetTranslation()).Length()/10.0f;
	Vector vecX = (Vector(1, 0, 0)*flScale);
	Vector vecY = (Vector(0, 1, 0)*flScale);
	Vector vecZ = (Vector(0, 0, 1)*flScale);

	if (DistanceToLine(m_trsTransform.m_vecTranslation, vecPosition, vecCamera) < 0.2f*flScale)
	{
		m_flOriginalDistance = (m_trsTransform.m_vecTranslation - vecCamera).Length();
		m_iLockedAxis = 0;
	}

	if (DistanceToLine(mTransform*vecX, vecPosition, vecCamera) < 0.1f*flScale)
	{
		float flDistance = (mTransform*vecX - vecCamera).Length();
		if (m_iLockedAxis < 0 || flDistance < m_flOriginalDistance)
		{
			m_flOriginalDistance = flDistance;
			m_iLockedAxis = (1<<1)|(1<<2);
		}
	}

	if (DistanceToLine(mTransform*vecY, vecPosition, vecCamera) < 0.1f*flScale)
	{
		float flDistance = (mTransform*vecY - vecCamera).Length();
		if (m_iLockedAxis < 0 || flDistance < m_flOriginalDistance)
		{
			m_flOriginalDistance = flDistance;
			m_iLockedAxis = (1<<0)|(1<<2);
		}
	}

	if (DistanceToLine(mTransform*vecZ, vecPosition, vecCamera) < 0.1f*flScale)
	{
		float flDistance = (mTransform*vecZ - vecCamera).Length();
		if (m_iLockedAxis < 0 || flDistance < m_flOriginalDistance)
		{
			m_flOriginalDistance = flDistance;
			m_iLockedAxis = (1<<0)|(1<<1);
		}
	}

	if (m_iLockedAxis >= 0)
	{
		m_flStartX = (float)x;
		m_flStartY = (float)y;
		m_bTransforming = true;

		if (m_pListener && Application()->IsShiftDown())
			m_pListener->DuplicateMove("");

		return true;
	}

	return false;
}

void CManipulatorTool::Render()
{
	if (!IsActive())
		return;

	Matrix4x4 mTransform = GetTransform(false, false);

	float flScale = (float)(Vector(GameServer()->GetCameraManager()->GetCameraPosition()) - mTransform.GetTranslation()).Length()/10.0f;

	if (flScale < 0.001f)
		flScale = 0.001f;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ClearDepth();

	c.UseProgram("model");
	c.SetUniform("vecColor", Color(255, 255, 255, 255));
	c.SetUniform("bDiffuse", false);

	c.Transform(mTransform);
	c.Scale(flScale, flScale, flScale);

	Vector vecBox(0.1f, 0.1f, 0.1f);
	c.RenderWireBox(AABB(-vecBox, vecBox));

	c.BeginRenderLines();
		c.Vertex(Vector());
		c.Vertex(Vector(1, 0, 0));
		c.Vertex(Vector());
		c.Vertex(Vector(0, 1, 0));
		c.Vertex(Vector());
		c.Vertex(Vector(0, 0, 1));
	c.EndRender();

	Vector vecHandle(0.05f, 0.05f, 0.05f);
	c.Translate(Vector(1, 0, 0));
	c.SetUniform("vecColor", Color(255, 0, 0, 255));
	if (GetTransfromType() == MT_TRANSLATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(2, 1, 1), vecHandle * Vector(2, 1, 1)));
	else if (GetTransfromType() == MT_ROTATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(1, 1, 2), vecHandle * Vector(1, 1, 2)));
	else if (GetTransfromType() == MT_SCALE)
		c.RenderWireBox(AABB(-vecHandle, vecHandle));

	c.Translate(Vector(-1, 1, 0));
	c.SetUniform("vecColor", Color(0, 255, 0, 255));
	if (GetTransfromType() == MT_TRANSLATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(1, 2, 1), vecHandle * Vector(1, 2, 1)));
	else if (GetTransfromType() == MT_ROTATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(2, 1, 1), vecHandle * Vector(2, 1, 1)));
	else if (GetTransfromType() == MT_SCALE)
		c.RenderWireBox(AABB(-vecHandle, vecHandle));

	c.Translate(Vector(0, -1, 1));
	c.SetUniform("vecColor", Color(0, 0, 255, 255));
	if (GetTransfromType() == MT_TRANSLATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(1, 1, 2), vecHandle * Vector(1, 1, 2)));
	else if (GetTransfromType() == MT_ROTATE)
		c.RenderWireBox(AABB(-vecHandle * Vector(1, 2, 1), vecHandle * Vector(1, 2, 1)));
	else if (GetTransfromType() == MT_SCALE)
		c.RenderWireBox(AABB(-vecHandle, vecHandle));

	c.SetUniform("vecColor", Color(255, 255, 255, 255));
	c.SetUniform("bDiffuse", true);
}

Matrix4x4 CManipulatorTool::GetTransform(bool bRotation, bool bScaling)
{
	if (m_bTransforming)
		return GetNewTRS().GetMatrix4x4(bRotation, bScaling);

	return m_trsTransform.GetMatrix4x4(bRotation, bScaling);
}

TRS CManipulatorTool::GetNewTRS()
{
	int x, y;
	Application()->GetMousePosition(x, y);

	Vector vecCamera = GameServer()->GetRenderer()->GetCameraPosition();

	Vector vecOldPosition = GameServer()->GetRenderer()->WorldPosition(Vector(m_flStartX, m_flStartY, 1));
	Vector vecNewPosition = GameServer()->GetRenderer()->WorldPosition(Vector((float)x, (float)y, 1));

	Vector vecOldTranslation = vecCamera + (vecOldPosition - vecCamera).Normalized() * m_flOriginalDistance;
	Vector vecNewTranslation = vecCamera + (vecNewPosition - vecCamera).Normalized() * m_flOriginalDistance;

	TRS trs = m_trsTransform;

	if (GetTransfromType() == MT_TRANSLATE)
	{
		Vector vecTranslation = trs.m_vecTranslation + (vecNewTranslation - vecOldTranslation);

		if (!(m_iLockedAxis & (1<<0)))
			trs.m_vecTranslation.x = vecTranslation.x;
		if (!(m_iLockedAxis & (1<<1)))
			trs.m_vecTranslation.y = vecTranslation.y;
		if (!(m_iLockedAxis & (1<<2)))
			trs.m_vecTranslation.z = vecTranslation.z;
	}
	else if (GetTransfromType() == MT_ROTATE)
	{
		if (m_iLockedAxis == 0)
		{
			
		}
		else if (m_iLockedAxis == ((1<<0)|(1<<1)))
			trs.m_angRotation.r += (x - m_flStartX)+(y - m_flStartY);
		else if (m_iLockedAxis == ((1<<1)|(1<<2)))
			trs.m_angRotation.y += (x - m_flStartX)+(y - m_flStartY);
		else if (m_iLockedAxis == ((1<<0)|(1<<2)))
			trs.m_angRotation.p += (x - m_flStartX)+(y - m_flStartY);
	}
	else if (GetTransfromType() == MT_SCALE)
	{
		float flScale = (float)(Vector(GameServer()->GetCameraManager()->GetCameraPosition()) - trs.m_vecTranslation).Length()/10.0f;

		Vector vecTranslation = vecNewTranslation - vecOldTranslation;
		Vector vecScaling = RemapVal<Vector>(vecTranslation, Vector(0, 0, 0), Vector(1, 1, 1)*flScale, Vector(1, 1, 1), Vector(2, 2, 2));

		if (!(m_iLockedAxis & (1<<0)))
			trs.m_vecScaling.x *= vecScaling.x;
		if (!(m_iLockedAxis & (1<<1)))
			trs.m_vecScaling.y *= vecScaling.y;
		if (!(m_iLockedAxis & (1<<2)))
			trs.m_vecScaling.z *= vecScaling.z;
	}

	return trs;
}

void CManipulatorTool::TranslateModeCallback(const tstring& sArgs)
{
	SetTransfromType(MT_TRANSLATE);
}

void CManipulatorTool::RotateModeCallback(const tstring& sArgs)
{
	SetTransfromType(MT_ROTATE);
}

void CManipulatorTool::ScaleModeCallback(const tstring& sArgs)
{
	SetTransfromType(MT_SCALE);
}

CManipulatorTool* CManipulatorTool::s_pManipulatorTool = nullptr;

CManipulatorTool* CManipulatorTool::Get()
{
	if (!s_pManipulatorTool)
		s_pManipulatorTool = new CManipulatorTool();

	return s_pManipulatorTool;
}
