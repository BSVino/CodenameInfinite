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

bool RayIntersectsQuad(const Ray& vecRay, const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3, Vector* pvecHit = NULL)
{
	if (RayIntersectsTriangle(vecRay, v0, v1, v2, pvecHit))
		return true;

	else return RayIntersectsTriangle(vecRay, v0, v2, v3, pvecHit);
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

	Vector vecLocalLocalCenter = (vecLocalCenter-vecPlayerEye).GetUnits(SCALE_METER);
	Vector v0 = vecLocalLocalCenter + vecMenuUp + vecMenuRight;
	Vector v1 = vecLocalLocalCenter - vecMenuUp + vecMenuRight;
	Vector v2 = vecLocalLocalCenter - vecMenuUp - vecMenuRight;
	Vector v3 = vecLocalLocalCenter + vecMenuUp - vecMenuRight;

	m_bMouseInMenu = RayIntersectsQuad(Ray(Vector(), vecDirection), v0, v1, v2, v3);

	if (!m_bMouseInMenu)
		return;

	m_iMouseInButton = -1;

	Vector vecButtonUp = vecUp * ButtonSize();
	Vector vecButtonRight = vecRight * ButtonSize();

	for (size_t i = 0; i < COMMAND_BUTTONS; i++)
	{
		Vector2D vecCenter = GetButtonCenter(i);

		Vector vec3DCenter = vecCenter.x * vecRight + vecCenter.y * vecUp;

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

		// Todo: draw unit name at the top.

		for (size_t i = 0; i < COMMAND_BUTTONS; i++)
		{
			if (!m_apButtons[i])
				continue;

			Vector2D vecCenter = GetButtonCenter(i);

			c.ResetTransformations();
			c.Translate(Vector(vecCenter.x, vecCenter.y, 0));

			c.UseMaterial("textures/commandmenubutton.mat");
			if (m_bMouseInMenu && m_iMouseInButton == i)
				c.SetUniform("flAlpha", 1.0f);
			else
				c.SetUniform("flAlpha", 0.4f);
			c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));

			c.RenderBillboard(CMaterialHandle("textures/commandmenubutton.mat"), ButtonSize(), Vector(0, 1, 0), Vector(1, 0, 0));

			float flTextWidth = glgui::CLabel::GetTextWidth(m_apButtons[i]->GetLabel(), -1, sFont, iFontSize);
			float flFontHeight = glgui::CLabel::GetFontHeight(sFont, iFontSize);

			c.ResetTransformations();
			c.Scale(0.003f, 0.003f, 0.003f);
			c.Translate(Vector(vecCenter.x*110000.0f, vecCenter.y*110000.0f - 15000, 0) + Vector(flTextWidth/2 * -300, flFontHeight/2 * 300, 0));

			c.UseProgram("text");

			if (m_bMouseInMenu && m_iMouseInButton == i)
				c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 1.0));
			else
				c.SetUniform("vecColor", Vector4D(1.0, 1.0, 1.0, 0.6f));
			c.SetUniform("flAlpha", 1.0f);

			c.RenderText(m_apButtons[i]->GetLabel(), -1, sFont, iFontSize);
		}
	}

	{
		CGameRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();

		Matrix4x4 mTransform = m_hOwner->BaseGetRenderTransform();
		mTransform += vecToPlayer * flProjectionDistance;
		mTransform.SetAngles(EAngle(0, 0, 0));
		c.Transform(mTransform);

		c.UseProgram("model");
		c.SetBlend(BLEND_ADDITIVE);
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

	// Close it if the player looks away.
	if (AngleVector(GetPlayerCharacter()->GetViewAngles()).Dot((GetOwner()->GetLocalOrigin()-GetPlayerCharacter()->GetLocalOrigin()).Normalized()) < 0.6f)
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
	flProjectionDistance = 0.2f;
	flProjectionRadius = 1.0f;
	Vector vecPlayerUp = m_hPlayerCharacter->GetLocalUpVector();
	vecProjectionDirection = (m_hPlayerCharacter->GetLocalOrigin() + m_hPlayerCharacter->EyeHeight() * DoubleVector(vecPlayerUp) - m_hOwner->GetLocalOrigin()).GetUnits(SCALE_METER).Normalized();
	vecRight = vecPlayerUp.Cross(vecProjectionDirection).Normalized();
	vecUp = vecProjectionDirection.Cross(vecRight).Normalized();
	vecLocalCenter = m_hOwner->GetLocalOrigin() + vecProjectionDirection * flProjectionDistance;
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
