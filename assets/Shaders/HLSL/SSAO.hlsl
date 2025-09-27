#ifndef __SSAO_HLSL__
#define __SSAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/SSAO.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

ConstantBuffer<ConstantBuffers::AmbientOcclusionCB> cbAO : register(b0);

SSAO_DrawAO_RootConstants(b1);

Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat> gi_NormalDepthMap  : register(t0);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>    gi_PositionMap     : register(t1);
Texture2D<ShadingConvention::SSAO::RandomVectorMapFormat>   gi_RandomVectorMap : register(t2);

RWTexture2D<ShadingConvention::SSAO::AOMapFormat>           go_AOMap           : register(u0);

[numthreads(
    ShadingConvention::SSAO::ThreadGroup::Default::Width,
    ShadingConvention::SSAO::ThreadGroup::Default::Height,
    ShadingConvention::SSAO::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    const float2 TexC = (DTid + 0.5f) * gInvTexDim;
    
    const float4 PosW = gi_PositionMap.SampleLevel(gsamPointClamp, TexC, 0);
    if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) {
        go_AOMap[DTid] = ShadingConvention::SSAO::InvalidAOValue;
        return;
    }
    
    const uint2 FullResDTid = DTid * 2;
    const uint NormalDepth = gi_NormalDepthMap[FullResDTid];
    
    float3 normalW;
    float dump;
    ValuePackaging::DecodeNormalDepth(NormalDepth, normalW, dump);
    
    const uint TexCSeed_X = Random::InitRand(DTid.x + DTid.y * cbAO.TextureDim.x, 1);
    const uint TexCSeed_Y = Random::InitRand(DTid.y + DTid.y * cbAO.TextureDim.x, 1);
    
    float2 randTexC;
    randTexC.x = Random::Random01inclusive(TexCSeed_X);
    randTexC.y = Random::Random01inclusive(TexCSeed_Y);
    
	// Extract random vector and map from [0,1] --> [-1, +1].
    const float3 RandVec = 2.f * gi_RandomVectorMap.SampleLevel(gsamLinearWrap, 4.f * randTexC, 0) - 1.f;

    float occlusionSum = 0.f;

	// Sample neighboring points about p in the hemisphere oriented by n.
	[loop]
    for (uint i = 0; i < cbAO.SampleCount; ++i) {
        const uint LoopSeed = Random::InitRand(DTid.x + DTid.y * cbAO.TextureDim.x * i, cbAO.FrameCount + i);        
        const float3 Direction = Random::CosHemisphereSample(LoopSeed, normalW);
    
		// Are offset vectors are fixed and uniformly distributed (so that our offset vectors
		// do not clump in the same direction).  If we reflect them about a random vector
		// then we get a random uniform distribution of offset vectors.
        const float3 Offset = reflect(Direction, RandVec);

		// Flip offset vector if it is behind the plane defined by (p, n).
        const float Flip = sign(dot(Offset, normalW));
        
        const float Radius = Random::Random01inclusive(LoopSeed) * cbAO.OcclusionRadius;
        
		// Sample a point near Pos within the occlusion radius.
        const float3 SamplePos = PosW.xyz + Flip * Radius * Offset;
        
        const float4 SamplePosV = mul(float4(SamplePos, 1.f), cbAO.View);

		// Project SamplePos and generate projective tex-coords.  
        float4 projPos = mul(SamplePosV, cbAO.ProjTex);
        projPos /= projPos.w;
                
        const float4 PosW_ = gi_PositionMap.SampleLevel(gsamPointClamp, projPos.xy, 0);
        if (!ShadingConvention::GBuffer::IsValidPosition(PosW_)) continue;
        		
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
        const float Dist = distance(PosW.xyz, PosW_.xyz);
        const float DotP = max(dot(normalW, normalize(PosW_.xyz - PosW.xyz)), 0.f);

        const float Occlusion = DotP * SSAO::OcclusionFunction(Dist, cbAO.SurfaceEpsilon, cbAO.OcclusionFadeStart, cbAO.OcclusionFadeEnd);

        occlusionSum += Occlusion;
    }

    occlusionSum /= cbAO.SampleCount;

    const float Access = 1.f - occlusionSum;

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
    go_AOMap[DTid] = saturate(pow(Access, cbAO.OcclusionStrength));
}

#endif // __SSAO_HLSL__