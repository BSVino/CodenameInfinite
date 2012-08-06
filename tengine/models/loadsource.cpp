#include "models.h"

#include <stdio.h>

#include <tinker_platform.h>
#include <files.h>

#include <modelconverter/modelconverter.h>
#include <renderer/renderer.h>
#include <textures/materiallibrary.h>
#include <tinker/application.h>
#include <datamanager/data.h>

tvector<tstring>			g_asTextures;
tvector<tvector<float> >	g_aaflData;
AABB						g_aabbVisBounds;
AABB						g_aabbPhysBounds;

void AddVertex(size_t iMaterial, const Vector& v, const Vector2D& vt)
{
	g_aaflData[iMaterial].push_back(v.x);
	g_aaflData[iMaterial].push_back(v.y);
	g_aaflData[iMaterial].push_back(v.z);
	g_aaflData[iMaterial].push_back(vt.x);
	g_aaflData[iMaterial].push_back(vt.y);

	for (int i = 0; i < 3; i++)
	{
		if (v[i] < g_aabbVisBounds.m_vecMins[i])
			g_aabbVisBounds.m_vecMins[i] = v[i];
		if (v[i] > g_aabbVisBounds.m_vecMaxs[i])
			g_aabbVisBounds.m_vecMaxs[i] = v[i];
	}
}

void LoadMeshInstanceIntoToy(CConversionScene* pScene, CConversionMeshInstance* pMeshInstance, const Matrix4x4& mParentTransformations)
{
	if (!pMeshInstance->IsVisible())
		return;

	CConversionMesh* pMesh = pMeshInstance->GetMesh();

	for (size_t m = 0; m < pScene->GetNumMaterials(); m++)
	{
		for (size_t j = 0; j < pMesh->GetNumFaces(); j++)
		{
			size_t k;
			CConversionFace* pFace = pMesh->GetFace(j);

			if (pFace->m == ~0)
				continue;

			CConversionMaterial* pMaterial = NULL;
			CConversionMaterialMap* pConversionMaterialMap = pMeshInstance->GetMappedMaterial(pFace->m);

			if (!pConversionMaterialMap)
				continue;

			if (!pConversionMaterialMap->IsVisible())
				continue;

			if (pConversionMaterialMap->m_iMaterial != m)
				continue;

			while (g_asTextures.size() <= pConversionMaterialMap->m_iMaterial)
			{
				g_asTextures.push_back(pScene->GetMaterial(pConversionMaterialMap->m_iMaterial)->GetDiffuseTexture());
				g_aaflData.push_back();
			}

			size_t iMaterial = pConversionMaterialMap->m_iMaterial;

			CConversionVertex* pVertex0 = pFace->GetVertex(0);

			for (k = 2; k < pFace->GetNumVertices(); k++)
			{
				CConversionVertex* pVertex1 = pFace->GetVertex(k-1);
				CConversionVertex* pVertex2 = pFace->GetVertex(k);

				AddVertex(iMaterial, mParentTransformations * pMesh->GetVertex(pVertex0->v), pMesh->GetUV(pVertex0->vu));
				AddVertex(iMaterial, mParentTransformations * pMesh->GetVertex(pVertex1->v), pMesh->GetUV(pVertex1->vu));
				AddVertex(iMaterial, mParentTransformations * pMesh->GetVertex(pVertex2->v), pMesh->GetUV(pVertex2->vu));
			}
		}
	}
}

void LoadSceneNodeIntoToy(CConversionScene* pScene, CConversionSceneNode* pNode, const Matrix4x4& mParentTransformations)
{
	if (!pNode)
		return;

	if (!pNode->IsVisible())
		return;

	Matrix4x4 mTransformations = mParentTransformations * pNode->m_mTransformations;

	for (size_t i = 0; i < pNode->GetNumChildren(); i++)
		LoadSceneNodeIntoToy(pScene, pNode->GetChild(i), mTransformations);

	for (size_t m = 0; m < pNode->GetNumMeshInstances(); m++)
		LoadMeshInstanceIntoToy(pScene, pNode->GetMeshInstance(m), mTransformations);
}

void LoadSceneIntoToy(CConversionScene* pScene)
{
	for (size_t i = 0; i < pScene->GetNumScenes(); i++)
		LoadSceneNodeIntoToy(pScene, pScene->GetScene(i), Matrix4x4());
}

bool CModel::LoadSourceFile()
{
	CConversionScene* pScene = new CConversionScene();
	CModelConverter c(pScene);

	if (!c.ReadModel(m_sFilename))
	{
		delete pScene;
		return false;
	}

	g_asTextures.clear();
	g_aaflData.clear();
	g_aabbVisBounds = AABB(Vector(999, 999, 999), Vector(-999, -999, -999));
	g_aabbPhysBounds = AABB(Vector(999, 999, 999), Vector(-999, -999, -999));

	LoadSceneIntoToy(pScene);

	size_t iTextures = g_asTextures.size();

	m_ahMaterials.resize(iTextures);
	m_aiVertexBuffers.resize(iTextures);
	m_aiVertexBufferSizes.resize(iTextures);

	for (size_t i = 0; i < iTextures; i++)
	{
		if (g_aaflData[i].size() == 0)
			continue;

		m_aiVertexBuffers[i] = CRenderer::LoadVertexDataIntoGL(g_aaflData[i].size()*4, &g_aaflData[i][0]);
		m_aiVertexBufferSizes[i] = g_aaflData[i].size()/5;

		CData oMaterialData;
		CData* pShader = oMaterialData.AddChild("Shader", "model");
		pShader->AddChild("Diffuse", g_asTextures[i]);
		m_ahMaterials[i] = CMaterialLibrary::AddMaterial(&oMaterialData);

		//TAssert(m_aiMaterials[i]);
		if (!m_ahMaterials[i].IsValid())
			TError(tstring("Couldn't create fake material for texture \"") + g_asTextures[i] + "\"\n");
	}

	m_aabbVisBoundingBox = g_aabbVisBounds;
	m_aabbPhysBoundingBox = g_aabbPhysBounds;

	delete pScene;

	return true;
}
