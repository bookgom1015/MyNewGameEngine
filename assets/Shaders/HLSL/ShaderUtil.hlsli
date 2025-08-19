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
        const float3 AbsDir = abs(dir);
        float u = 0.f;
        float v = 0.f;

        if (AbsDir.x >= AbsDir.y && AbsDir.x >= AbsDir.z) {
        // X-major face
            if (dir.x > 0.f) {
            // +X face: (-Z, -Y)
                u = -dir.z / AbsDir.x;
                v = -dir.y / AbsDir.x;
            }
            else {
            // -X face: (Z, -Y)
                u =  dir.z / AbsDir.x;
                v = -dir.y / AbsDir.x;
            }
        }
        else if (AbsDir.y >= AbsDir.x && AbsDir.y >= AbsDir.z) {
        // Y-major face
            if (dir.y > 0.f) {
            // +Y face: (X, Z)
                u = dir.x / AbsDir.y;
                v = dir.z / AbsDir.y;
            }
            else {
            // -Y face: (X, -Z)
                u =  dir.x / AbsDir.y;
                v = -dir.z / AbsDir.y;
            }
        }
        else {
        // Z-major face
            if (dir.z > 0.f) {
            // +Z face: (X, -Y)
                u =  dir.x / AbsDir.z;
                v = -dir.y / AbsDir.z;
            }
            else {
            // -Z face: (-X, -Y)
                u = -dir.x / AbsDir.z;
                v = -dir.y / AbsDir.z;
            }
        }

        return float2(u, v) * 0.5f + 0.5f; // Map from [-1,1] to [0,1]
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
    
    /***************************************************************/
    // 3D value noise
    // Ref: https://www.shadertoy.com/view/XsXfRH
    // The MIT License
    // Copyright © 2017 Inigo Quilez
    // Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    float Hash(in float3 p) {
        p = frac(p * 0.3183099 + .1);
        p *= 17.0;
        return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
    }
    
    
    float3 ThreadIdToNdc(in uint3 DTid, in uint3 dims) {
    	float3 ndc = DTid;
    	ndc += 0.5f;
    	ndc *= float3(2.f / dims.x, -2.f / dims.y, 1.f / dims.z);
    	ndc += float3(-1.f, 1.f, 0.f);
    	return ndc;
    }
    
    float3 NdcToWorldPosition(in float3 ndc, in float depthV, in float4x4 invView, in float4x4 invProj) {
    	float4 rayV = mul(float4(ndc, 1.f), invProj);
    	rayV /= rayV.w;
    	rayV /= rayV.z; // So as to set the z depth value to 1.
    
    	const float4 PosW = mul(float4(rayV.xyz * depthV, 1.f), invView);
    	return PosW.xyz;
    }
    
    float3 ThreadIdToWorldPosition(in uint3 DTid, in uint3 dims, in float z_exp, in float near, in float far, in float4x4 invView, in float4x4 invProj) {
    	const float3 Ndc = ThreadIdToNdc(DTid, dims);
    	const float DepthVS = NdcDepthToExpViewDepth(Ndc.z, z_exp, near, far);
    	return NdcToWorldPosition(Ndc, DepthVS, invView, invProj);
    }
}

#endif // __SHADERUITL_HLSLI__