#ifndef __COMPUTECONTACTSHADOW_HLSL__
#define __COMPUTECONTACTSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

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
    const Render::DX::Foundation::Light light = cbLight.Lights[0];
    
    const float StepLength = gRayMaxDistance / gMaxSteps;
    
    const float3 RayDirection = -light.Direction;    
    const float3 RayStep = RayDirection * StepLength;
    
    const float4 PosW = gi_PositionMap[DTid];
    float3 rayPos = PosW.xyz;
    
    bool occluded = false;
    float2 rayTexC = 0.f;    
    for (uint i = 0; i < gMaxSteps; ++i) {
        rayPos += RayStep;
        
        float4 viewProjPos = mul(float4(rayPos, 1.f), cbPass.ViewProj);
        viewProjPos /= viewProjPos.w;
        
        rayTexC = viewProjPos.xy * 0.5f + 0.5f;
        rayTexC.y = 1.f - rayTexC.y;
        
        if (any(rayTexC < 0.f) || any(rayTexC > 1.f)) continue;
        const float ZSampleDepth = gi_DepthMap.SampleLevel(gsamLinearClamp, rayTexC, 0);
        const float ZSampleDepthV = ShaderUtil::NdcDepthToViewDepth(ZSampleDepth, cbPass.Proj);
        
        const float RayDepthV = mul(float4(rayPos, 1.f), cbPass.View).z;       
        
        const float DepthDelta = RayDepthV - ZSampleDepthV;
                
        if (DepthDelta > 0.f && DepthDelta < gThickness) {            
            go_DebugMap[DTid] = DepthDelta;
            occluded = true;
            break;
        }
    }
    
    go_ContactShadowMap[DTid] = occluded;
}

#endif // __COMPUTECONTACTSHADOW_HLSL__