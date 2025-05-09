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
    
    uint GetCubeFaceIndex(in float3 direction) {
        const float3 AbsDir = abs(direction);
        if (AbsDir.x >= AbsDir.y && AbsDir.x >= AbsDir.z)
            return (direction.x > 0.f) ? 0 : 1; // +X : -X
        else if (AbsDir.y >= AbsDir.x && AbsDir.y >= AbsDir.z)
            return (direction.y > 0.f) ? 2 : 3; // +Y : -Y
        else
            return (direction.z > 0.f) ? 4 : 5; // +Z : -Z    
    }
    
    // Convert normalized direction to UV coordinates for the 2D texture
    float2 ConvertDirectionToUV(in float3 dir) {
        const float AbsX = abs(dir.x);
        const float AbsY = abs(dir.y);
        const float AbsZ = abs(dir.z);

        const float DirX = dir.x > 0.f ? dir.x : min(dir.x, -1e-6f);
        const float DirY = dir.y > 0.f ? dir.y : min(dir.y, -1e-6f);
        const float DirZ = dir.z > 0.f ? dir.z : min(dir.z, -1e-6f);

        float u, v;

	// Check which face the vector corresponds to
        if (AbsX >= AbsY && AbsX >= AbsZ) {
		// +X or -X face
            if (dir.x > 0.f) {
                u = 0.5f * (-dir.z / DirX + 1.f);
                v = 0.5f * (-dir.y / DirX + 1.f);
            }
            else {
                u = 0.5f * (dir.z / -DirX + 1.f);
                v = 0.5f * (dir.y / DirX + 1.f);
            }
        }
        else if (AbsY >= AbsX && AbsY >= AbsZ) {
		// +Y or -Y face
            if (dir.y > 0.f) {
                u = 0.5f * (dir.x / DirY + 1.f);
                v = 0.5f * (dir.z / DirY + 1.f);
            }
            else {
                u = 0.5f * (dir.x / -DirY + 1.f);
                v = 0.5f * (dir.z / DirY + 1.f);
            }
        }
        else {
		// +Z or -Z face
            if (dir.z > 0.f) {
                u = 0.5f * (dir.x / DirZ + 1.f);
                v = 0.5f * (-dir.y / DirZ + 1.f);
            }
            else {
                u = 0.5f * (dir.x / DirZ + 1.f);
                v = 0.5f * (dir.y / DirZ + 1.f);
            }
        }

        return float2(u, v);
    }
    
    float2 CalcVelocity(in float4 curr_pos, in float4 prev_pos) {
        curr_pos.xy = (curr_pos.xy + (float2) 1.f) * 0.5f;
        curr_pos.y = 1.f - curr_pos.y;

        prev_pos.xy = (prev_pos.xy + (float2) 1.f) * 0.5f;
        prev_pos.y = 1.f - prev_pos.y;

        return (curr_pos - prev_pos).xy;
    }
    
    float NdcDepthToViewDepth(in float z_ndc, in float4x4 proj) {
	    // z_ndc = A + B/viewZ, where proj[2,2]=A and proj[3,2]=B.
        const float viewZ = proj[3][2] / (z_ndc - proj[2][2]);
        
        return viewZ;
    }

    float NdcDepthToExpViewDepth(in float z_ndc, in float z_exp, in float near, in float far) {
        const float viewZ = pow(z_ndc, z_exp) * (far - near) + near;
        
        return viewZ;
    }
}

#endif // __SHADERUITL_HLSLI__