#ifndef __APPLYCONTACTSHADOW_HLSL__
#define __APPLYCONTACTSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"

RWTexture2D<ShadingConvention::SSCS::ContactShadowMapFormat> gi_ContactShadowMap   : register(u0);

RWTexture2D<ShadingConvention::Shadow::ShadowMapFormat>      gio_ShadowMap         : register(u1);

[numthreads(
    ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Width, 
    ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Height, 
    ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const uint ContactShadowValue = gi_ContactShadowMap[DTid];
    uint value = gio_ShadowMap[DTid];
    
    gio_ShadowMap[DTid] = ContactShadowValue & value;
}

#endif // __APPLYCONTACTSHADOW_HLSL__