#ifndef __BRDF_HLSLI__
#define __BRDF_HLSLI__

#include "./../../../assets/Shaders/HLSL5/CookTorrance.hlsli"

float3 ComputeDirectionalLight(
        in Common::Foundation::Light light, 
        in Material mat, 
        in float3 normal, 
        in float3 toEye) {
    const float3 L = -light.Direction;
    const float3 Li = light.Color * light.Intensity;

    const float NoL = max(dot(normal, L), 0);

    return CookTorrance(mat, Li, L, normal, toEye, NoL);
}

float3 ComputeBRDF(
        in Common::Foundation::Light light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye, 
        in float shadowFactor) {
    float3 result = 0;
        
    if (light.Type == Common::Foundation::LightType_Directional)
        result += shadowFactor * ComputeDirectionalLight(light, mat, normal, toEye);
    //else if (light.Type == LightType_Point)
    //    result += shadowFactor * ComputePointLight(light, mat, pos, normal, toEye);
    //else if (light.Type == LightType_Spot)
    //    result += shadowFactor * ComputeSpotLight(light, mat, pos, normal, toEye);    

    return result;
}

#endif // __BRDF_HLSLI__