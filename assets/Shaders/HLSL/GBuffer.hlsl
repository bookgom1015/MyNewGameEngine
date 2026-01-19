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
    float4 CurrPosH    : POSITION0;
    float4 PrevPosH    : POSITION1;
    float3 PosW        : POSITION2;
    float3 PosL        : POSITION3;
    float3 NormalW     : NORMAL0;
    float3 PrevNormalW : NORMAL1;
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
                                                                                                                                                                                                                                                                                    
static const float gThresholdMatrix8x8[8][8] = {
    {  0.f / 64.f, 48.f / 64.f, 12.f / 64.f, 60.f / 64.f,  3.f / 64.f, 51.f / 64.f, 15.f / 64.f, 63.f / 64.f },
    { 32.f / 64.f, 16.f / 64.f, 44.f / 64.f, 28.f / 64.f, 35.f / 64.f, 19.f / 64.f, 47.f / 64.f, 31.f / 64.f },
    {  8.f / 64.f, 56.f / 64.f,  4.f / 64.f, 52.f / 64.f, 11.f / 64.f, 59.f / 64.f,  7.f / 64.f, 55.f / 64.f },
    { 40.f / 64.f, 24.f / 64.f, 36.f / 64.f, 20.f / 64.f, 43.f / 64.f, 27.f / 64.f, 39.f / 64.f, 23.f / 64.f },
    {  2.f / 64.f, 50.f / 64.f, 14.f / 64.f, 62.f / 64.f,  1.f / 64.f, 49.f / 64.f, 13.f / 64.f, 61.f / 64.f },
    { 34.f / 64.f, 18.f / 64.f, 46.f / 64.f, 30.f / 64.f, 33.f / 64.f, 17.f / 64.f, 45.f / 64.f, 29.f / 64.f },
    { 10.f / 64.f, 58.f / 64.f,  6.f / 64.f, 54.f / 64.f,  9.f / 64.f, 57.f / 64.f,  5.f / 64.f, 53.f / 64.f },
    { 42.f / 64.f, 26.f / 64.f, 38.f / 64.f, 22.f / 64.f, 41.f / 64.f, 25.f / 64.f, 37.f / 64.f, 21.f / 64.f }
};     

static const float gThresholdMatrix8x8_Ornament[8][8] = {
    {  0.f / 64.f,  8.f / 64.f, 16.f / 64.f, 24.f / 64.f, 25.f / 64.f, 17.f / 64.f,  9.f / 64.f,  1.f / 64.f },
    {  7.f / 64.f, 15.f / 64.f, 23.f / 64.f, 31.f / 64.f, 32.f / 64.f, 26.f / 64.f, 18.f / 64.f, 10.f / 64.f },
    { 14.f / 64.f, 22.f / 64.f, 30.f / 64.f, 38.f / 64.f, 39.f / 64.f, 33.f / 64.f, 27.f / 64.f, 19.f / 64.f },
    { 21.f / 64.f, 29.f / 64.f, 37.f / 64.f, 45.f / 64.f, 46.f / 64.f, 40.f / 64.f, 34.f / 64.f, 28.f / 64.f },
    { 20.f / 64.f, 36.f / 64.f, 44.f / 64.f, 52.f / 64.f, 53.f / 64.f, 47.f / 64.f, 41.f / 64.f, 35.f / 64.f },
    { 13.f / 64.f, 43.f / 64.f, 51.f / 64.f, 59.f / 64.f, 60.f / 64.f, 54.f / 64.f, 48.f / 64.f, 42.f / 64.f },
    {  6.f / 64.f, 12.f / 64.f, 58.f / 64.f, 50.f / 64.f, 61.f / 64.f, 55.f / 64.f, 49.f / 64.f, 57.f / 64.f },
    {  5.f / 64.f,  4.f / 64.f, 11.f / 64.f,  3.f / 64.f, 62.f / 64.f, 56.f / 64.f, 63.f / 64.f,  2.f / 64.f }
};

static const float gThresholdMatrix8x8_Honey[8][8] = {
    { 12.f / 64.f, 20.f / 64.f, 28.f / 64.f, 36.f / 64.f, 37.f / 64.f, 29.f / 64.f, 21.f / 64.f, 13.f / 64.f },
    { 19.f / 64.f,  4.f / 64.f, 44.f / 64.f, 52.f / 64.f, 53.f / 64.f, 45.f / 64.f,  5.f / 64.f, 27.f / 64.f },
    { 11.f / 64.f, 43.f / 64.f,  2.f / 64.f, 60.f / 64.f, 61.f / 64.f,  3.f / 64.f, 51.f / 64.f, 35.f / 64.f },
    { 18.f / 64.f, 50.f / 64.f, 59.f / 64.f,  0.f / 64.f,  1.f / 64.f, 58.f / 64.f, 42.f / 64.f, 26.f / 64.f },
    { 25.f / 64.f, 41.f / 64.f, 57.f / 64.f,  8.f / 64.f,  9.f / 64.f, 56.f / 64.f, 34.f / 64.f, 17.f / 64.f },
    { 33.f / 64.f, 49.f / 64.f,  7.f / 64.f, 55.f / 64.f, 54.f / 64.f,  6.f / 64.f, 48.f / 64.f, 24.f / 64.f },
    { 40.f / 64.f, 15.f / 64.f, 47.f / 64.f, 31.f / 64.f, 30.f / 64.f, 46.f / 64.f, 23.f / 64.f, 39.f / 64.f },
    { 14.f / 64.f, 22.f / 64.f, 38.f / 64.f, 16.f / 64.f, 17.f / 64.f, 37.f / 64.f, 29.f / 64.f, 21.f / 64.f }
};

static const float gThresholdMatrix8x8_Wave[8][8] = {
    { 32.f / 64.f, 40.f / 64.f, 48.f / 64.f, 56.f / 64.f, 48.f / 64.f, 40.f / 64.f, 32.f / 64.f, 24.f / 64.f },
    { 24.f / 64.f, 32.f / 64.f, 40.f / 64.f, 48.f / 64.f, 40.f / 64.f, 32.f / 64.f, 24.f / 64.f, 16.f / 64.f },
    { 16.f / 64.f, 24.f / 64.f, 32.f / 64.f, 40.f / 64.f, 32.f / 64.f, 24.f / 64.f, 16.f / 64.f,  8.f / 64.f },
    {  8.f / 64.f, 16.f / 64.f, 24.f / 64.f, 32.f / 64.f, 24.f / 64.f, 16.f / 64.f,  8.f / 64.f,  0.f / 64.f },
    {  0.f / 64.f,  8.f / 64.f, 16.f / 64.f, 24.f / 64.f, 16.f / 64.f,  8.f / 64.f,  0.f / 64.f,  8.f / 64.f },
    {  8.f / 64.f, 16.f / 64.f, 24.f / 64.f, 32.f / 64.f, 24.f / 64.f, 16.f / 64.f,  8.f / 64.f, 16.f / 64.f },
    { 16.f / 64.f, 24.f / 64.f, 32.f / 64.f, 40.f / 64.f, 32.f / 64.f, 24.f / 64.f, 16.f / 64.f, 24.f / 64.f },
    { 24.f / 64.f, 32.f / 64.f, 40.f / 64.f, 48.f / 64.f, 40.f / 64.f, 32.f / 64.f, 24.f / 64.f, 32.f / 64.f }
};

static const float gThresholdMatrix8x8_Swirl[8][8] = {
    { 20.f / 64.f, 28.f / 64.f, 36.f / 64.f, 44.f / 64.f, 52.f / 64.f, 60.f / 64.f, 61.f / 64.f, 53.f / 64.f },
    { 12.f / 64.f,  4.f / 64.f,  5.f / 64.f, 13.f / 64.f, 21.f / 64.f, 29.f / 64.f, 37.f / 64.f, 45.f / 64.f },
    { 19.f / 64.f, 11.f / 64.f,  3.f / 64.f,  2.f / 64.f, 10.f / 64.f, 18.f / 64.f, 26.f / 64.f, 34.f / 64.f },
    { 27.f / 64.f, 35.f / 64.f, 17.f / 64.f,  9.f / 64.f,  1.f / 64.f,  0.f / 64.f,  8.f / 64.f, 16.f / 64.f },
    { 43.f / 64.f, 51.f / 64.f, 25.f / 64.f, 33.f / 64.f, 41.f / 64.f, 49.f / 64.f, 57.f / 64.f, 56.f / 64.f },
    { 59.f / 64.f, 58.f / 64.f, 50.f / 64.f, 42.f / 64.f, 34.f / 64.f, 26.f / 64.f, 18.f / 64.f, 10.f / 64.f },
    { 62.f / 64.f, 54.f / 64.f, 46.f / 64.f, 38.f / 64.f, 30.f / 64.f, 22.f / 64.f, 14.f / 64.f,  6.f / 64.f },
    { 63.f / 64.f, 55.f / 64.f, 47.f / 64.f, 39.f / 64.f, 31.f / 64.f, 23.f / 64.f, 15.f / 64.f,  7.f / 64.f }
};

static const float gThresholdMatrix8x8_Organic[8][8] = {
    {  7.f / 64.f, 35.f / 64.f, 12.f / 64.f, 44.f / 64.f, 18.f / 64.f, 53.f / 64.f, 27.f / 64.f, 60.f / 64.f },
    { 42.f / 64.f, 16.f / 64.f, 50.f / 64.f,  3.f / 64.f, 38.f / 64.f,  9.f / 64.f, 57.f / 64.f, 24.f / 64.f },
    { 21.f / 64.f, 46.f / 64.f,  5.f / 64.f, 32.f / 64.f, 14.f / 64.f, 41.f / 64.f, 11.f / 64.f, 55.f / 64.f },
    { 49.f / 64.f, 28.f / 64.f, 37.f / 64.f,  1.f / 64.f, 58.f / 64.f, 20.f / 64.f, 34.f / 64.f,  8.f / 64.f },
    {  4.f / 64.f, 52.f / 64.f, 23.f / 64.f, 47.f / 64.f, 10.f / 64.f, 61.f / 64.f, 15.f / 64.f, 40.f / 64.f },
    { 31.f / 64.f,  6.f / 64.f, 54.f / 64.f, 19.f / 64.f, 45.f / 64.f, 13.f / 64.f, 36.f / 64.f, 26.f / 64.f },
    { 56.f / 64.f, 17.f / 64.f, 39.f / 64.f, 29.f / 64.f,  2.f / 64.f, 48.f / 64.f, 22.f / 64.f, 51.f / 64.f },
    { 25.f / 64.f, 59.f / 64.f, 33.f / 64.f, 43.f / 64.f, 30.f / 64.f,  0.f / 64.f, 62.f / 64.f, 63.f / 64.f }
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
    const uint TotalPrimCount = gIndexCount / 3;
    const uint GlobalPrimId = Gid * MESH_SHADER_MAX_PRIMITIVES + GTid;
    
    const uint Remaining = TotalPrimCount - Gid * MESH_SHADER_MAX_PRIMITIVES;
    const uint LocalPrimCount = min(Remaining, MESH_SHADER_MAX_PRIMITIVES);
        
    SetMeshOutputCounts(LocalPrimCount * 3, LocalPrimCount);
    
    if (GTid >= LocalPrimCount) return;
    
    const uint LocalPrimId = GTid;
    const uint PrimIndex = Gid * MESH_SHADER_MAX_PRIMITIVES + LocalPrimId;
    
    const uint3 Indices = uint3(
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 0),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 1),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 2));
        
    prims[LocalPrimId] = uint3(
        LocalPrimId * 3 + 0,
        LocalPrimId * 3 + 1,
        LocalPrimId * 3 + 2
    );
    
    [unroll]
    for (uint i = 0; i < 3; ++i) {
        const uint OutVert = LocalPrimId * 3 + i;
    
        Common::Foundation::Mesh::Vertex vin = gi_VertexBuffer[Indices[i]];
    
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
     
        verts[OutVert] = vout;
    }
}

PixelOut PS(in VertexOut pin) {
    pin.CurrPosH /= pin.CurrPosH.w;
        
    const float2 UV = pin.CurrPosH.xy * 0.5f + 0.5f;
    const uint2 ScreenPos = (uint2)floor(UV * gTexDim);
    const uint2 ScreenPos_xN = ScreenPos >> 1;
    const float Threshold = gThresholdMatrix8x8[ScreenPos_xN.y & 7][ScreenPos_xN.x & 7];
    
    float4 posV = mul(pin.CurrPosH, cbPass.InvProj);
	posV /= posV.w;

	const float Dist = (posV.z - gDitheringMinDist) / (gDitheringMaxDist - gDitheringMinDist);

	clip(Dist - Threshold);
    
    PixelOut pout = (PixelOut) 0;
    
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