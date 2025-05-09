#ifndef __XB_Cube_SC
#define __SSAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/SSAO.hlsli"

ConstantBuffer<ConstantBuffers::SsaoCB> cbSsao : register(b0);

SSAO_Default_RootConstants(b1);

Texture2D<ShadingConvention::GBuffer::NormalMapFormat>	  gi_NormalMap	     : register(t0);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>  gi_PositionMap     : register(t1);
Texture2D<ShadingConvention::SSAO::RandomVectorMapFormat> gi_RandomVectorMap : register(t2);

RWTexture2D<ShadingConvention::SSAO::AOMapFormat>         go_AOMap           : register(u0);

[numthreads(
    ShadingConvention::SSAO::ThreadGroup::Default::Width,
    ShadingConvention::SSAO::ThreadGroup::Default::Height,
    ShadingConvention::SSAO::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    const float2 TexC = (DTid + 0.5f) * gInvTexDim;
    
    const float4 PosW = gi_PositionMap.SampleLevel(gsamLinearClamp, TexC, 0);
    if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) {
        go_AOMap[DTid] = 1.f;
        return;
    }
    
    const float3 PosV = mul(float4(PosW.xyz, 1.f), cbSsao.View).xyz;
	
    const float3 NormalW = normalize(gi_NormalMap.SampleLevel(gsamLinearClamp, TexC, 0).xyz);
    const float3 NormalV = mul(NormalW, (float3x3)cbSsao.View);

	// Extract random vector and map from [0,1] --> [-1, +1].
    const float3 RandVec = 2.f * gi_RandomVectorMap.SampleLevel(gsamLinearWrap, 4.f * TexC, 0) - 1.f;

    float occlusionSum = 0.f;

	// Sample neighboring points about p in the hemisphere oriented by n.
	[loop]
    for (uint i = 0; i < cbSsao.SampleCount; ++i) {
		// Are offset vectors are fixed and uniformly distributed (so that our offset vectors
		// do not clump in the same direction).  If we reflect them about a random vector
		// then we get a random uniform distribution of offset vectors.
        const float3 Offset = reflect(cbSsao.OffsetVectors[i].xyz, RandVec);

		// Flip offset vector if it is behind the plane defined by (p, n).
        const float Flip = sign(dot(Offset, NormalV));

		// Sample a point near p within the occlusion radius.
        const float3 q = PosV + Flip * cbSsao.OcclusionRadius * Offset;

		// Project q and generate projective tex-coords.  
        float4 projQ = mul(float4(q, 1), cbSsao.ProjTex);
        projQ /= projQ.w;
                
        const float4 PosW_ = gi_PositionMap.SampleLevel(gsamLinearClamp, projQ.xy, 0);
        if (!ShadingConvention::GBuffer::IsValidPosition(PosW_)) continue;
        
        const float3 PosV_ = mul(float4(PosW_.xyz, 1.f), cbSsao.View).xyz;
		
        const float DistZ = PosV.z - PosV_.z;
        const float DotP = max(dot(NormalV, normalize(PosV_ - PosV)), 0.f);

        const float Occlusion = DotP * SSAO::OcclusionFunction(DistZ, cbSsao.SurfaceEpsilon, cbSsao.OcclusionFadeStart, cbSsao.OcclusionFadeEnd);

        occlusionSum += Occlusion;
    }

    occlusionSum /= cbSsao.SampleCount;

    const float access = 1.f - occlusionSum;

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
    go_AOMap[DTid] = saturate(pow(access, 6.f));
}

#endif // __SSAO_HLSL__