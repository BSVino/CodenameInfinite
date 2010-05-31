#ifndef DT_DISSOLVER_H

#include <vector>
#include <vector.h>
#include <matrix.h>

class CDissolveTri
{
public:
									CDissolveTri(const Vector& _v1, const Vector& _v2, const Vector& _v3, const Vector& _vt1, const Vector& _vt2, const Vector& _vt3);

public:
	void 							Reset();

public:
	bool							m_bActive;

	Matrix4x4						m_mPosition;
	Matrix4x4						m_mVelocity;

	float							m_flAlpha;
	float							m_flSpawnTime;

	size_t							m_iTexture;

	Vector							v1;
	Vector							vu1;
	Vector							v2;
	Vector							vu2;
	Vector							v3;
	Vector							vu3;
};

class CModelDissolver
{
public:
									CModelDissolver();

public:
	void							AddScene(class CModel* pModel, const Matrix4x4& mTransform);
	void							AddSceneNode(class CModel* pModel, class CConversionSceneNode* pNode, const Matrix4x4& mTransform);
	void							AddMeshInstance(class CModel* pModel, class CConversionMeshInstance* pMeshInstance, const Matrix4x4& mTransform);
	void							AddTriangle(class CConversionMeshInstance* pMeshInstance, class CConversionVertex* pV0, class CConversionVertex* pV1, class CConversionVertex* pV2, size_t iTexture, const Matrix4x4& mTransform);

	size_t							GetNumTriangles() { return m_iNumTrianglesAlive; };

public:
	static void						Simulate();
	static void						Render();

	static void						AddModel(class CBaseEntity* pEntity);

	static CModelDissolver*			Get() { return s_pModelDissolver; };

protected:
	std::vector<CDissolveTri>		m_aTriangles;

	size_t							m_iNumTrianglesAlive;

	float							m_flLifeTime;

private:
	static CModelDissolver*			s_pModelDissolver;
};

#endif