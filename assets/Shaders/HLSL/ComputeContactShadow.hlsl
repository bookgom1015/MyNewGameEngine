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
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap    : register(t1);

RWTexture2D<ShadingConvention::SSCS::ContactShadowMapFormat> go_ContactShadowMap   : register(u0);
RWTexture2D<ShadingConvention::SSCS::DebugMapFormat>         go_DebugMap           : register(u1);

[numthreads(
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Width,
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Height,
    ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    const float StepLength = gRayMaxDistance / gMaxSteps;
    
    uint result = 0;
    for (uint i = 0 ; i < cbLight.LightCount; ++i) {
        const Render::DX::Foundation::Light light = cbLight.Lights[i];
        
        const uint Seed = Random::InitRand(DTid.x + DTid.y * gTextureDimX, gFrameCount);
        const float Noise = Random::Random01(Seed) * 0.5f + 0.25f;
        
        const float4 PosW = gi_PositionMap[DTid];
        
        float3 rayDirection;
        if (light.Type == Common::Render::LightType::E_Directional) {
            rayDirection = -light.Direction;
        }
        else if (light.Type == Common::Render::LightType::E_Point) {
            rayDirection = normalize(light.Position - PosW.xyz);
        }
        
        const float3 RayStep = rayDirection * StepLength;
        
        const float k = max(distance(cbPass.EyePosW, PosW.xyz) * 0.5f, 1.f);
        const float Thickness = k * gThickness;
        
        float3 rayPos = PosW.xyz;        
        float2 rayTexC = 0.f;        
        bool occluded = false;
        for (uint i = 0; i < gMaxSteps; ++i) {
            rayPos += RayStep * k * Noise;
            
            float4 viewProjPos = mul(float4(rayPos, 1.f), cbPass.ViewProj);
            viewProjPos /= viewProjPos.w;
            
            rayTexC = viewProjPos.xy * 0.5f + 0.5f;
            rayTexC.y = 1.f - rayTexC.y;
            
            if (any(rayTexC < 0.f) || any(rayTexC > 1.f)) break;
            const float ZSampleDepth = gi_DepthMap.SampleLevel(gsamLinearClamp, rayTexC, 0);
            const float ZSampleDepthV = ShaderUtil::NdcDepthToViewDepth(ZSampleDepth, cbPass.Proj);
            
            const float RayDepthV = mul(float4(rayPos, 1.f), cbPass.View).z;       
            
            const float DepthDelta = RayDepthV - ZSampleDepthV;
                    
            if (DepthDelta > 0.f && DepthDelta < Thickness) {
                occluded = true;
                break;
            }
        }
        
        result = Shadow::CalcShiftedShadowValueF(occluded ? 0 : 1, result, i);
    }
    
    go_ContactShadowMap[DTid] = result;
}

#endif // __COMPUTECONTACTSHADOW_HLSL__