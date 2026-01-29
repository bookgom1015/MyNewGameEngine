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
        in Common::Foundation::Light lights[MaxLights], 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye, 
        in float shadowFactors[MaxLights],
        in uint lightCount) {    
    float3 result = 0;

	[loop]
    for (uint i = 0; i < lightCount; ++i) {
        Common::Foundation::Light light = lights[i];
        
        const float factor = shadowFactors[i];
        
        if (light.Type == Common::Foundation::LightType_Directional)
            result += factor * ComputeDirectionalLight(light, mat, normal, toEye);
        //else if (light.Type == Common::Foundation::LightType::E_Point)
        //    result += factor * ComputePointLight(light, mat, pos, normal, toEye);
        //else if (light.Type == Common::Foundation::LightType::E_Spot)
        //    result += factor * ComputeSpotLight(light, mat, pos, normal, toEye);
        //else if (light.Type == Common::Foundation::LightType::E_Rect)
        //    result += ComputeRectLight(light, mat, pos, normal, toEye);
        //else if (light.Type == Common::Foundation::LightType::E_Tube)
        //    result += factor * ComputeTubeLight(light, mat, pos, normal, toEye);
    }

    return result;
}

#endif // __BRDF_HLSLI__