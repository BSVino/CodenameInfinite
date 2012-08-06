#pragma once

/*
	The Toy model format:

	First comes a header looking very much like PNG's ( http://en.wikipedia.org/wiki/Portable_Network_Graphics#Technical_details )

	0x89		- 1 byte with the high bit set
	'TOY'		- 3 bytes, marker
	'BASE'		- 4 bytes, or another four letter marker for what kind of data it is (BASE|MESH|PHYS|AREA)
	0x0D 0x0A	- 2 bytes, dos style ending
	0x1A		- 1 byte, EOF character
	0x0A		- 1 byte, unix style ending

	Base data format - Information about the model

	4 bytes, float, Visual Bounds min x (of the model in its default pose)
	4 bytes, float, Visual Bounds min y
	4 bytes, float, Visual Bounds min z
	4 bytes, float, Visual Bounds max x
	4 bytes, float, Visual Bounds max y
	4 bytes, float, Visual Bounds max z
	4 bytes, float, Physics Bounds min x (of the model in its default pose)
	4 bytes, float, Physics Bounds min y
	4 bytes, float, Physics Bounds min z
	4 bytes, float, Physics Bounds max x
	4 bytes, float, Physics Bounds max y
	4 bytes, float, Physics Bounds max z
	1 byte, unsigned, number of materials
	∀ material:
		4 bytes, unsigned, MESH file offset of material
		4 bytes, unsigned, number of vertices (# of triangles * 3)
	4 byte, unsigned, number of scene areas
	∀ scene area:
		4 bytes, unsigned, AREA file offset of scene area

	Mesh data format - Stuff that can be freed once it's loaded into the engine. Vertex data, materials, triangles, uv's, normals, etc

	∀ material:
		2 bytes, unsigned, material name length (including null termination)
		x bytes, null terminated string, material file name
		∀ vertex:
			4 bytes, float, x
			4 bytes, float, y
			4 bytes, float, z
			4 bytes, float, u
			4 bytes, float, v

	Phys data format - Physics stuff that must remain in memory so the physics simulation can use it

	4 bytes, unsigned, number of verts
	4 bytes, unsigned, number of triangles
	4 bytes, unsigned, number of boxes
	∀ vertex:
		4 bytes, float, x position
		4 bytes, float, y position
		4 bytes, float, z position
	∀ triangle:
		4 bytes, unsigned, vertex 1
		4 bytes, unsigned, vertex 2
		4 bytes, unsigned, vertex 3
	∀ box:
		4 bytes, float, x translation
		4 bytes, float, y translation
		4 bytes, float, z translation
		4 bytes, float, p rotation
		4 bytes, float, y rotation
		4 bytes, float, r rotation
		4 bytes, float, x scaling
		4 bytes, float, y scaling
		4 bytes, float, z scaling

	Area data format - Scene areas used for scene management

	∀ scene area:
		4 bytes, float, x min
		4 bytes, float, y min
		4 bytes, float, z min
		4 bytes, float, x max
		4 bytes, float, y max
		4 bytes, float, z max
		4 bytes, unsigned, number of visible areas from this area
		∀ visible area:
			4 bytes, unsigned, area id
		2 byte, unsigned, file name length
		x bytes, null terminated string, file name
*/

#include <geometry.h>
#include <trs.h>

class CToy
{
public:
				CToy();
				~CToy();

public:
	const AABB&	GetVisBounds();
	const AABB&	GetPhysBounds();
	size_t		GetNumMaterials();
	size_t		GetMaterialNameLength(size_t iMaterial);
	char*		GetMaterialName(size_t iMaterial);
	size_t		GetMaterialNumVerts(size_t iMaterial);
	float*		GetMaterialVerts(size_t iMaterial);
	float*		GetMaterialVert(size_t iMaterial, size_t iVert);
	size_t		GetNumSceneAreas();

	size_t		GetVertexSize();
	size_t		GetVertexPosition();
	size_t		GetVertexUV();

	size_t		GetPhysicsNumVerts();
	size_t		GetPhysicsNumTris();
	size_t		GetPhysicsNumBoxes();
	float*		GetPhysicsVerts();
	float*		GetPhysicsVert(size_t iVert);
	int*		GetPhysicsTris();
	int*		GetPhysicsTri(size_t iTri);
	TRS*		GetPhysicsBoxes();
	TRS&		GetPhysicsBox(size_t iBox);
	Vector		GetPhysicsBoxHalfSize(size_t iBox);

	const AABB&	GetSceneAreaAABB(size_t iSceneArea);
	size_t		GetSceneAreaNumVisible(size_t iSceneArea);
	size_t		GetSceneAreasVisible(size_t iSceneArea, size_t iArea);
	char*		GetSceneAreaFileName(size_t iSceneArea);

protected:
	char*		GetMaterial(size_t i);
	char*		GetSceneArea(size_t i);

public:
	char*		AllocateBase(size_t iSize);
	char*		AllocateMesh(size_t iSize);
	void		DeallocateMesh();
	char*		AllocatePhys(size_t iSize);
	char*		AllocateArea(size_t iSize);

protected:
	char*		m_pBase;
	char*		m_pMesh;
	char*		m_pPhys;
	char*		m_pArea;

public:
	static AABB	s_aabbBoxDimensions;
};
