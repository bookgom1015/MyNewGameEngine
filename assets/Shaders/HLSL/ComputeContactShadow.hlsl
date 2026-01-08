#ifndef __COMPUTECONTACTSHADOW_HLSL__
#define __COMPUTECONTACTSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"
#include "./../../../assets/Shaders/HLSL/Random.hlsli"

ConstantBuffer<ConstantBuffers::PassCB>  cbPass  : register(b0);
ConstantBuffer<ConstantBuffers::LightCB> cbLight : register(b1);

SSCS_ComputeContactShadow_RootConstants(b2)

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap   : register(t1);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap    : register(t2);

RWTexture2D<ShadingConvention::SSCS::ContactShadowMapFormat> go_ContactShadowMap   : register(u0);
RWTexture2D<ShadingConvention::SSCS::DebugMapFormat>         go_DebugMap           : register(u1);

[numthreads(
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Width,
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Height,
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    const float StepLength = gRayMaxDistance / gMaxSteps;
    
    uint result = 0;
    for (uint lightIndex = 0 ; lightIndex < cbLight.LightCount; ++lightIndex) {
        const Render::DX::Foundation::Light light = cbLight.Lights[lightIndex];
        
        const uint Seed = Random::InitRand(DTid.x + DTid.y * gTextureDimX, gFrameCount ^ (lightIndex * 9781u));
        const float Noise = lerp(0.75f, 1.25f, Random::Random01(Seed));
        
        const float4 PosW = gi_PositionMap[DTid];
        
        float3 rayDirection;
        if (light.Type == Common::Render::LightType::E_Directional) {
            rayDirection = -light.Direction;
        }
        else if (light.Type == Common::Render::LightType::E_Point) {
            rayDirection = normalize(light.Position - PosW.xyz);
        }
        
        const float3 RayStep = rayDirection * StepLength;
        
        const float ViewDist = distance(PosW.xyz, cbPass.EyePosW);
        const float gStepScaleFar = 2.f;
        const float gStepScaleFarDist = 40.f;
        const float StepScale = lerp(1.f, gStepScaleFar, saturate(ViewDist / gStepScaleFarDist));
        const float gThicknessBase = 0.01f;
        const float gThicknessFarScale = 2.f;
        const float gThicknessFarDist = 40.f;
        const float Thickness = gThicknessBase + gThicknessFarScale * saturate(ViewDist / gThicknessFarDist);
        
        const float3 N = gi_NormalMap[DTid].xyz;
        const float NoL = dot(N, rayDirection);
        const float BiasW = gBiasBase + gBiasSlope * (1.f - NoL);
        
        float3 rayPos = PosW.xyz + N * BiasW;        
        float2 rayTexC = 0.f;        
        bool occluded = false;
        for (uint stepIndex = 0; stepIndex < gMaxSteps; ++stepIndex) {
            rayPos += RayStep * StepScale * Noise;
            
            float4 viewProjPos = mul(float4(rayPos, 1.f), cbPass.ViewProj);
            viewProjPos /= viewProjPos.w;
            
            rayTexC = viewProjPos.xy * 0.5f + 0.5f;
            rayTexC.y = 1.f - rayTexC.y;
            
            if (any(rayTexC < 0.f) || any(rayTexC > 1.f)) break;
            const float ZSampleDepth = gi_DepthMap.SampleLevel(gsamLinearClamp, rayTexC, 0);
            const float ZSampleDepthV = ShaderUtil::NdcDepthToViewDepth(ZSampleDepth, cbPass.Proj);
            
            const float RayDepthV = mul(float4(rayPos, 1.f), cbPass.View).z;       
            
            const float DepthDelta = RayDepthV - ZSampleDepthV;
                    
            const float Epsilon = gDepthEpsilonBase + gDepthEpsilonScale * abs(RayDepthV);
            if (DepthDelta > Epsilon && DepthDelta < Thickness) {
                occluded = true;
                break;
            }
        }
        
        result = Shadow::CalcShiftedShadowValueF(occluded ? 0 : 1, result, lightIndex);
    }
    
    go_ContactShadowMap[DTid] = result;
}

#endif // __COMPUTECONTACTSHADOW_HLSL__