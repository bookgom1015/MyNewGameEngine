#ifndef __DRAWSKYSPHERE_HLSL__
#define __DRAWSKYSPHERE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"

ConstantBuffer<ConstantBuffers::PassCB>   cbPass   : register(b0);
ConstantBuffer<ConstantBuffers::ObjectCB> cbObject : register(b1);

EnvironmentMap_DrawSkySphere_RootConstants(b2);

StructuredBuffer<Common::Foundation::Mesh::Vertex> gi_VertexBuffer : register(t0);
ByteAddressBuffer gi_IndexBuffer : register(t1);

TextureCube<ShadingConvention::EnvironmentMap::EnvironmentCubeMapFormat> gi_EnvCubeMap : register(t0, space1);

VERTEX_IN

struct VertexOut {
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;

    // Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;
    
    float4 posW = mul(float4(vout.PosL, 1.f), cbObject.World);
    // Always center sky about camera.
    posW.xyz += cbPass.EyePosW;
    
    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(posW, cbPass.ViewProj).xyww;
    
    return vout;
}

[outputtopology("triangle")]
[numthreads(ShadingConvention::EnvironmentMap::ThreadGroup::MeshShader::ThreadsPerGroup, 1, 1)]
void MS(
        in uint GTid : SV_GroupThreadID,
        in uint Gid : SV_GroupID,
        out vertices VertexOut verts[MESH_SHADER_MAX_VERTICES],
        out indices uint3 prims[MESH_SHADER_MAX_PRIMITIVES]) {    
    SetMeshOutputCounts(MESH_SHADER_MAX_VERTICES, MESH_SHADER_MAX_PRIMITIVES);
        
    const uint TotalPrimCount = gIndexCount / 3;
    const uint GlobalPrimId = Gid * MESH_SHADER_MAX_PRIMITIVES + GTid;
    const uint VertexBase = GTid * 3;
    
    if (GlobalPrimId < TotalPrimCount) {
        const uint3 Index = uint3(
            ShaderUtil::GetIndex16(gi_IndexBuffer, GlobalPrimId * 3 + 0),
            ShaderUtil::GetIndex16(gi_IndexBuffer, GlobalPrimId * 3 + 1),
            ShaderUtil::GetIndex16(gi_IndexBuffer, GlobalPrimId * 3 + 2));
                
        prims[GTid] = uint3(VertexBase + 0, VertexBase + 1, VertexBase + 2);        
        
        [unroll]
        for (uint i = 0; i < 3; ++i) {
            Common::Foundation::Mesh::Vertex vin = gi_VertexBuffer[Index[i]];
    
            VertexOut vout = (VertexOut) 0;
            vout.PosL = vin.Position;
            
            float4 posW = mul(float4(vout.PosL, 1.f), cbObject.World);
            // Always center sky about camera.
            posW.xyz += cbPass.EyePosW;
            
            vout.PosH = mul(posW, cbPass.ViewProj).xyww;
         
            verts[VertexBase + i] = vout;
        }
    }
}

float4 PS(in VertexOut pin) : SV_Target {
    return gi_EnvCubeMap.SampleLevel(gsamLinearClamp, normalize(pin.PosL), 0);
}

#endif // __DRAWSKYSPHERE_HLSL__