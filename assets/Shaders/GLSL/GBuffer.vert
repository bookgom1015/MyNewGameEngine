#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "GBuffer.glsli"

layout (location = 0) in vec3 iPosL;
layout (location = 1) in vec3 iNormalL;
layout (location = 2) in vec2 iTexC;

layout (location = 0) out vec3 oPosW;
layout (location = 1) out vec3 oNormalW;
layout (location = 2) out vec2 oTexC;

void main() {
	const vec4 PosW = ubObj.World * vec4(iPosL, 1.f);
	oPosW = PosW.xyz;

	const vec4 PosV = ubPass.View * PosW;
	gl_Position = ubPass.Proj * PosV;

	oNormalW = mat3(ubObj.World) * iNormalL;

	oTexC = iTexC;
}