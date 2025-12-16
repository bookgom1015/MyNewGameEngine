#ifndef __BOKEH_HLSL__
#define __BOKEH_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/Bokeh.hlsli"

DOF_Bokeh_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_BackBuffer	: register(t0);
Texture2D<ShadingConvention::DOF::CircleOfConfusionMapFormat> gi_CoCMap : register(t1);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader   

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
    const float dx = gBokehRadius * gInvTexDim.x;
    const float dy = gBokehRadius * gInvTexDim.y;

	const float3 CenterColor = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;

    const float CenterCoC = abs(gi_CoCMap.SampleLevel(gsamLinearClamp, pin.TexC, 0));
	const uint BlurRadius = round(gSampleCount * CenterCoC);
	if (BlurRadius < 1) return float4(CenterColor, 1.f);

	float3 poweredSum = pow(CenterColor, gHighlightPower.xxx);
	float3 colorSum = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb * poweredSum;
	[loop]
	for (int i = -gSampleCount; i <= gSampleCount; ++i) {
		[loop]
		for (int j = -gSampleCount; j <= gSampleCount; ++j) {
			const float Radius = sqrt(i * i + j * j);
			if ((i == 0 && j == 0) || Radius > BlurRadius) continue;

			const float2 texc = pin.TexC + float2(i * dx, j * dy);
			const float NeighborCoC = abs(gi_CoCMap.SampleLevel(gsamLinearClamp, texc, 0));
			if (NeighborCoC < gThreshold) continue;

			const float3 color = gi_BackBuffer.SampleLevel(gsamLinearClamp, texc, 0).rgb;
			const float3 powered = pow(color, gHighlightPower.xxx);

			colorSum += color * powered;
			poweredSum += powered;
		}
	}
	colorSum /= poweredSum;

	return float4(colorSum, 1.f);
}

#endif // __BOKEH_HLSL__