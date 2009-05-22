#include <stdio.h>

#include "modelconverter.h"

#include <direct.h>

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		printf("I need a file to convert!\n");
		return 0;
	}

	size_t iFileLength = strlen(argv[1]);
	const char* pszExtension = argv[1]+iFileLength-4;

	CModelConverter c;

	if (strcmp(pszExtension, ".obj") == 0)
	{
		printf("Reading the OBJ... ");
		c.ReadOBJ(argv[1]);
	}
	else if (strcmp(pszExtension, ".sia") == 0)
	{
		printf("Reading the Silo ASCII file... ");
		c.ReadSIA(argv[1]);
	}

	printf("Done.\n");
	printf("\n");

	printf("-------------\n");
	printf("Materials   : %d\n", c.m_Scene.GetNumMaterials());
	printf("Meshes      : %d\n", c.m_Scene.GetNumMeshes());
	printf("\n");

	for (size_t i = 0; i < c.m_Scene.GetNumMeshes(); i++)
	{
		CConversionMesh* pMesh = c.m_Scene.GetMesh(i);

		printf("-------------\n");
		printf("Mesh: %s\n", pMesh->GetBoneName(0));
		printf("Vertices    : %d\n", pMesh->GetNumVertices());
		printf("Normals     : %d\n", pMesh->GetNumNormals());
		printf("UVs         : %d\n", pMesh->GetNumUVs());
		printf("Bones       : %d\n", pMesh->GetNumBones());
		printf("Faces       : %d\n", pMesh->GetNumFaces());
		printf("\n");
	}

	printf("Writing the SMD... ");
	c.WriteSMDs();
	printf("Done.\n");
	printf("\n");

	printf("Press enter to continue...\n");
	getchar();

	return 0;
}
