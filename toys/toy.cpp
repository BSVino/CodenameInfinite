#include "toy.h"

#include <stdlib.h>
#include <stdio.h>

#include <common.h>
#include <tstring.h>

#include <shell.h>

#include "toy_offsets.h"

AABB CToy::s_aabbBoxDimensions = AABB(-Vector(0.5f, 0.5f, 0.5f), Vector(0.5f, 0.5f, 0.5f));

CToy::CToy()
{
	m_pBase = nullptr;
	m_pMesh = nullptr;
	m_pPhys = nullptr;
	m_pArea = nullptr;
}

CToy::~CToy()
{
	if (m_pBase)
		free(m_pBase);

	if (m_pMesh)
		free(m_pMesh);

	if (m_pPhys)
		free(m_pPhys);

	if (m_pArea)
		free(m_pArea);
}

const AABB& CToy::GetVisBounds()
{
	if (!m_pBase)
	{
		static AABB aabb;
		return aabb;
	}

	return *((AABB*)(m_pBase+TOY_HEADER_SIZE));
}

const AABB& CToy::GetPhysBounds()
{
	if (!m_pBase)
	{
		static AABB aabb;
		return aabb;
	}

	return *((AABB*)(m_pBase+TOY_HEADER_SIZE+BASE_AABB_SIZE));
}

size_t CToy::GetNumMaterials()
{
	if (!m_pBase)
		return 0;

	return (int)*((uint8_t*)(m_pBase+TOY_HEADER_SIZE+BASE_AABB_SIZE+BASE_AABB_SIZE));
}

size_t CToy::GetMaterialNameLength(size_t i)
{
	return (int)*((uint16_t*)GetMaterial(i));
}

char* CToy::GetMaterialName(size_t i)
{
	return GetMaterial(i)+MESH_MATERIAL_TEXNAME_LENGTH_SIZE;
}

size_t CToy::GetMaterialNumVerts(size_t i)
{
	size_t iMaterialTableEntry = TOY_HEADER_SIZE+BASE_AABB_SIZE+BASE_AABB_SIZE+BASE_MATERIAL_TABLE_SIZE+i*BASE_MATERIAL_TABLE_STRIDE;
	size_t iMaterialVertCount = (size_t)*((size_t*)(m_pBase+iMaterialTableEntry+BASE_MATERIAL_TABLE_OFFSET_SIZE));
	return iMaterialVertCount;
}

float* CToy::GetMaterialVerts(size_t iMaterial)
{
	char* pVerts = GetMaterial(iMaterial)+MESH_MATERIAL_TEXNAME_LENGTH_SIZE+GetMaterialNameLength(iMaterial);
	return (float*)pVerts;
}

float* CToy::GetMaterialVert(size_t iMaterial, size_t iVert)
{
	char* pVerts = GetMaterial(iMaterial)+MESH_MATERIAL_TEXNAME_LENGTH_SIZE+GetMaterialNameLength(iMaterial);
	return (float*)(pVerts + iVert*MESH_MATERIAL_VERTEX_SIZE);
}

size_t CToy::GetNumSceneAreas()
{
	if (!m_pBase)
		return 0;

	size_t iSceneTable = TOY_HEADER_SIZE+BASE_AABB_SIZE+BASE_AABB_SIZE+BASE_MATERIAL_TABLE_SIZE+GetNumMaterials()*BASE_MATERIAL_TABLE_STRIDE;
	return (int)*((uint32_t*)(m_pBase+iSceneTable));
}

size_t CToy::GetVertexSize()
{
	return MESH_MATERIAL_VERTEX_SIZE;
}

size_t CToy::GetVertexPosition()
{
	return 0;
}

size_t CToy::GetVertexUV()
{
	return 3*4;
}

size_t CToy::GetPhysicsNumVerts()
{
	if (!m_pPhys)
		return 0;

	return (int)*((uint32_t*)(m_pPhys+TOY_HEADER_SIZE));
}

size_t CToy::GetPhysicsNumTris()
{
	if (!m_pPhys)
		return 0;

	return (size_t)*((uint32_t*)(m_pPhys+TOY_HEADER_SIZE+PHYS_VERTS_LENGTH_SIZE));
}

size_t CToy::GetPhysicsNumBoxes()
{
	if (!m_pPhys)
		return 0;

	return (size_t)*((uint32_t*)(m_pPhys+TOY_HEADER_SIZE+PHYS_VERTS_LENGTH_SIZE+PHYS_TRIS_LENGTH_SIZE));
}

float* CToy::GetPhysicsVerts()
{
	if (!m_pPhys)
		return nullptr;

	return (float*)(m_pPhys+TOY_HEADER_SIZE+PHYS_VERTS_LENGTH_SIZE+PHYS_TRIS_LENGTH_SIZE+PHYS_BOXES_LENGTH_SIZE);
}

float* CToy::GetPhysicsVert(size_t iVert)
{
	if (!m_pPhys)
		return nullptr;

	return GetPhysicsVerts() + iVert*3;
}

int* CToy::GetPhysicsTris()
{
	if (!m_pPhys)
		return nullptr;

	size_t iVertsSize = GetPhysicsNumVerts()*PHYS_VERT_SIZE;
	return (int*)(m_pPhys+TOY_HEADER_SIZE+PHYS_VERTS_LENGTH_SIZE+PHYS_TRIS_LENGTH_SIZE+PHYS_BOXES_LENGTH_SIZE+iVertsSize);
}

int* CToy::GetPhysicsTri(size_t iTri)
{
	if (!m_pPhys)
		return nullptr;

	return GetPhysicsTris() + iTri*3;
}

TRS* CToy::GetPhysicsBoxes()
{
	if (!m_pPhys)
		return nullptr;

	size_t iVertsSize = GetPhysicsNumVerts()*PHYS_VERT_SIZE;
	size_t iTrisSize = GetPhysicsNumTris()*PHYS_TRI_SIZE;
	return (TRS*)(m_pPhys+TOY_HEADER_SIZE+PHYS_VERTS_LENGTH_SIZE+PHYS_TRIS_LENGTH_SIZE+PHYS_BOXES_LENGTH_SIZE+iVertsSize+iTrisSize);
}

TRS& CToy::GetPhysicsBox(size_t iBox)
{
	if (!m_pPhys)
	{
		static TRS d;
		return d;
	}

	return *(GetPhysicsBoxes() + iBox);
}

Vector CToy::GetPhysicsBoxHalfSize(size_t iBox)
{
	TRS& trs = GetPhysicsBox(iBox);

	TAssert(trs.m_angRotation.p == 0);
	TAssert(trs.m_angRotation.y == 0);
	TAssert(trs.m_angRotation.r == 0);

	Matrix4x4 mTRS = trs.GetMatrix4x4();

	AABB aabbBox = s_aabbBoxDimensions;
	aabbBox.m_vecMins = mTRS*aabbBox.m_vecMins;
	aabbBox.m_vecMaxs = mTRS*aabbBox.m_vecMaxs;

	return aabbBox.m_vecMaxs - aabbBox.Center();
}

const AABB& CToy::GetSceneAreaAABB(size_t iSceneArea)
{
	if (!m_pArea)
	{
		static AABB aabb;
		return aabb;
	}

	return *((AABB*)(GetSceneArea(iSceneArea)));
}

size_t CToy::GetSceneAreaNumVisible(size_t iSceneArea)
{
	if (!m_pArea)
		return 0;

	return (size_t)*((uint32_t*)(GetSceneArea(iSceneArea) + AREA_AABB_SIZE));
}

size_t CToy::GetSceneAreasVisible(size_t iSceneArea, size_t iArea)
{
	if (!m_pArea)
		return 0;

	return (size_t)*((uint32_t*)(GetSceneArea(iSceneArea) + AREA_AABB_SIZE + AREA_VISIBLE_AREAS_SIZE + iArea*AREA_VISIBLE_AREAS_STRIDE));
}

char* CToy::GetSceneAreaFileName(size_t iSceneArea)
{
	if (!m_pArea)
		return nullptr;

	size_t iOffset = AREA_AABB_SIZE + AREA_VISIBLE_AREAS_SIZE + GetSceneAreaNumVisible(iSceneArea)*AREA_VISIBLE_AREAS_STRIDE + AREA_VISIBLE_AREA_NAME_SIZE;
	return GetSceneArea(iSceneArea) + iOffset;
}

char* CToy::GetMaterial(size_t i)
{
	TAssert(m_pMesh);
	if (!m_pMesh)
		return nullptr;

	size_t iMaterialTableEntry = TOY_HEADER_SIZE+BASE_AABB_SIZE+BASE_AABB_SIZE+BASE_MATERIAL_TABLE_SIZE+i*BASE_MATERIAL_TABLE_STRIDE;
	size_t iMaterialOffset = (size_t)*((uint32_t*)(m_pBase+iMaterialTableEntry));
	return m_pMesh+iMaterialOffset;
}

char* CToy::GetSceneArea(size_t i)
{
	TAssert(m_pArea);
	if (!m_pArea)
		return nullptr;

	size_t iSceneTable = TOY_HEADER_SIZE+BASE_AABB_SIZE+BASE_AABB_SIZE+BASE_MATERIAL_TABLE_SIZE+GetNumMaterials()*BASE_MATERIAL_TABLE_STRIDE+BASE_SCENE_TABLE_SIZE;
	size_t iSceneTableEntry = iSceneTable+i*AREA_VISIBLE_AREAS_STRIDE;
	size_t iSceneOffset = (size_t)*((uint32_t*)(m_pBase+iSceneTableEntry));
	return m_pArea+iSceneOffset;
}

char* CToy::AllocateBase(size_t iSize)
{
	if (m_pBase)
		free(m_pBase);

	if (m_pMesh)
	{
		free(m_pMesh);
		m_pMesh = nullptr;
	}

	m_pBase = (char*)malloc(iSize);
	return m_pBase;
}

char* CToy::AllocateMesh(size_t iSize)
{
	if (m_pMesh)
		free(m_pMesh);

	m_pMesh = (char*)malloc(iSize);
	return m_pMesh;
}

void CToy::DeallocateMesh()
{
	if (m_pMesh)
		free(m_pMesh);

	m_pMesh = nullptr;
}

char* CToy::AllocatePhys(size_t iSize)
{
	if (m_pPhys)
		free(m_pPhys);

	m_pPhys = (char*)malloc(iSize);
	return m_pPhys;
}

char* CToy::AllocateArea(size_t iSize)
{
	if (m_pArea)
		free(m_pArea);

	m_pArea = (char*)malloc(iSize);
	return m_pArea;
}
