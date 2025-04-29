#ifndef __SHADERUITL_HLSLI__
#define __SHADERUITL_HLSLI__

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

#ifndef FitToCubeVertexShader
#define FitToCubeVertexShader             \
VertexOut VS(in uint vid : SV_VertexID) { \
    VertexOut vout = (VertexOut) 0;       \
    vout.PosL = gVertices[vid];           \
    return vout;                          \
}
#endif // FitToCubeVertexShader

#ifndef FitToCubeGeometryShader
#define FitToCubeGeometryShader                                                 \
[maxvertexcount(18)]                                                            \
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) { \
    GeoOut gout = (GeoOut) 0;                                                   \
	[unroll]                                                                    \
    for (uint face = 0; face < 6; ++face) {                                     \
        const float4x4 view = cbProjectToCube.View[face];                       \
		[unroll]                                                                \
        for (uint i = 0; i < 3; ++i) {                                          \
            const float3 posL = gin[i].PosL;                                    \
            const float4 posV = mul(float4(posL, 1.f), view);                   \
            const float4 posH = mul(posV, cbProjectToCube.Proj);                \
            gout.PosL = posL;                                                   \
            gout.PosH = posH.xyww;                                              \
            gout.ArrayIndex = face;                                             \
            triStream.Append(gout);                                             \
        }                                                                       \
        triStream.RestartStrip();                                               \
    }                                                                           \
}
#endif // FitToCubeGeometryShader

#ifndef FitToScreenMeshShader
#define FitToScreenMeshShader                                \
[outputtopology("triangle")]                                 \
[numthreads(2, 1, 1)]                                        \
void MS(                                                     \
        in uint GTid : SV_GroupThreadID,                     \
        in uint Gid : SV_GroupID,                            \
        out vertices VertexOut verts[6],                     \
        out indices uint3 prims[2]) {                        \
    SetMeshOutputCounts(6, 2);                               \
    const uint VertexBase = GTid * 3;                        \
    const uint3 Index = uint3(                               \
        VertexBase + 0,                                      \
        VertexBase + 1,                                      \
        VertexBase + 2);                                     \
    prims[GTid] = Index;                                     \
    [unroll]                                                 \
    for (uint i = 0;i < 3; ++i) {                            \
        VertexOut vout = (VertexOut)0;                       \
        vout.TexC = gVertices[Index[i]];                     \
        vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
        verts[VertexBase + i] = vout;                        \
    }                                                        \
}
#endif // FitToScreenMeshShader

namespace ShaderUtil {
    float4 TexCoordToScreen(in float2 texc) {
        return float4(2.f * texc.x - 1.f, 1.f - 2.f * texc.y, 0.f, 1.f);;
    }
    
    uint GetIndex16(in ByteAddressBuffer buf, in uint index) {
        const uint BaseIndex = index / 2;
        const bool IsOdd = index % 2 != 0;
        
        uint value = 0;
        if (IsOdd) {
            value = buf.Load(BaseIndex * 4);
            value = value >> 16;
        }
        else {
            value = buf.Load(BaseIndex * 4) & 0x0000FFFF;
        }
        
        return value;
    }
    
    uint GetIndex32(in ByteAddressBuffer buf, in uint idx) {
        return buf.Load(idx * 4);
    }
}

#endif // __SHADERUITL_HLSLI__