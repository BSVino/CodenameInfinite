#include "shaders.h"

#include <GL/glew.h>
#include <time.h>

#include <common.h>
#include <platform.h>
#include <worklistener.h>

#include <tinker/application.h>
#include <tinker/cvar.h>
#include <game/gameserver.h>

CShaderLibrary* CShaderLibrary::s_pShaderLibrary = NULL;
static CShaderLibrary g_ShaderLibrary = CShaderLibrary();

CShaderLibrary::CShaderLibrary()
{
	s_pShaderLibrary = this;

	m_bCompiled = false;
}

CShaderLibrary::~CShaderLibrary()
{
	for (size_t i = 0; i < m_aShaders.size(); i++)
	{
		CShader* pShader = &m_aShaders[i];
		glDetachShader((GLuint)pShader->m_iProgram, (GLuint)pShader->m_iVShader);
		glDetachShader((GLuint)pShader->m_iProgram, (GLuint)pShader->m_iFShader);
		glDeleteProgram((GLuint)pShader->m_iProgram);
		glDeleteShader((GLuint)pShader->m_iVShader);
		glDeleteShader((GLuint)pShader->m_iFShader);
	}

	s_pShaderLibrary = NULL;
}

void CShaderLibrary::AddShader(const tstring& sName, const tstring& sVertex, const tstring& sFragment)
{
	TAssert(!Get()->m_bCompiled);
	if (Get()->m_bCompiled)
		return;

	Get()->m_aShaders.push_back(CShader(sName, sVertex, sFragment));
	Get()->m_aShaderNames[sName] = Get()->m_aShaders.size()-1;
}

void CShaderLibrary::CompileShaders()
{
	if (Get()->m_bCompiled)
		return;

	Get()->ClearLog();

	if (GameServer()->GetWorkListener())
		GameServer()->GetWorkListener()->SetAction(_T("Compiling shaders"), Get()->m_aShaders.size());

	bool bShadersCompiled = true;
	for (size_t i = 0; i < Get()->m_aShaders.size(); i++)
	{
		bShadersCompiled &= Get()->m_aShaders[i].Compile();

		if (!bShadersCompiled)
			break;

		if (GameServer()->GetWorkListener())
			GameServer()->GetWorkListener()->WorkProgress(i);
	}

	if (bShadersCompiled)
		Get()->m_bCompiled = true;
	else
		DestroyShaders();
}

void CShaderLibrary::DestroyShaders()
{
	for (size_t i = 0; i < Get()->m_aShaders.size(); i++)
		Get()->m_aShaders[i].Destroy();

	Get()->m_bCompiled = false;
}

void CShaderLibrary::ClearLog()
{
	m_bLogNeedsClearing = true;
}

void CShaderLibrary::WriteLog(const char* pszLog, const char* pszShaderText)
{
	if (!pszLog || strlen(pszLog) == 0)
		return;

	tstring sFile = GetAppDataDirectory(Application()->AppDirectory(), _T("shaders.txt"));

	if (m_bLogNeedsClearing)
	{
		// Only clear it if we're actually going to write to it so we don't create the file.
		FILE* fp = tfopen(sFile, _T("w"));
		fclose(fp);
		m_bLogNeedsClearing = false;
	}

	char szText[100];
	strncpy(szText, pszShaderText, 99);
	szText[99] = '\0';

	FILE* fp = tfopen(sFile, _T("a"));
	fprintf(fp, "Shader compile output %d\n", (int)time(NULL));
	fprintf(fp, "%s\n\n", pszLog);
	fprintf(fp, "%s...\n\n", szText);
	fclose(fp);
}

CShader* CShaderLibrary::GetShader(const tstring& sName)
{
	eastl::map<tstring, size_t>::const_iterator i = m_aShaderNames.find(sName);
	if (i == m_aShaderNames.end())
		return NULL;

	return &m_aShaders[i->second];
}

CShader* CShaderLibrary::GetShader(size_t i)
{
	if (i >= m_aShaders.size())
		return NULL;

	return &m_aShaders[i];
}

size_t CShaderLibrary::GetProgram(const tstring& sName)
{
	TAssert(Get());
	if (!Get())
		return 0;

	TAssert(Get()->GetShader(sName));
	if (!Get()->GetShader(sName))
		return 0;

	return Get()->GetShader(sName)->m_iProgram;
}

size_t CShaderLibrary::GetProgram(size_t iProgram)
{
	TAssert(Get());
	if (!Get())
		return 0;

	TAssert(Get()->GetShader(iProgram));
	if (!Get()->GetShader(iProgram))
		return 0;

	return Get()->GetShader(iProgram)->m_iProgram;
}

CShader::CShader(const tstring& sName, const tstring& sVertexFile, const tstring& sFragmentFile)
{
	m_sName = sName;
	m_sVertexFile = sVertexFile;
	m_sFragmentFile = sFragmentFile;
	m_iVShader = 0;
	m_iFShader = 0;
	m_iProgram = 0;
}

bool CShader::Compile()
{
	tstring sFunctions =
	"float RemapVal(float flInput, float flInLo, float flInHi, float flOutLo, float flOutHi)"
	"{"
	"	return (((flInput-flInLo) / (flInHi-flInLo)) * (flOutHi-flOutLo)) + flOutLo;"
	"}"

	"float LengthSqr(vec3 v)"
	"{"
	"	return v.x*v.x + v.y*v.y + v.z*v.z;"
	"}"

	"float LengthSqr(vec2 v)"
	"{"
	"	return v.x*v.x + v.y*v.y;"
	"}"

	"float Length2DSqr(vec3 v)"
	"{"
	"	return v.x*v.x + v.z*v.z;"
	"}"

	"float Lerp(float x, float flLerp)"
	"{"
		"if (flLerp == 0.5)"
			"return x;"
		"return pow(x, log(flLerp) * -1.4427);"
	"}"

	"float DistanceToLineSegmentSqr(vec3 p, vec3 v1, vec3 v2)"
	"{"
		"float flResult;"
		"vec3 v = v2 - v1;"
		"vec3 w = p - v1;"
		"float c1 = dot(w, v);"
		"if (c1 < 0.0)"
		"	flResult = LengthSqr(v1-p);"
		"else"
		"{"
		"	float c2 = dot(v, v);"
		"	if (c2 < c1)"
		"		flResult = LengthSqr(v2-p);"
		"	else"
		"	{"
		"		float b = c1/c2;"
		"		vec3 vb = v1 + v*b;"
		"		flResult = LengthSqr(vb - p);"
		"	}"
		"}"
		"return flResult;"
	"}"

	"float AngleDifference(float a, float b)"
	"{"
		"float flYawDifference = a - b;"
		"if ( a > b )"
		"	while ( flYawDifference >= 180.0 )"
		"		flYawDifference -= 360.0;"
		"else"
		"	while ( flYawDifference <= -180.0 )"
		"		flYawDifference += 360.0;"
		"return flYawDifference;"
	"}";

	tstring sVertexShader = sFunctions;
	if (m_sVertexFile == "pass")
		sVertexShader = CShaderLibrary::GetVSPassShader();
	else
	{
		FILE* f = tfopen("shaders/" + m_sVertexFile + ".vs", "r");

		TAssert(f);
		if (!f)
			return false;

		tstring sLine;
		while (fgetts(sLine, f))
			sVertexShader += sLine;

		fclose(f);
	}

	FILE* f = tfopen("shaders/" + m_sFragmentFile + ".fs", "r");

	TAssert(f);
	if (!f)
		return false;

	tstring sFragmentShader = sFunctions;
	tstring sLine;
	while (fgetts(sLine, f))
		sFragmentShader += sLine;

	fclose(f);

	m_iVShader = glCreateShader(GL_VERTEX_SHADER);
	const char* pszStr = sVertexShader.c_str();
	glShaderSource((GLuint)m_iVShader, 1, &pszStr, NULL);
	glCompileShader((GLuint)m_iVShader);

	int iLogLength = 0;
	char szLog[1024];
	glGetShaderInfoLog((GLuint)m_iVShader, 1024, &iLogLength, szLog);
	CShaderLibrary::Get()->WriteLog(szLog, pszStr);

	int iVertexCompiled;
	glGetShaderiv((GLuint)m_iVShader, GL_COMPILE_STATUS, &iVertexCompiled);

	m_iFShader = glCreateShader(GL_FRAGMENT_SHADER);
	pszStr = sFragmentShader.c_str();
	glShaderSource((GLuint)m_iFShader, 1, &pszStr, NULL);
	glCompileShader((GLuint)m_iFShader);

	szLog[0] = '\0';
	iLogLength = 0;
	glGetShaderInfoLog((GLuint)m_iFShader, 1024, &iLogLength, szLog);
	CShaderLibrary::Get()->WriteLog(szLog, pszStr);

	int iFragmentCompiled;
	glGetShaderiv((GLuint)m_iFShader, GL_COMPILE_STATUS, &iFragmentCompiled);

	m_iProgram = glCreateProgram();
	glAttachShader((GLuint)m_iProgram, (GLuint)m_iVShader);
	glAttachShader((GLuint)m_iProgram, (GLuint)m_iFShader);
	glLinkProgram((GLuint)m_iProgram);

	szLog[0] = '\0';
	iLogLength = 0;
	glGetProgramInfoLog((GLuint)m_iProgram, 1024, &iLogLength, szLog);
	CShaderLibrary::Get()->WriteLog(szLog, "link");

	int iProgramLinked;
	glGetProgramiv((GLuint)m_iProgram, GL_LINK_STATUS, &iProgramLinked);

	TAssert(iVertexCompiled == GL_TRUE && iFragmentCompiled == GL_TRUE && iProgramLinked == GL_TRUE);
	if (iVertexCompiled == GL_TRUE && iFragmentCompiled == GL_TRUE && iProgramLinked == GL_TRUE)
		return true;
	else
		return false;
}

void CShader::Destroy()
{
	glDetachShader((GLuint)m_iProgram, (GLuint)m_iVShader);
	glDetachShader((GLuint)m_iProgram, (GLuint)m_iFShader);
	glDeleteShader((GLuint)m_iVShader);
	glDeleteShader((GLuint)m_iFShader);
	glDeleteProgram((GLuint)m_iProgram);
}

void ReloadShaders(class CCommand* pCommand, eastl::vector<tstring>& asTokens, const tstring& sCommand)
{
	CShaderLibrary::DestroyShaders();
	CShaderLibrary::CompileShaders();
	if (CShaderLibrary::Get()->IsCompiled())
		TMsg("Shaders reloaded.\n");
	else
		TMsg("Shaders compile failed. See shaders.txt\n");
}

CCommand shaders_reload("shaders_reload", ::ReloadShaders);
