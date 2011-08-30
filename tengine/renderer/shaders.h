#ifndef DT_SHADERS_H
#define DT_SHADERS_H

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/map.h>

#include <tstring.h>

class CShader
{
public:
							CShader(const tstring& sName, const tstring& sVertexFile, const tstring& sFragmentFile);

public:
	bool					Compile();
	void					Destroy();

public:
	eastl::string			m_sName;
	eastl::string			m_sVertexFile;
	eastl::string			m_sFragmentFile;
	size_t					m_iVShader;
	size_t					m_iFShader;
	size_t					m_iProgram;
	eastl::vector<int>		m_aiTexCoordAttributes;
};

class CShaderLibrary
{
	friend class CShader;

public:
							CShaderLibrary();
							~CShaderLibrary();

public:
	static size_t			GetNumShaders() { return Get()->m_aShaders.size(); };

	static CShader*			GetShader(const tstring& sName);
	static CShader*			GetShader(size_t i);

	static void				AddShader(const tstring& sName, const tstring& sVertex, const tstring& sFragment);

	static size_t			GetProgram(const tstring& sName);

	static const char*		GetVSPassShader();

	static void				CompileShaders();
	static void				DestroyShaders();

	static bool				IsCompiled() { return Get()->m_bCompiled; };

	static CShaderLibrary*	Get() { return s_pShaderLibrary; };

protected:
	void					ClearLog();
	void					WriteLog(const char* pszLog, const char* pszShaderText);

protected:
	eastl::map<tstring, size_t>	m_aShaderNames;
	eastl::vector<CShader>	m_aShaders;
	bool					m_bCompiled;

	size_t					m_iTerrain;
	size_t					m_iModel;
	size_t					m_iProp;
	size_t					m_iScrollingTexture;
	size_t					m_iExplosion;
	size_t					m_iBlur;
	size_t					m_iBrightPass;
	size_t					m_iDarken;
	size_t					m_iStencil;
	size_t					m_iCameraGuided;

	bool					m_bLogNeedsClearing;

private:
	static CShaderLibrary*	s_pShaderLibrary;
};

inline const char* CShaderLibrary::GetVSPassShader()
{
	return
		"attribute vec2 vecTexCoord0;"
		"attribute vec2 vecTexCoord1;"

		"varying vec2 vecFragmentTexCoord0;"
		"varying vec2 vecFragmentTexCoord1;"

		"void main()"
		"{"
		"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
		"	vecFragmentTexCoord0 = vecTexCoord0;"
		"	vecFragmentTexCoord1 = vecTexCoord1;"
		"	gl_FrontColor = gl_Color;"
		"}";
}

#define ENDL "\n"

#define REMAPVAL \
	"float RemapVal(float flInput, float flInLo, float flInHi, float flOutLo, float flOutHi)" \
	"{" \
	"	return (((flInput-flInLo) / (flInHi-flInLo)) * (flOutHi-flOutLo)) + flOutLo;" \
	"}" \

#define LENGTHSQR \
	"float LengthSqr(vec3 v)" \
	"{" \
	"	return v.x*v.x + v.y*v.y + v.z*v.z;" \
	"}" \
	"float LengthSqr(vec2 v)" \
	"{" \
	"	return v.x*v.x + v.y*v.y;" \
	"}" \

#define LENGTH2DSQR \
	"float Length2DSqr(vec3 v)" \
	"{" \
	"	return v.x*v.x + v.z*v.z;" \
	"}" \

#define LERP \
	"float Lerp(float x, float flLerp)" \
	"{" \
		"if (flLerp == 0.5)" \
			"return x;" \
		"return pow(x, log(flLerp) * -1.4427);" \
	"}" \

// Depends on LENGTHSQR
#define DISTANCE_TO_SEGMENT_SQR \
	"float DistanceToLineSegmentSqr(vec3 p, vec3 v1, vec3 v2)" \
	"{" \
		"float flResult;" \
		"vec3 v = v2 - v1;" \
		"vec3 w = p - v1;" \
		"float c1 = dot(w, v);" \
		"if (c1 < 0.0)" \
		"	flResult = LengthSqr(v1-p);" \
		"else" \
		"{" \
		"	float c2 = dot(v, v);" \
		"	if (c2 < c1)" \
		"		flResult = LengthSqr(v2-p);" \
		"	else" \
		"	{" \
		"		float b = c1/c2;" \
		"		vec3 vb = v1 + v*b;" \
		"		flResult = LengthSqr(vb - p);" \
		"	}" \
		"}" \
		"return flResult;" \
	"}" \

#define ANGLE_DIFFERENCE \
	"float AngleDifference(float a, float b)" \
	"{" \
		"float flYawDifference = a - b;" \
		"if ( a > b )" \
		"	while ( flYawDifference >= 180.0 )" \
		"		flYawDifference -= 360.0;" \
		"else" \
		"	while ( flYawDifference <= -180.0 )" \
		"		flYawDifference += 360.0;" \
		"return flYawDifference;" \
	"}" \

/*
	struct gl_LightSourceParameters {
		vec4 ambient; 
		vec4 diffuse; 
		vec4 specular; 
		vec4 position; 
		vec4 halfVector; 
		vec3 spotDirection; 
		float spotExponent; 
		float spotCutoff; // (range: [0.0,90.0], 180.0)
		float spotCosCutoff; // (range: [1.0,0.0],-1.0)
		float constantAttenuation; 
		float linearAttenuation; 
		float quadraticAttenuation;	
	};

	uniform gl_LightSourceParameters gl_LightSource[gl_MaxLights];

	struct gl_LightModelParameters {
		vec4 ambient; 
	};

	uniform gl_LightModelParameters gl_LightModel;

	struct gl_MaterialParameters {
		vec4 emission;   
		vec4 ambient;    
		vec4 diffuse;    
		vec4 specular;   
		float shininess; 
	};

	uniform gl_MaterialParameters gl_FrontMaterial;
	uniform gl_MaterialParameters gl_BackMaterial;
*/

#endif
