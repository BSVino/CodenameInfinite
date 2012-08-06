#include "physics_debugdraw.h"

#include <renderer/renderingcontext.h>
#include <game/gameserver.h>
#include <tinker/application.h>
#include <renderer/game_renderer.h>

btIDebugDraw* debugDrawerPtr;

CPhysicsDebugDrawer::CPhysicsDebugDrawer()
{
	// This is so that some of the more special debug commands inside bullet can have access to the debug drawer.
	debugDrawerPtr = this;
}

void CPhysicsDebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& fromColor, const btVector3& toColor)
{
	if (!m_bDrawing)
		return;

	CRenderingContext c(GameServer()->GetRenderer(), true);
	c.UseProgram("model");
	c.SetUniform("bDiffuse", false);
	c.SetUniform("vecColor", Color(Vector((const float*)fromColor)));
	c.BeginRenderDebugLines();
		c.Color(Color(Vector((const float*)fromColor)));
		c.Vertex(Vector((const float*)from));
		c.Color(Color(Vector((const float*)fromColor)));
		c.Vertex(Vector((const float*)to));
	c.EndRender();
}

void CPhysicsDebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
{
	if (!m_bDrawing)
		return;

	drawLine(from,to,color,color);
}

void CPhysicsDebugDrawer::drawSphere(btScalar radius, const btTransform& transform, const btVector3& color)
{
	if (!m_bDrawing)
		return;

	if (!GameServer()->GetRenderer()->IsSphereInFrustum(Vector((const float*)transform.getOrigin()), radius))
		return;

	BaseClass::drawSphere(radius, transform, color);
}

void CPhysicsDebugDrawer::drawCapsule(btScalar radius, btScalar halfHeight, int upAxis, const btTransform& transform, const btVector3& color)
{
	if (!m_bDrawing)
		return;

	if (!GameServer()->GetRenderer()->IsSphereInFrustum(Vector((const float*)transform.getOrigin()), halfHeight))
		return;

	BaseClass::drawCapsule(radius, halfHeight, upAxis, transform, color);
}

void CPhysicsDebugDrawer::drawBox (const btVector3& boxMin, const btVector3& boxMax, const btVector3& color, btScalar alpha)
{
	if (!m_bDrawing)
		return;

	TAssert(!"Unimplemented");
}

void CPhysicsDebugDrawer::drawTriangle(const btVector3& a,const btVector3& b,const btVector3& c,const btVector3& color,btScalar alpha)
{
	if (!m_bDrawing)
		return;

	CRenderingContext r(GameServer()->GetRenderer(), true);
	r.UseProgram("model");
	r.SetUniform("bDiffuse", false);
	r.SetColor(Color(Vector((const float*)color)));
	if (alpha < 1)
		r.SetBlend(BLEND_ALPHA);
	r.SetAlpha(alpha);

	const btVector3	n=btCross(b-a,c-a).normalized();

	r.BeginRenderTris();
	r.Normal(Vector((const float*)n));
	r.Vertex(Vector((const float*)a));
	r.Vertex(Vector((const float*)b));
	r.Vertex(Vector((const float*)c));
	r.EndRender();
}

void CPhysicsDebugDrawer::draw3dText(const btVector3& location,const char* textString)
{
	if (!m_bDrawing)
		return;

	TAssert(!"Unimplemented");
}

void CPhysicsDebugDrawer::reportErrorWarning(const char* warningString)
{
	if (!m_bDrawing)
		return;

	TMsg(sprintf(tstring("CPhysicsDebugDrawer: %s\n"), warningString));
}

void CPhysicsDebugDrawer::drawContactPoint(const btVector3& pointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
{
	if (!m_bDrawing)
		return;

	btVector3 to=pointOnB+normalOnB*1;//distance;
	const btVector3& from = pointOnB;

	CRenderingContext r(GameServer()->GetRenderer(), true);
	r.UseProgram("model");
	r.SetUniform("bDiffuse", false);
	r.SetColor(Color(Vector((const float*)color)));

	r.BeginRenderDebugLines();
	r.Vertex(Vector((const float*)from));
	r.Vertex(Vector((const float*)to));
	r.EndRender();
}
