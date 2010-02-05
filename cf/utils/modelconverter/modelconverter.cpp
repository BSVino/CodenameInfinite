#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "modelconverter.h"
#include "strutils.h"

CModelConverter::CModelConverter(CConversionScene* pScene)
{
	m_pScene = pScene;
	m_pWorkListener = NULL;
}

bool CModelConverter::ReadModel(const wchar_t* pszFilename)
{
	std::wstring sExtension;

	size_t iFileLength = wcslen(pszFilename);
	sExtension = pszFilename+iFileLength-4;

	if (wcscmp(sExtension.c_str(), L".obj") == 0)
		ReadOBJ(pszFilename);
	else if (wcscmp(sExtension.c_str(), L".sia") == 0)
		ReadSIA(pszFilename);
	else if (wcscmp(sExtension.c_str(), L".dae") == 0)
		ReadDAE(pszFilename);
	else
		return false;

	return true;
}

// Takes a path + filename + extension and removes path and extension to return only the filename.
// It is destructive on the string that is passed into it.
wchar_t* CModelConverter::MakeFilename(wchar_t* pszPathFilename)
{
	wchar_t* pszFile = pszPathFilename;
	int iLastChar = -1;
	int i = -1;

	while (pszFile[++i])
		if (pszFile[i] == L'\\' || pszFile[i] == L'/')
			iLastChar = i;

	pszFile += iLastChar+1;

	i = -1;
	while (pszFile[++i])
		if (pszFile[i] == L'.')
			iLastChar = i;

	if (iLastChar >= 0)
		pszFile[iLastChar] = L'\0';

	return pszFile;
}

// Destructive
wchar_t* CModelConverter::GetDirectory(wchar_t* pszFilename)
{
	int iLastSlash = -1;
	int i = -1;

	while (pszFilename[++i])
		if (pszFilename[i] == L'\\' || pszFilename[i] == L'/')
			iLastSlash = i;

	if (iLastSlash >= 0)
		pszFilename[iLastSlash] = L'\0';

	return pszFilename;
}

bool CModelConverter::IsWhitespace(wchar_t cChar)
{
	return (cChar == L' ' || cChar == L'\t' || cChar == L'\r' || cChar == L'\n');
}

wchar_t* CModelConverter::StripWhitespace(wchar_t* pszLine)
{
	if (!pszLine)
		return NULL;

	wchar_t* pszSpace = pszLine;
	while (IsWhitespace(pszSpace[0]) && pszSpace[0] != L'\0')
		pszSpace++;

	int iEnd = ((int)wcslen(pszSpace))-1;
	while (iEnd >= 0 && IsWhitespace(pszSpace[iEnd]))
		iEnd--;

	if (iEnd >= -1)
	{
		pszSpace[iEnd+1] = L'\0';
	}

	return pszSpace;
}

std::wstring CModelConverter::StripWhitespace(std::wstring sLine)
{
	int i = 0;
	while (IsWhitespace(sLine[i]) && sLine[i] != L'\0')
		i++;

	int iEnd = ((int)sLine.length())-1;
	while (iEnd >= 0 && IsWhitespace(sLine[iEnd]))
		iEnd--;

	if (iEnd >= -1)
		sLine[iEnd+1] = L'\0';

	return sLine.substr(i);
}
