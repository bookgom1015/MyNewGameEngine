#ifndef __SHADERUITL_HLSLI__
#define __SHADERUITL_HLSLI__

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

namespace ShaderUtil {
    float4 TexCoordToScreen(in float2 texc) {
        return float4(2.f * texc.x - 1.f, 1.f - 2.f * texc.y, 0.f, 1.f);;
    }
    
    float NdcDepthToViewDepth(in float z_ndc, in float4x4 proj) {
	    // z_ndc = A + B/viewZ, where proj[2,2]=A and proj[3,2]=B.
        const float viewZ = proj[3][2] / (z_ndc - proj[2][2]);
        
        return viewZ;
    }
}

#ifndef FitToScreenVertexOut
#define FitToScreenVertexOut    \
struct VertexOut {              \
    float4 PosH : SV_Position;  \
    float2 TexC : TEXCOORD0;    \
};
#endif // FitToScreenVertexOut

#ifndef FitToScreenVertexShader
#define FitToScreenVertexShader                          \
VertexOut VS(                                            \
        in uint vid : SV_VertexID,                       \
        in uint instanceID : SV_InstanceID) {            \
    VertexOut vout = (VertexOut)0;                       \
    vout.TexC = gVertices[ vid];                         \
    vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
    return vout;                                         \
}
#endif // FitToScreenVertexShader

#endif // __SHADERUITL_HLSLI__