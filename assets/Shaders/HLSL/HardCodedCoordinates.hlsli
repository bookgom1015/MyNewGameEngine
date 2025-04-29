#ifndef __HARDCODEDCOORDINATES_HLSLI__
#define __HARDCODEDCOORDINATES_HLSLI__

#ifdef _CUBE_COORD
static const float3 gVertices[36] = {
	// Front Face
	float3(-0.5, -0.5, -0.5),	// Bottom Left
	float3(0.5,  0.5, -0.5),	// Top Right
	float3(0.5, -0.5, -0.5),	// Bottom Right
	float3(-0.5, -0.5, -0.5),	// Bottom Left
	float3(-0.5,  0.5, -0.5),	// Top Left
	float3(0.5,  0.5, -0.5),	// Top Right
	// Back Face
	float3(-0.5, -0.5,  0.5),	// Bottom Left
	float3(0.5, -0.5,  0.5),	// Bottom Right
	float3(0.5,  0.5,  0.5),	// Top Right
	float3(-0.5, -0.5,  0.5),	// Bottom Left
	float3(0.5,  0.5,  0.5),	// Top Right
	float3(-0.5,  0.5,  0.5),	// Top Left
	// Bottom Face
	float3(-0.5, -0.5, -0.5),	// Bottom Left (Front)
	float3(0.5, -0.5, -0.5),	// Bottom Right (Front)
	float3(0.5, -0.5,  0.5),	// Bottom Right (Back)
	float3(-0.5, -0.5, -0.5),	// Bottom Left (Front)
	float3(0.5, -0.5,  0.5),	// Bottom Right (Back)
	float3(-0.5, -0.5,  0.5),	// Bottom Left (Back)
	// Top Face
	float3(-0.5,  0.5, -0.5),	// Top Left (Front)
	float3(0.5,  0.5,  0.5),	// Top Right (Back)
	float3(0.5,  0.5, -0.5),	// Top Right (Front)
	float3(-0.5,  0.5, -0.5),	// Top Left (Front)
	float3(-0.5,  0.5,  0.5),	// Top Left (Back)
	float3(0.5,  0.5,  0.5),	// Top Right (Back)
	// Left Face
	float3(-0.5, -0.5, -0.5),	// Bottom Left
	float3(-0.5, -0.5,  0.5),	// Bottom Right
	float3(-0.5,  0.5,  0.5),	// Top Right
	float3(-0.5, -0.5, -0.5),	// Bottom Left
	float3(-0.5,  0.5,  0.5),	// Top Right
	float3(-0.5,  0.5, -0.5),	// Top Left
	// Right Face
	float3(0.5, -0.5, -0.5),	// Bottom Left
	float3(0.5,  0.5,  0.5),	// Top Right
	float3(0.5, -0.5,  0.5),	// Bottom Right
	float3(0.5, -0.5, -0.5),	// Bottom Left
	float3(0.5,  0.5, -0.5),	// Top Left
	float3(0.5,  0.5,  0.5),	// Top Right
};
#endif // _CUBE_COORD

#ifdef _FIT_TO_SCREEN_COORD
static const float2 gVertices[6] = {
	float2(0.f, 1.f),
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 0.f),
	float2(1.f, 1.f)
};
#endif // _FIT_TO_SCREEN_COORD

#endif // __HARDCODEDCOORDINATES_HLSLI__