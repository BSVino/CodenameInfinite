#include <FCollada.h>
#include <FCDocument/FCDocument.h>
#include <FCDocument/FCDLibrary.h>
#include <FCDocument/FCDMaterial.h>
#include <FCDocument/FCDGeometry.h>
#include <FCDocument/FCDGeometryMesh.h>
#include <FCDocument/FCDGeometrySource.h>
#include <FCDocument/FCDGeometryPolygons.h>
#include <FCDocument/FCDGeometryPolygonsInput.h>

#include "modelconverter.h"

void CModelConverter::ReadDAE(const char* pszFilename)
{
	FCollada::Initialize();

	size_t iOriginalSize = strlen(pszFilename) + 1;
	const size_t iNewSize = 100;
	size_t iConvertedChars = 0;
	wchar_t wszNewString[iNewSize];
	mbstowcs_s(&iConvertedChars, wszNewString, iOriginalSize, pszFilename, _TRUNCATE);

	FCDocument* pDoc = FCollada::NewTopDocument();
	if (FCollada::LoadDocumentFromFile(pDoc, wszNewString))
	{
		size_t i;

		FCDMaterialLibrary* pMatLib = pDoc->GetMaterialLibrary();
		size_t iEntities = pMatLib->GetEntityCount();
		for (i = 0; i < iEntities; ++i)
		{
			FCDMaterial* pMaterial = pMatLib->GetEntity(i);
			m_Scene.AddMaterial(pMaterial->GetDaeId().c_str());
		}

		FCDGeometryLibrary* pGeoLib = pDoc->GetGeometryLibrary();
		iEntities = pGeoLib->GetEntityCount();
		for (i = 0; i < iEntities; ++i)
		{
			FCDGeometry* pGeometry = pGeoLib->GetEntity(i);
			if (pGeometry->IsMesh())
			{
				size_t j;

				size_t iMesh = m_Scene.AddMesh("mesh");
				CConversionMesh* pMesh = m_Scene.GetMesh(iMesh);
				pMesh->AddBone("mesh");

				FCDGeometryMesh* pGeoMesh = pGeometry->GetMesh();
				FCDGeometrySource* pPositionSource = pGeoMesh->GetPositionSource();
				size_t iVertexCount = pPositionSource->GetValueCount();

				for (j = 0; j < iVertexCount; j++)
				{
					const float* pflValues = pPositionSource->GetValue(j);
					pMesh->AddVertex(pflValues[0], pflValues[1], pflValues[2]);
				}

				FCDGeometrySource* pNormalSource = pGeoMesh->FindSourceByType(FUDaeGeometryInput::NORMAL);
				iVertexCount = pNormalSource->GetValueCount();

				for (j = 0; j < iVertexCount; j++)
				{
					const float* pflValues = pNormalSource->GetValue(j);
					pMesh->AddNormal(pflValues[0], pflValues[1], pflValues[2]);
				}

				FCDGeometrySource* pUVSource = pGeoMesh->FindSourceByType(FUDaeGeometryInput::TEXCOORD);
				iVertexCount = pUVSource->GetValueCount();

				for (j = 0; j < iVertexCount; j++)
				{
					const float* pflValues = pUVSource->GetValue(j);
					pMesh->AddUV(pflValues[0], pflValues[1]);
				}

				for (j = 0; j < pGeoMesh->GetPolygonsCount(); j++)
				{
					FCDGeometryPolygons* pPolygons = pGeoMesh->GetPolygons(j);
					FCDGeometryPolygonsInput* pPositionInput = pPolygons->FindInput(FUDaeGeometryInput::POSITION);
					FCDGeometryPolygonsInput* pNormalInput = pPolygons->FindInput(FUDaeGeometryInput::NORMAL);
					FCDGeometryPolygonsInput* pUVInput = pPolygons->FindInput(FUDaeGeometryInput::TEXCOORD);

					size_t iPositionCount = pPositionInput->GetIndexCount();
					uint32* pPositions = pPositionInput->GetIndices();

					size_t iNormalCount = pNormalInput->GetIndexCount();
					uint32* pNormals = pNormalInput->GetIndices();

					size_t iUVCount = pUVInput->GetIndexCount();
					uint32* pUVs = pUVInput->GetIndices();

					fm::stringT<fchar> sMaterial = pPolygons->GetMaterialSemantic();

					iOriginalSize = wcslen(sMaterial.c_str()) + 1;
					size_t iConvertedChars = 0;
					char szNewString[iNewSize];
					wcstombs_s(&iConvertedChars, szNewString, iOriginalSize, sMaterial.c_str(), _TRUNCATE);

					size_t iCurrentMaterial = m_Scene.FindMaterial(szNewString);

					if (pPolygons->TestPolyType() == 3)
					{
						// All triangles!
						for (size_t k = 0; k < iPositionCount; k+=3)
						{
							size_t iFace = pMesh->AddFace(0);

							pMesh->AddVertexToFace(iFace, pPositions[k+0], pUVs[k+0], pNormals[k+0]);
							pMesh->AddVertexToFace(iFace, pPositions[k+1], pUVs[k+1], pNormals[k+1]);
							pMesh->AddVertexToFace(iFace, pPositions[k+2], pUVs[k+2], pNormals[k+2]);
						}
					}
					else if (pPolygons->TestPolyType() == 4)
					{
						// All quads!
						for (size_t k = 0; k < iPositionCount; k+=4)
						{
							size_t iFace = pMesh->AddFace(iCurrentMaterial);

							pMesh->AddVertexToFace(iFace, pPositions[k+0], pUVs[k+0], pNormals[k+0]);
							pMesh->AddVertexToFace(iFace, pPositions[k+1], pUVs[k+1], pNormals[k+1]);
							pMesh->AddVertexToFace(iFace, pPositions[k+2], pUVs[k+2], pNormals[k+2]);
							pMesh->AddVertexToFace(iFace, pPositions[k+3], pUVs[k+3], pNormals[k+3]);
						}
					}
				}
			}
		}
	}
	else
		printf("Oops! Some kind of error happened!\n");

	for (size_t i = 0; i < m_Scene.GetNumMeshes(); i++)
	{
		m_Scene.GetMesh(i)->TranslateOrigin();
	}

	pDoc->Release();

	FCollada::Release();
}

