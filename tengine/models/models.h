#ifndef DT_MODELS_H
#define DT_MODELS_H

#include <tvector.h>
#include <color.h>
#include <geometry.h>
#include <tstring.h>

#include <textures/materialhandle.h>

class CToy;

class CModel
{
public:
							CModel(const tstring& sFilename);
							~CModel();

public:
	bool					Load();
	bool					LoadSourceFile();
	size_t					LoadBufferIntoGL(size_t iMaterial);

	void					Unload();
	void					UnloadBufferFromGL(size_t iBuffer);

	void					Reload();

public:
	size_t					m_iReferences;

	tstring					m_sFilename;
	CToy*					m_pToy;

	tvector<CMaterialHandle>	m_ahMaterials;
	tvector<size_t>			m_aiVertexBuffers;
	tvector<size_t>			m_aiVertexBufferSizes;	// How many vertices in this vertex buffer?

	AABB					m_aabbVisBoundingBox;
	AABB					m_aabbPhysBoundingBox;
};

class CModelLibrary
{
public:
							CModelLibrary();
							~CModelLibrary();

public:
	static size_t			GetNumModelsLoaded() { return Get()->m_iModelsLoaded; }

	static size_t			AddModel(const tstring& sModel);
	static size_t			FindModel(const tstring& sModel);
	static CModel*			GetModel(size_t i);
	static void				ReleaseModel(const tstring& sModel);
	static void				ReleaseModel(size_t i);
	static void				UnloadModel(size_t i);

	static void				ResetReferenceCounts();
	static void				ClearUnreferenced();

	static void				LoadAllIntoPhysics();

	static CModelLibrary*	Get() { return s_pModelLibrary; };

protected:
	tvector<CModel*>		m_apModels;
	size_t					m_iModelsLoaded;

private:
	static CModelLibrary*	s_pModelLibrary;
};

#endif