#ifndef __SSAO_HLSL__
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

ConstantBuffer<ConstantBuffers::AmbientOcclusionCB> cbAO : register(b0);

SSAO_Default_RootConstants(b1);

Texture2D<ShadingConvention::GBuffer::NormalMapFormat>	            gi_NormalMap	   : register(t0);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap     : register(t1);
Texture2D<ShadingConvention::SSAO::RandomVectorMapFormat>           gi_RandomVectorMap : register(t2);

RWTexture2D<ShadingConvention::SSAO::AOMapFormat>                   go_AOMap           : register(u0);

[numthreads(
    ShadingConvention::SSAO::ThreadGroup::Default::Width,
    ShadingConvention::SSAO::ThreadGroup::Default::Height,
    ShadingConvention::SSAO::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    const float2 TexC = (DTid + 0.5f) * gInvTexDim;
    
    const float4 PosW = gi_PositionMap.SampleLevel(gsamPointClamp, TexC, 0);
    if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) {
        go_AOMap[DTid] = 1.f;
        return;
    }
    
    const float3 PosV = mul(PosW, cbAO.View).xyz;
	
    const float3 NormalW = normalize(gi_NormalMap.SampleLevel(gsamPointClamp, TexC, 0).xyz);
    const float3 NormalV = mul(NormalW, (float3x3)cbAO.View);

	// Extract random vector and map from [0,1] --> [-1, +1].
    const float3 RandVec = 2.f * gi_RandomVectorMap.SampleLevel(gsamLinearWrap, 4.f * TexC, 0) - 1.f;

    float occlusionSum = 0.f;

	// Sample neighboring points about p in the hemisphere oriented by n.
	[loop]
    for (uint i = 0; i < cbAO.SampleCount; ++i) {
		// Are offset vectors are fixed and uniformly distributed (so that our offset vectors
		// do not clump in the same direction).  If we reflect them about a random vector
		// then we get a random uniform distribution of offset vectors.
        const float3 Offset = reflect(cbAO.OffsetVectors[i].xyz, RandVec);

		// Flip offset vector if it is behind the plane defined by (p, n).
        const float Flip = sign(dot(Offset, NormalV));

		// Sample a point near PosV within the occlusion radius.
        const float3 SamplePos = PosV + Flip * cbAO.OcclusionRadius * Offset;

		// Project SamplePos and generate projective tex-coords.  
        float4 projPos = mul(float4(SamplePos, 1), cbAO.ProjTex);
        projPos /= projPos.w;
                
        const float4 PosW_ = gi_PositionMap.SampleLevel(gsamPointClamp, projPos.xy, 0);
        if (!ShadingConvention::GBuffer::IsValidPosition(PosW_)) continue;
        
        const float3 PosV_ = mul(PosW_, cbAO.View).xyz;
		
        //
		// Test whether r occludes p.
		//   * The product dot(n, normalize(r - p)) measures how much in front
		//     of the plane(p,n) the occluder point r is.  The more in front it is, the
		//     more occlusion weight we give it.  This also prevents self shadowing where 
		//     a point r on an angled plane (p,n) could give a false occlusion since they
		//     have different depth values with respect to the eye.
		//   * The weight of the occlusion is scaled based on how far the occluder is from
		//     the point we are computing the occlusion of.  If the occluder r is far away
		//     from p, then it does not occlude it.
		// 
        const float DistZ = PosV.z - PosV_.z;
        const float DotP = max(dot(NormalV, normalize(PosV_ - PosV)), 0.f);

        const float Occlusion = DotP * SSAO::OcclusionFunction(DistZ, cbAO.SurfaceEpsilon, cbAO.OcclusionFadeStart, cbAO.OcclusionFadeEnd);

        occlusionSum += Occlusion;
    }

    occlusionSum /= cbAO.SampleCount;

    const float Access = 1.f - occlusionSum;

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
    go_AOMap[DTid] = saturate(pow(Access, cbAO.OcclusionStrength));
}

#endif // __SSAO_HLSL__