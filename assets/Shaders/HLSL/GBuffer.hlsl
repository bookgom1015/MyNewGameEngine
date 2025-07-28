#ifndef __GBUFFER_INL__
#define __GBUFFER_INL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

ConstantBuffer<ConstantBuffers::PassCB>     cbPass      : register(b0);
ConstantBuffer<ConstantBuffers::ObjectCB>   cbObject    : register(b1);
ConstantBuffer<ConstantBuffers::MaterialCB> cbMaterial  : register(b2);

GBuffer_Default_RootConstants(b3)

StructuredBuffer<Common::Foundation::Mesh::Vertex> gi_VertexBuffer : register(t0);
ByteAddressBuffer gi_IndexBuffer : register(t1);

Texture2D<float4> gi_Textures[ShadingConvention::GBuffer::MaxNumTextures] : register(t0, space1);

VERTEX_IN

struct VertexOut {
    float4 PosH        : SV_Position;
    float4 CurrPosH    : CURR_POSITION_H;
    float4 PrevPosH    : PREV_POSITION_H;
    float3 PosW        : POSITION_W;
    float3 PosL        : POSITION_L;
    float3 NormalW     : NORMAL_W;
    float3 PrevNormalW : PrevNORMAL_W;
    float2 TexC        : TEXCOORD;
};

struct PixelOut {
    ShadingConvention::GBuffer::AlbedoMapFormat             Color              : SV_TARGET0;
    ShadingConvention::GBuffer::NormalMapFormat             Normal             : SV_TARGET1;
    ShadingConvention::GBuffer::NormalDepthMapFormat        NormalDepth        : SV_TARGET2;
    ShadingConvention::GBuffer::NormalDepthMapFormat        PrevNormalDepth    : SV_TARGET3;
    ShadingConvention::GBuffer::SpecularMapFormat           Specular           : SV_TARGET4;
    ShadingConvention::GBuffer::RoughnessMetalnessMapFormat RoughnessMetalness : SV_TARGET5;
    ShadingConvention::GBuffer::VelocityMapFormat           Velocity           : SV_TARGET6;
    ShadingConvention::GBuffer::PositionMapFormat           Position           : SV_TARGET7;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut) 0;
    
    vout.PosL = vin.PosL;
    
    const float4 PosW = mul(float4(vin.PosL, 1.f), cbObject.World);
    vout.PosW = PosW.xyz;
    
    const float4 PosH = mul(PosW, cbPass.ViewProj);
    vout.CurrPosH = PosH;
    vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
    const float4 PrevPosW = mul(float4(vin.PosL, 1), cbObject.PrevWorld);
    vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
    
    vout.NormalW = mul(vin.NormalL, (float3x3)cbObject.World);
    vout.PrevNormalW = mul(vin.NormalL, (float3x3)cbObject.PrevWorld);
    
    float4 TexC = mul(float4(vin.TexC, 0.f, 1.f), cbObject.TexTransform);
    vout.TexC = mul(TexC, cbMaterial.MatTransform).xy;
    
    return vout;
}

[outputtopology("triangle")]
[numthreads(ShadingConvention::GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup, 1, 1)]
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
            ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 0),
            ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 1),
            ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 2));
                
        prims[GTid] = uint3(VertexBase + 0, VertexBase + 1, VertexBase + 2);
        
        [unroll]
        for (uint i = 0; i < 3; ++i) {
            Common::Foundation::Mesh::Vertex vin = gi_VertexBuffer[Index[i]];
    
            VertexOut vout = (VertexOut) 0;
            vout.PosL = vin.Position;
            
            float4 PosW = mul(float4(vout.PosL, 1.f), cbObject.World);
            vout.PosW = PosW.xyz;
    
            const float4 PosH = mul(PosW, cbPass.ViewProj);
            vout.CurrPosH = PosH;
            vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
            const float4 PrevPosW = mul(float4(vin.Position, 1), cbObject.PrevWorld);
            vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
            
            vout.NormalW = mul(vin.Normal, (float3x3)cbObject.World);
            vout.PrevNormalW = mul(vin.Normal, (float3x3)cbObject.PrevWorld);
    
            float4 TexC = mul(float4(vin.TexCoord, 0.f, 1.f), cbObject.TexTransform);
            vout.TexC = TexC.xy;
         
            verts[VertexBase + i] = vout;
        }
    }
}

PixelOut PS(in VertexOut pin) {
    PixelOut pout = (PixelOut) 0;
    
    pin.CurrPosH /= pin.CurrPosH.w;
    pin.PrevPosH /= pin.PrevPosH.w;
    const float2 Velocity = ShaderUtil::CalcVelocity(pin.CurrPosH, pin.PrevPosH);
    
    pout.Color = cbMaterial.Albedo;
    pout.Normal = float4(pin.NormalW, 1.f);
    pout.NormalDepth = ValuePackaging::EncodeNormalDepth(pin.NormalW, pin.CurrPosH.z);
    pout.PrevNormalDepth = ValuePackaging::EncodeNormalDepth(pin.PrevNormalW, pin.PrevPosH.z);
    pout.Specular = float4(cbMaterial.Specular, 1.f);
    pout.RoughnessMetalness = float2(cbMaterial.Roughness, cbMaterial.Metalness);
    pout.Velocity = Velocity;
    pout.Position = float4(pin.PosW, 1.f);
    
    return pout;
}

#endif // __GBUFFER_INL__