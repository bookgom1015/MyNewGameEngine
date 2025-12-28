#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "GBuffer.glsli"

layout (location = 0) in vec3 iPosW;
layout (location = 1) in vec3 iNormalW;
layout (location = 2) in vec2 iTexC;

layout (location = 0) out vec4 oColor;
layout (location = 1) out vec4 oNormal;
layout (location = 2) out vec4 oPosition;

void main() {
	oColor = ubMat.Albedo;

	const vec3 NormalW = normalize(iNormalW);
	oNormal = vec4(NormalW, 1.f);

	oPosition = vec4(iPosW, 1.f);
}