//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef __AORAYGEN_HLSL__
#define __AORAYGEN_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/Random.hlsli"

ConstantBuffer<ConstantBuffers::RayGenCB> cbRayGen : register(b0);

StructuredBuffer<ShadingConvention::RayGen::AlignedHemisphereSample3D>     gi_SampleSets                 : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>                gi_NormalDepthMap             : register(t1);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>                   gi_PositionMap                : register(t2);

RWTexture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>              go_RayDirectionOriginDepthMap : register(u0);
RWTexture2D<float4>                                                        go_DebugMap                   : register(u1);

static const uint PixelStepX = 2;

float3 GetRandomRayDirection(in uint2 srcRayIndex, in float3 surfaceNormal, in uint2 textureDim, in uint raySampleIndexOffset) {
    // Calculate coordinate system for the hemisphere.
    float3 u, v, w;
    w = surfaceNormal;

    // Get a vector that's not parallel to w.
    const float3 Right = 0.3f * w + float3(-0.72f, 0.56f, -0.34f);
    v = normalize(cross(w, Right));
    u = cross(v, w);

    // Calculate offsets to the pregenerated sample set.
    uint sampleSetJump = 0; // Offset to the start of the sample set
    uint sampleJump = 0; // Offset to the first sample for this pixel within a sample set.
    {
        // Neighboring samples NxN share a sample set, but use different samples within a set.
        // Sharing a sample set lets the pixels in the group get a better coverage of the hemisphere 
        // than if each pixel used a separate sample set with less samples pregenerated per set.

        // Get a common sample set ID and seed shared across neighboring pixels.
        const uint NumSampleSetsInX = (textureDim.x + cbRayGen.NumPixelsPerDimPerSet - 1) / cbRayGen.NumPixelsPerDimPerSet;
        const uint2 SampleSetId = srcRayIndex / cbRayGen.NumPixelsPerDimPerSet;

        // Get a common hitPosition to adjust the sampleSeed by. 
        // This breaks noise correlation on camera movement which otherwise results 
        // in noise pattern swimming across the screen on camera movement.
        const uint2 PixelZeroId = SampleSetId * cbRayGen.NumPixelsPerDimPerSet;
        uint2 PixelZeroIdFullRes = PixelZeroId;
        if (cbRayGen.CheckerboardRayGenEnabled) PixelZeroIdFullRes.x = PixelZeroIdFullRes.x * PixelStepX;
        
        const float3 PixelZeroHitPosition = gi_PositionMap[PixelZeroIdFullRes].xyz;        
        const uint SampleSetSeed = (SampleSetId.y * NumSampleSetsInX + SampleSetId.x) * ShaderUtil::Hash(PixelZeroHitPosition) + cbRayGen.Seed;
        uint randomState = Random::SeedThread(SampleSetSeed);

        sampleSetJump = Random::Random(randomState, 0, cbRayGen.NumSampleSets - 1) * cbRayGen.NumSamplesPerSet;

        // Get a pixel ID within the shared set across neighboring pixels.
        const uint2 PixeIDPerSet2D = srcRayIndex % cbRayGen.NumPixelsPerDimPerSet;
        const uint PixeIDPerSet = PixeIDPerSet2D.y * cbRayGen.NumPixelsPerDimPerSet + PixeIDPerSet2D.x;

        // Randomize starting sample position within a sample set per neighbor group 
        // to break group to group correlation resulting in square alias.
        const uint NumPixelsPerSet = cbRayGen.NumPixelsPerDimPerSet * cbRayGen.NumPixelsPerDimPerSet;
        sampleJump = PixeIDPerSet + Random::Random(randomState, 0, NumPixelsPerSet - 1) + raySampleIndexOffset;
    }

    // Load a pregenerated random sample from the sample set.
    const float3 Sample = gi_SampleSets[sampleSetJump + (sampleJump % cbRayGen.NumSamplesPerSet)].Value;
    const float3 RayDirection = normalize(Sample.x * u + Sample.y * v + Sample.z * w);
    
    return RayDirection;
}

[numthreads(
    ShadingConvention::RayGen::ThreadGroup::Default::Width, 
    ShadingConvention::RayGen::ThreadGroup::Default::Height, 
    ShadingConvention::RayGen::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID, in uint2 GTid : SV_GroupThreadID) {
    uint2 DTidFullRes = DTid;
    
    if (cbRayGen.CheckerboardRayGenEnabled) {
        const bool IsEvenPixelY = (DTid.y & 1) == 0;
        const uint PixelOffsetX = IsEvenPixelY != cbRayGen.CheckerboardGenerateRaysForEvenPixels;
        DTidFullRes.x = DTid.x * PixelStepX + PixelOffsetX;
    }
    
    float3 surfaceNormal;
    float rayOriginDepth;
    ValuePackaging::DecodeNormalDepth(gi_NormalDepthMap[DTidFullRes], surfaceNormal, rayOriginDepth);
    
    float3 rayDirection = 0;
    if (rayOriginDepth != ShadingConvention::GBuffer::InvalidNormalDepthValue) 
        rayDirection = GetRandomRayDirection(DTid, surfaceNormal, cbRayGen.TextureDim, 0);
    
    go_RayDirectionOriginDepthMap[DTid] = ValuePackaging::EncodeNormalDepth(rayDirection, rayOriginDepth);
    go_DebugMap[DTid] = float4(rayDirection, 0.f);
}

#endif // __AORAYGEN_HLSL__