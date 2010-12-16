#include "weaponpanel.h"

#include <game/digitanks/digitanksgame.h>
#include <renderer/renderer.h>

using namespace glgui;

CWeaponPanel::CWeaponPanel()
	: CPanel(0, 0, 300, 300)
{
	m_pInfo = new CLabel(0, 0, 100, 300, L"");
	AddControl(m_pInfo);

	m_iWeaponsSheet = CRenderer::LoadTextureIntoGL(L"textures/hud/hud-weapons-01.png");
}

void CWeaponPanel::Layout()
{
	CPanel::Layout();

	m_pInfo->SetSize(200, 200);
	m_pInfo->SetPos(GetWidth()+2, GetHeight()/2 - m_pInfo->GetHeight()/2);

	for (size_t i = 0; i < m_apWeapons.size(); i++)
	{
		m_apWeapons[i]->Destructor();
		m_apWeapons[i]->Delete();
	}

	m_apWeapons.clear();

	if (!DigitanksGame())
		return;

	CDigitanksTeam* pCurrentTeam = DigitanksGame()->GetCurrentLocalDigitanksTeam();
	if (!pCurrentTeam)
		return;

	CDigitank* pTank = pCurrentTeam->GetPrimarySelectionTank();
	if (!pTank)
		return;

	size_t iWeapons = pTank->GetNumWeapons();
	size_t iRows = (size_t)sqrt((float)iWeapons);

	int iButtonSize = GetWidth()/(iRows+1);

	if (iButtonSize > 64)
		iButtonSize = 64;

	int iXWidth = iButtonSize*20;
	int iYWidth = iButtonSize*20;

	int iPaddingSize = 10;

	int iMaxHeight = 0;

	size_t iWeapon = 0;
	for (size_t i = 0; i <= iRows; i++)
	{
		if (iWeapon >= pTank->GetNumWeapons())
			break;

		for (size_t j = 0; j <= iRows; j++)
		{
			if (iWeapon >= pTank->GetNumWeapons())
				break;

			weapon_t eWeapon = pTank->GetWeapon(iWeapon++);

			m_apWeapons.push_back(new CWeaponButton(this));
			CWeaponButton* pWeapon = m_apWeapons[m_apWeapons.size()-1];
			pWeapon->SetSize(iButtonSize-2, iButtonSize-1);
			pWeapon->SetPos(i*iButtonSize + iPaddingSize*i, j*iButtonSize + iPaddingSize*j);
			pWeapon->SetFontFaceSize(10);
			pWeapon->SetWeapon(eWeapon);
			AddControl(pWeapon);

			pWeapon->SetEnabled(true);
			pWeapon->SetFGColor(Color(0, 0, 0));
			pWeapon->SetClickedListener(pWeapon, &CWeaponButton::ChooseWeapon);

			SetTextureForWeapon(pWeapon, eWeapon);
			pWeapon->SetText(CProjectile::GetWeaponName(eWeapon));

			pWeapon->SetButtonColor(Color(100, 100, 100));

			int iHeight = (j+1)*iButtonSize + iPaddingSize*j;
			if (iHeight > iMaxHeight)
				iMaxHeight = iHeight;
		}
	}

	SetSize(300, iMaxHeight);

	SetPos(CRootPanel::Get()->GetWidth()/2 + 100, CRootPanel::Get()->GetHeight()/2-GetHeight()/2);

	UpdateInfo(WEAPON_NONE);
}

void CWeaponPanel::Paint(int x, int y, int w, int h)
{
	if (m_pInfo->GetText().length() > 1)
	{
		int ix, iy, iw, ih;
		m_pInfo->GetAbsDimensions(ix, iy, iw, ih);
		CRootPanel::PaintRect(ix, iy, iw, ih, Color(0, 0, 0, GetAlpha()));
	}

	CPanel::Paint(x, y, w, h);
}

void CWeaponPanel::UpdateInfo(weapon_t eWeapon)
{
	if (!eWeapon)
	{
		m_pInfo->SetText("");
		return;
	}

	eastl::string16 s;
	eastl::string16 p;

	eastl::string16 sName = CProjectile::GetWeaponName(eWeapon);
	sName.make_upper();

	s += sName + L"\n \n";
	s += eastl::string16(CProjectile::GetWeaponDescription(eWeapon)) + L"\n \n";
	s += p.sprintf(L"Attack Energy Required: %d%%\n", ((int)CProjectile::GetWeaponEnergy(eWeapon)*10));
	if (eWeapon == WEAPON_CHARGERAM)
		s += p.sprintf(L"Movement energy required: %d%%\n", ((int)CProjectile::GetWeaponEnergy(eWeapon)*10));
	s += p.sprintf(L"Damage: %.1f\n", CProjectile::GetWeaponDamage(eWeapon));

	m_pInfo->SetText(s);

	m_pInfo->SetSize(m_pInfo->GetWidth(), 9999);
	m_pInfo->SetSize(m_pInfo->GetWidth(), (int)m_pInfo->GetTextHeight()+20);
}

void CWeaponPanel::SetTextureForWeapon(CWeaponButton* pWeapon, weapon_t eWeapon)
{
	int iTextureWidth = 512;
	int iTextureHeight = 256;

	switch (eWeapon)
	{
	default:
	case WEAPON_NONE:
	case PROJECTILE_SMALL:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 0, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_MEDIUM:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 64, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_LARGE:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 128, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_AOE:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 192, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_EMP:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 256, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_ICBM:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 320, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_GRENADE:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 384, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_DAISYCHAIN:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 448, 0, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_CLUSTERBOMB:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 0, 64, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_SPLOOGE:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 64, 64, 64, 64, iTextureWidth, iTextureHeight);
		break;

	case PROJECTILE_TRACTORBOMB:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 128, 64, 64, 64, iTextureWidth, 256);
		break;

	case PROJECTILE_EARTHSHAKER:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 192, 64, 64, 64, iTextureWidth, 256);
		break;

	case PROJECTILE_CAMERAGUIDED:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 256, 64, 64, 64, iTextureWidth, 256);
		break;

	case WEAPON_LASER:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 320, 64, 64, 64, iTextureWidth, 256);
		break;

	case WEAPON_CHARGERAM:
		pWeapon->SetSheetTexture(m_iWeaponsSheet, 384, 64, 64, 64, iTextureWidth, 256);
		break;
	}
}

CWeaponButton::CWeaponButton(CWeaponPanel* pPanel)
	: CPictureButton(L"")
{
	m_pWeaponPanel = pPanel;
	m_eWeapon = WEAPON_NONE;
}

void CWeaponButton::SetWeapon(weapon_t eWeapon)
{
	m_eWeapon = eWeapon;
}

void CWeaponButton::CursorIn()
{
	CPictureButton::CursorIn();

	m_pWeaponPanel->UpdateInfo(m_eWeapon);
}

void CWeaponButton::ChooseWeaponCallback()
{
	CDigitanksTeam* pTeam = DigitanksGame()->GetCurrentLocalDigitanksTeam();

	for (size_t i = 0; i < pTeam->GetNumTanks(); i++)
	{
		CDigitank* pTank = pTeam->GetTank(i);
		if (!pTank || !pTeam->IsSelected(pTank))
			continue;

		pTank->SetCurrentWeapon(m_eWeapon);
	}

	m_pWeaponPanel->SetVisible(false);

	DigitanksGame()->SetControlMode(MODE_AIM);

	if (m_eWeapon == WEAPON_CHARGERAM)
		DigitanksGame()->SetAimType(AIM_MOVEMENT);
	else if (m_eWeapon == WEAPON_LASER)
		DigitanksGame()->SetAimType(AIM_NORANGE);
	else if (m_eWeapon == PROJECTILE_CAMERAGUIDED)
		DigitanksGame()->SetAimType(AIM_NORANGE);
	else
		DigitanksGame()->SetAimType(AIM_NORMAL);

	CRootPanel::Get()->Layout();
}
