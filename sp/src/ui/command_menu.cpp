#include "command_menu.h"

#include <geometry.h>

#include <glgui/label.h>

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "entities/sp_player.h"
#include "sp_renderer.h"
#include "entities/bots/bot.h"
#include "entities/structures/structure.h"

CCommandMenu::CCommandMenu(CBaseEntity* pOwner, CPlayerCharacter* pCharacter)
{
	m_hOwner = pOwner;
	m_hPlayerCharacter = pCharacter;

	memset(m_apButtons, 0, sizeof(m_apButtons));

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();
	if (pPlayer->GetPlayerCharacter() == pCharacter)
		pPlayer->CommandMenuOpened(this);

	m_bMouseInMenu = false;

	m_flCurrentProgress = m_flMaxProgress = 0;
}

CCommandMenu::~CCommandMenu()
{
	for (size_t i = 0; i < COMMAND_BUTTONS; i++)
		delete m_apButtons[i];

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();
	pPlayer->CommandMenuClosed(this);
}

void CCommandMenu::SetButton(size_t i, const tstring& sLabel, const tstring& sCommand)
{
	TAssert(i < COMMAND_BUTTONS);
	if (i >= COMMAND_BUTTONS)
		return;

	if (!m_apButtons[i])
		m_apButtons[i] = new CCommandButton();

	m_apButtons[i]->SetLabel(sLabel);
	m_apButtons[i]->SetCommand(sCommand);
}

void CCommandMenu::SetButtonEnabled(size_t i, bool bEnabled)
{
	TAssert(i < COMMAND_BUTTONS);
	if (i >= COMMAND_BUTTONS)
		return;

	if (!m_apButtons[i])
		m_apButtons[i] = new CCommandButton();

	m_apButtons[i]->SetEnabled(bEnabled);
}

void CCommandMenu::SetButtonToolTip(size_t i, const tstring& sToolTip)
{
	TAssert(i < COMMAND_BUTTONS);
	if (i >= COMMAND_BUTTONS)
		return;

	if (!m_apButtons[i])
		m_apButtons[i] = new CCommandButton();

	m_apButtons[i]->SetToolTip(sToolTip);
}

void CCommandMenu::SetProgressBar(double flCurrent, double flMax)
{
	m_flCurrentProgress = flCurrent;
	m_flMaxProgress = flMax;
}

void CCommandMenu::DisableProgressBar()
{
	m_flCurrentProgress = m_flMaxProgress = 0;
}

void CCommandMenu::Think()
{
	Vector vecToPlayer, vecPlayerUp, vecRight, vecUp;
	float flProjectionDistance, flProjectionRadius;
	TVector vecLocalCenter;
	GetVectors(vecLocalCenter, vecToPlayer, vecUp, vecRight, flProjectionDistance, flProjectionRadius);

	Vector vecMenuUp = vecUp * flProjectionRadius;
	Vector vecMenuRight = vecRight * flProjectionRadius;

	TVector vecPlayerEye = m_hPlayerCharacter->GetLocalOrigin() + m_hPlayerCharacter->EyeHeight() * DoubleVector(m_hPlayerCharacter->GetLocalUpVector());
	Vector vecDirection = AngleVector(m_hPlayerCharacter->GetViewAngles());

	Vector vecLocalLocalCenter = (vecLocalCenter-vecPlayerEye).GetMeters();
	Vector v0 = vecLocalLocalCenter + vecMenuUp + vecMenuRight;
	Vector v1 = vecLocalLocalCenter - vecMenuUp + vecMenuRight;
	Vector v2 = vecLocalLocalCenter - vecMenuUp - vecMenuRight;
	Vector v3 = vecLocalLocalCenter + vecMenuUp - vecMenuRight;

	m_bMouseInMenu = RayIntersectsQuad(Ray(Vector(), vecDirection), v0, v1, v2, v3);

	if (!m_bMouseInMenu)
		return;

	m_iMouseInButton = -1;

	Vector vecButtonUp = vecMenuUp * ButtonSize();
	Vector vecButtonRight = vecMenuRight * ButtonSize();

	for (size_t i = 0; i < COMMAND_BUTTONS; i++)
	{
		Vector2D vecCenter = GetButtonCenter(i);

		Vector vec3DCenter = vecCenter.x * vecMenuRight + vecCenter.y * vecMenuUp;

		Vector v0 = vecLocalLocalCenter + vec3DCenter + vecButtonUp + vecButtonRight;
		Vector v1 = vecLocalLocalCenter + vec3DCenter - vecButtonUp + vecButtonRight;
		Vector v2 = vecLocalLocalCenter + vec3DCenter - vecButtonUp - vecButtonRight;
		Vector v3 = vecLocalLocalCenter + vec3DCenter + vecButtonUp - vecButtonRight;

		if (RayIntersectsQuad(Ray(Vector(), vecDirection), v0, v1, v2, v3))
		{
			m_iMouseInButton = i;
			return;
		}
	}
}

void CCommandMenu::Render() const
{
	if (!m_hOwner || !m_hPlayerCharacter)
		return;

	tstring sFont = "sans-serif";
	int iFontSize = 42;
	float flFontHeight = glgui::CLabel::GetFontHeight(sFont, iFontSize);

	int iSmallFontSize = 26;
	float flSmallFontHeight = glgui::CLabel::GetFontHeight(sFont, iSmallFontSize);

	Vector vecToPlayer, vecPlayerUp, vecRight, vecUp;
	float flProjectionDistance, flProjectionRadius;
	TVector vecLocalCenter;
	GetVectors(vecLocalCenter, vecToPlayer, vecUp, vecRight, flProjectionDistance, flProjectionRadius);

	{
		CRenderingContext c(GameServer()->GetRenderer());

		c.UseFrameBuffer(SPGame()->GetSPRenderer()->GetCommandMenuBuffer());
		if (m_bMouseInMenu)
			c.ClearColor(Color(155, 155, 155, 15));
		else
			c.ClearColor(Color(155, 155, 155, 5));

		if (m_sTitle.length())
		{
			c.UseProgram("text");
			c.SetBlend(BLEND_ALPHA);

			c.SetUniform("vecColor", Vector4D(1, 1, 1, 1));
			c.SetUniform("flAlpha", 1.0f);

			float flTextWidth = glgui::CLabel::GetTextWidth(m_sTitle, -1, sFont, iFontSize);

			c.ResetTransformations();
			c.Scale(0.003f, 0.003f, 0.003f);
			c.Translate(Vector(0, 80000, 0) + Vector(flTextWidth/2 * -300, flFontHeight/2 * 300, 0));

			c.RenderText(m_sTitle, -1, sFont, iFontSize);

			if (m_sSubtitle.length())
			{
				float flTextWidth = glgui::CLabel::GetTextWidth(m_sSubtitle, -1, sFont, iSmallFontSize);

				c.ResetTransformations();
				c.Scale(0.003f, 0.003f, 0.003f);
				c.Translate(Vector(0, 70000, 0) + Vector(flTextWidth/2 * -300, flSmallFontHeight/2 * 300, 0));

				c.RenderText(m_sSubtitle, -1, sFont, iSmallFontSize);
			}
		}

		for (size_t i = 0; i < COMMAND_BUTTONS; i++)
		{
			if (!m_apButtons[i])
				continue;

			Vector2D vecCenter = GetButtonCenter(i);

			c.ResetTransformations();
			c.Translate(Vector(vecCenter.x, vecCenter.y, 0));

			c.UseMaterial("textures/commandmenubutton.mat");

			float flAlpha = 1.0f;

			if (m_bMouseInMenu && m_iMouseInButton == i)
				flAlpha = 1.0f;
			else
				flAlpha = 0.4f;

			c.SetUniform("flAlpha", flAlpha);

			Vector4D vecColor(1, 1, 1, 1);

			if (!m_apButtons[i]->IsEnabled())
				vecColor = Vector4D(1, 0, 0, 1);

			c.SetUniform("vecColor", vecColor);

			c.RenderBillboard(CMaterialHandle("textures/commandmenubutton.mat"), ButtonSize(), Vector(0, 1, 0), Vector(1, 0, 0));

			float flTextWidth = glgui::CLabel::GetTextWidth(m_apButtons[i]->GetLabel(), -1, sFont, iFontSize);

			c.ResetTransformations();
			c.Scale(0.003f, 0.003f, 0.003f);
			c.Translate(Vector(vecCenter.x*110000.0f, vecCenter.y*110000.0f - 15000, 0) + Vector(flTextWidth/2 * -300, flFontHeight/2 * 300, 0));

			c.UseProgram("text");

			if (m_bMouseInMenu && m_iMouseInButton == i)
				vecColor = Vector4D(1, 1, 1, 1);
			else
				vecColor = Vector4D(1, 1, 1, 0.6f);

			if (!m_apButtons[i]->IsEnabled())
				vecColor = Vector4D(1, 0, 0, 0.6f);

			c.SetUniform("vecColor", vecColor);

			c.SetUniform("flAlpha", 1.0f);

			c.RenderText(m_apButtons[i]->GetLabel(), -1, sFont, iFontSize);
		}

		if (m_flMaxProgress)
		{
			c.ResetTransformations();

			c.RenderBillboard(CMaterialHandle("textures/commandmenunotice.mat"), 1, Vector(0, 0.1f, 0), Vector(0.9f, 0, 0));

			c.SetBlend(BLEND_NONE);
			c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));
			c.RenderBillboard(CMaterialHandle("textures/commandmenunotice.mat"), 1, Vector(0, 0.06f, 0), Vector((float)(0.85 * m_flCurrentProgress/m_flMaxProgress), 0, 0));

			tstring sProgress = sprintf("%d%% complete", (int)(100*m_flCurrentProgress/m_flMaxProgress));

			float flTextWidth = glgui::CLabel::GetTextWidth(sProgress, -1, sFont, iSmallFontSize);

			c.UseProgram("text");
			c.ResetTransformations();
			c.Scale(0.003f, 0.003f, 0.003f);
			c.Translate(Vector(0, -7000, 0) + Vector(flTextWidth/2 * -300, flSmallFontHeight/2 * 300, 0));

			c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));
			c.SetUniform("flAlpha", 1.0f);
			c.RenderText(sProgress, -1, sFont, iSmallFontSize);
		}

		if (m_bMouseInMenu && m_iMouseInButton >= 0 && m_iMouseInButton < COMMAND_BUTTONS)
		{
			if (m_apButtons[m_iMouseInButton] && m_apButtons[m_iMouseInButton]->GetToolTip().length())
			{
				c.ResetTransformations();

				c.RenderBillboard(CMaterialHandle("textures/commandmenunotice.mat"), 1, Vector(0, 0.2f, 0), Vector(0.9f, 0, 0));

				float flTextWidth = glgui::CLabel::GetTextWidth(m_apButtons[m_iMouseInButton]->GetToolTip(), -1, sFont, iFontSize);

				c.UseProgram("text");
				c.ResetTransformations();
				c.Scale(0.003f, 0.003f, 0.003f);
				c.Translate(Vector(0, -15000, 0) + Vector(flTextWidth/2 * -300, flFontHeight/2 * 300, 0));

				c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));
				c.SetUniform("flAlpha", 1.0f);
				c.RenderText(m_apButtons[m_iMouseInButton]->GetToolTip(), -1, sFont, iFontSize);
			}
		}
	}

	{
		CGameRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();

		Matrix4x4 mTransform = m_hOwner->BaseGetRenderTransform();
		mTransform += m_hOwner->GetLocalTransform().TransformVector(m_hOwner->GameData().GetCommandMenuRenderOffset());
		mTransform += vecToPlayer * flProjectionDistance;
		mTransform.SetAngles(EAngle(0, 0, 0));
		c.Transform(mTransform);

		c.UseProgram("model");
		c.SetBlend(BLEND_ADDITIVE);
		c.SetDepthMask(false);
		c.SetUniform("flAlpha", 1.0f);
		c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));

		c.BindBufferTexture(*SPGame()->GetSPRenderer()->GetCommandMenuBuffer());

		vecRight *= flProjectionRadius;
		vecUp *= flProjectionRadius;

		c.BeginRenderTriFan();
			c.TexCoord(0.0f, 0.0f);
			c.Vertex(-vecRight + vecUp);
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(-vecRight - vecUp);
			c.TexCoord(1.0f, 1.0f);
			c.Vertex(vecRight - vecUp);
			c.TexCoord(1.0f, 0.0f);
			c.Vertex(vecRight + vecUp);
		c.EndRender();
	}
}

bool CCommandMenu::MouseInput(int iButton, int iState)
{
	if (m_bMouseInMenu)
	{
		if (m_iMouseInButton < COMMAND_BUTTONS && m_apButtons[m_iMouseInButton])
		{
			const tstring& sCommand = m_apButtons[m_iMouseInButton]->GetCommand();
			CBot* pBot = dynamic_cast<CBot*>(GetOwner());
			if (pBot)
				pBot->MenuCommand(sCommand);
			else
			{
				CStructure* pStructure = dynamic_cast<CStructure*>(GetOwner());
				if (pStructure)
					pStructure->MenuCommand(sCommand);
				else
					TAssert(!"Invalid command menu entity");
			}
		}
	}

	return m_bMouseInMenu;
}

bool CCommandMenu::WantsToClose() const
{
	// Close it if the player died or disconnected or something.
	if (!GetPlayerCharacter())
		return true;

	// Not sure how this could happen but whatever.
	if (!GetOwner())
		return true;

	TVector vecLocalCenter;
	Vector vecProjectionDirection, vecUp, vecRight;
	float flProjectionDistance, flProjectionRadius;
	GetVectors(vecLocalCenter, vecProjectionDirection, vecUp, vecRight, flProjectionDistance, flProjectionRadius);

	// Close it if the player looks away.
	if (AngleVector(GetPlayerCharacter()->GetViewAngles()).Dot(vecProjectionDirection) > -0.6f)
		return true;

	if ((GetOwner()->GetLocalOrigin()-GetPlayerCharacter()->GetLocalOrigin()).LengthSqr() > TFloat(5).Squared())
		return true;

	return false;
}

size_t CCommandMenu::GetNumActiveButtons() const
{
	size_t iButtons = 0;
	for (size_t i = 0; i < COMMAND_BUTTONS; i++)
	{
		if (m_apButtons[i])
			iButtons++;
	}

	return iButtons;
}

void CCommandMenu::GetVectors(TVector& vecLocalCenter, Vector& vecProjectionDirection, Vector& vecUp, Vector& vecRight, float& flProjectionDistance, float& flProjectionRadius) const
{
	flProjectionDistance = m_hOwner->GameData().GetCommandMenuProjectionDistance();
	flProjectionRadius = m_hOwner->GameData().GetCommandMenuProjectionRadius();
	Vector vecPlayerUp = m_hPlayerCharacter->GetLocalUpVector();

	TVector vecPlayerEyes = m_hPlayerCharacter->GetLocalOrigin() + m_hPlayerCharacter->EyeHeight() * DoubleVector(vecPlayerUp);
	TVector vecOwnerProjectionPoint = m_hOwner->GetLocalOrigin() + m_hOwner->GetLocalTransform().TransformVector(m_hOwner->GameData().GetCommandMenuRenderOffset());

	Vector vecToPlayerEyes = (vecPlayerEyes - vecOwnerProjectionPoint).GetMeters();
	float flDistanceToPlayerEyes = vecToPlayerEyes.Length();
	vecProjectionDirection = vecToPlayerEyes/flDistanceToPlayerEyes;

	flProjectionDistance = RemapValClamped(flDistanceToPlayerEyes, 1, 2.5f, flProjectionDistance*2/3, flProjectionDistance);
	flProjectionRadius = RemapValClamped(flDistanceToPlayerEyes, 1, 2.5f, flProjectionRadius/4, flProjectionRadius);

	vecRight = vecPlayerUp.Cross(vecProjectionDirection).Normalized();
	vecUp = vecProjectionDirection.Cross(vecRight).Normalized();
	vecLocalCenter = vecOwnerProjectionPoint + vecProjectionDirection * flProjectionDistance;
}

Vector2D CCommandMenu::GetButtonCenter(size_t iButton) const
{
	size_t j = iButton%(COMMAND_BUTTONS/2);
	float flRight = (iButton < COMMAND_BUTTONS/2)?-1.0f: 1.0f;

	Vector2D vecCenter;
	vecCenter.x = flRight * 0.6f;
	vecCenter.y = 0.6f - (float)j*0.6f;

	return vecCenter;
}

CCommandButton::CCommandButton()
{
	m_bEnabled = true;
}
