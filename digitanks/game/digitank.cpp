#include "digitank.h"

#include "maths.h"

CDigitank::CDigitank()
{
	m_flTotalPower = 10;
	m_flAttackPower = 3;
	m_flDefensePower = 4;
	m_flMovementPower = 3;

	m_flPreviewTurn = 0;

	m_bDesiredMove = false;
	m_bDesiredTurn = false;

	m_flTotalHealth = 10;
	m_flHealth = 10;

	m_flMaxShieldStrength = m_flFrontShieldStrength = m_flLeftShieldStrength = m_flRightShieldStrength = m_flBackShieldStrength = 5;

	m_pTeam = NULL;
}

float CDigitank::GetAttackPower(bool bPreview)
{
	float flMovementLength;
	
	if (bPreview)
	{
		flMovementLength = GetPreviewMoveTurnPower();

		if (flMovementLength > m_flTotalPower)
			return m_flAttackPower/m_flTotalPower;

		return RemapVal(flMovementLength, 0, m_flTotalPower, m_flAttackPower/(m_flAttackPower+m_flDefensePower), 0);
	}

	return m_flAttackPower/m_flTotalPower;
}

float CDigitank::GetDefensePower(bool bPreview)
{
	float flMovementLength;

	if (bPreview)
	{
		flMovementLength = GetPreviewMoveTurnPower();

		if (flMovementLength > m_flTotalPower)
			return m_flDefensePower/m_flTotalPower;

		return RemapVal(flMovementLength, 0, m_flTotalPower, m_flDefensePower/(m_flAttackPower+m_flDefensePower), 0);
	}

	return m_flDefensePower/m_flTotalPower;
}

float CDigitank::GetMovementPower(bool bPreview)
{
	float flMovementLength;
	
	if (bPreview)
	{
		flMovementLength = GetPreviewMoveTurnPower();

		if (flMovementLength > m_flTotalPower)
			return m_flMovementPower/m_flTotalPower;

		return flMovementLength/m_flTotalPower;
	}

	return m_flMovementPower/m_flTotalPower;
}

void CDigitank::SetAttackPower(float flAttackPower)
{
	if (flAttackPower > m_flTotalPower - m_flMovementPower)
		return;

	m_flAttackPower = flAttackPower;
	m_flDefensePower = m_flTotalPower - m_flMovementPower - m_flAttackPower;
}

float CDigitank::GetPreviewMoveTurnPower()
{
	return GetPreviewMovePower() + GetPreviewTurnPower();
}

float CDigitank::GetPreviewMovePower()
{
	if (HasDesiredMove())
		return (m_vecDesiredMove - GetOrigin()).Length();

	return (m_vecPreviewMove - GetOrigin()).Length();
}

float CDigitank::GetPreviewTurnPower()
{
	if (HasDesiredTurn())
		return fabs(AngleDifference(m_flDesiredTurn, GetAngles().y)/TurnPerPower());

	return fabs(AngleDifference(m_flPreviewTurn, GetAngles().y)/TurnPerPower());
}

void CDigitank::CalculateAttackDefense()
{
	m_flAttackPower = RemapVal(m_flMovementPower, 0, m_flTotalPower, m_flAttackPower/(m_flAttackPower+m_flDefensePower)*m_flTotalPower, 0);
	m_flDefensePower = m_flTotalPower - m_flMovementPower - m_flAttackPower;
}

float CDigitank::GetFrontShieldStrength()
{
	return m_flFrontShieldStrength/m_flMaxShieldStrength * GetDefensePower(true);
}

float CDigitank::GetLeftShieldStrength()
{
	return m_flLeftShieldStrength/m_flMaxShieldStrength * GetDefensePower(true);
}

float CDigitank::GetRightShieldStrength()
{
	return m_flRightShieldStrength/m_flMaxShieldStrength * GetDefensePower(true);
}

float CDigitank::GetRearShieldStrength()
{
	return m_flBackShieldStrength/m_flMaxShieldStrength * GetDefensePower(true);
}

float* CDigitank::GetShieldForAttackDirection(Vector vecAttack)
{
	Vector vecForward, vecRight;
	AngleVectors(GetAngles(), &vecForward, &vecRight, NULL);

	float flForwardDot = vecForward.Dot(vecAttack);
	float flRightDot = vecRight.Dot(vecAttack);

	if (flForwardDot > 0.5f)
		return &m_flFrontShieldStrength;
	else if (flForwardDot < -0.5f)
		return &m_flBackShieldStrength;
	else if (flRightDot > 0.5f)
		return &m_flRightShieldStrength;
	else
		return &m_flLeftShieldStrength;
}

void CDigitank::StartTurn()
{
	m_flMovementPower = 0;
	CalculateAttackDefense();

	for (size_t i = 0; i < TANK_SHIELDS; i++)
		m_flShieldStrengths[i] = Approach(m_flMaxShieldStrength, m_flShieldStrengths[i], ShieldRechargeRate());

	m_flHealth = Approach(m_flTotalHealth, m_flHealth, HealthRechargeRate());

	m_vecPreviewMove = GetOrigin();
	m_flPreviewTurn = GetAngles().y;
}

void CDigitank::ClearPreviewMove()
{
	m_vecPreviewMove = GetOrigin();
}

void CDigitank::ClearPreviewTurn()
{
	m_flPreviewTurn = GetAngles().y;
}

void CDigitank::SetDesiredMove()
{
	float flMovePower = GetPreviewMovePower();

	if (flMovePower > m_flTotalPower)
		return;

	m_vecDesiredMove = m_vecPreviewMove;

	m_flMovementPower = flMovePower;
	CalculateAttackDefense();

	m_bDesiredMove = true;
}

void CDigitank::CancelDesiredMove()
{
	m_bDesiredMove = false;

	m_flMovementPower = GetPreviewTurnPower();
	CalculateAttackDefense();

	ClearPreviewMove();
}

Vector CDigitank::GetDesiredMove()
{
	if (!HasDesiredMove())
		return GetOrigin();

	return m_vecDesiredMove;
}

void CDigitank::SetDesiredTurn()
{
	float flMovePower = GetPreviewMoveTurnPower();

	if (flMovePower > m_flTotalPower)
		return;

	m_flDesiredTurn = m_flPreviewTurn;

	m_flMovementPower = flMovePower;
	CalculateAttackDefense();

	m_bDesiredTurn = true;
}

void CDigitank::CancelDesiredTurn()
{
	m_bDesiredTurn = false;

	if (HasDesiredMove())
		m_flMovementPower = (GetOrigin() - GetDesiredMove()).Length();
	else
		m_flMovementPower = 0;

	CalculateAttackDefense();

	ClearPreviewTurn();
}

float CDigitank::GetDesiredTurn()
{
	if (!HasDesiredTurn())
		return GetAngles().y;

	return m_flDesiredTurn;
}

void CDigitank::Move()
{
	if (m_bDesiredMove)
	{
		m_bDesiredMove = false;
		SetOrigin(m_vecDesiredMove);
	}

	if (m_bDesiredTurn)
	{
		m_bDesiredTurn = false;
		SetAngles(EAngle(0, m_flDesiredTurn, 0));
	}
}

void CDigitank::Fire()
{
	if (m_hTarget == NULL)
		return;

	float flDistanceSqr = (m_hTarget->GetOrigin() - GetOrigin()).LengthSqr();
	if (flDistanceSqr > 50*50)
		return;

	bool bHit = false;
	float flDistance = sqrt(flDistanceSqr);

	if (flDistanceSqr < 30*30)
		bHit = true;
	else
		bHit = (rand()%100) > RemapVal(flDistance, 30, 50, 0, 100);

	if (bHit)
		m_hTarget->TakeDamage(this, m_flAttackPower);
}

void CDigitank::TakeDamage(CBaseEntity* pAttacker, float flDamage)
{
	Vector vecAttackDirection = (pAttacker->GetOrigin() - GetOrigin()).Normalized();

	float* pflShield = GetShieldForAttackDirection(vecAttackDirection);

	float flDamageBlocked = (*pflShield) * GetDefensePower();

	if (flDamage - flDamageBlocked <= 0)
	{
		*pflShield -= flDamage;
		return;
	}

	flDamage -= flDamageBlocked;

	*pflShield = 0;

	BaseClass::TakeDamage(pAttacker, flDamage);
}
