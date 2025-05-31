//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************
#ifndef __LIGHTINGUTIL_HLSLI__
#define __LIGHTINGUTIL_HLSLI__

#include "./../../../../inc/Render/DX/Foundation/Light.h"
#include "./../../../../assets/Shaders/HLSL/BRDF.hlsli"

static const float DEG2RAD = 3.14159265359f / 180.f;

float DegToRad(in float degrees) {
    return degrees * DEG2RAD;
}

float CalcLinearAttenuation(in float d, in float attenRadius) {
    return saturate((attenRadius - d) / (attenRadius));
}

float CalcInverseSquareAttenuation(in float d, in float attenRadius) {
    const float X = pow(d / attenRadius, 4);
    const float numer = pow(saturate(1 - X), 2);
    const float denom = d * d + 1;

    return numer / denom;
}

float3 CalcPlaneIntersection(in float3 pos, in float3 r, in float3 dir, in float3 center) {
    return pos + r * (dot(dir, center - pos) / dot(dir, r));
}

float3 ComputeDirectionalLight(
        in Render::DX::Foundation::Light light, 
        in Material mat, 
        in float3 normal, 
        in float3 toEye) {
    const float3 L = -light.Direction;
    const float3 Li = light.Color * light.Intensity;

    const float NoL = max(dot(normal, L), 0);

#if defined(BLINN_PHONG)
	return BlinnPhong(mat, Li, L, normal, toEye, NoL);    
#elif defined(COOK_TORRANCE)
	return CookTorrance(mat, Li, L, normal, toEye, NoL);
#else
    return 0;
#endif
}

float3 ComputePointLight(
        in Render::DX::Foundation::Light light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 Ldisp = light.Position - pos;
    const float d = length(Ldisp);

    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

    const float3 r = reflect(-toEye, normal);
    const float3 CenterToRay = dot(Ldisp, r) * r - Ldisp;
    const float3 ClosestPoint = Ldisp + CenterToRay * saturate(light.Radius / length(CenterToRay));
    const float3 L = normalize(ClosestPoint);

    const float NoL = max(dot(normal, L), 0);
    
#if defined(BLINN_PHONG)
	return BlinnPhong(mat, Li, L, normal, toEye, NoL);
#elif defined(COOK_TORRANCE)
	return CookTorrance_GGXModified(mat, Li, L, normal, toEye, NoL, d, light.Radius);
#else
    return 0;
#endif
}

float3 ComputeSpotLight(
        in Render::DX::Foundation::Light light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 Ldisp = light.Position - pos;
    const float3 L = normalize(Ldisp);

    const float Theta = dot(-L, light.Direction);

    const float RadOuter = DegToRad(light.OuterConeAngle);
    const float CosOuter = cos(RadOuter);

    if (Theta < CosOuter) return 0;

    const float RadInner = DegToRad(light.InnerConeAngle);
    const float CosInner = cos(RadInner);

    const float Epsilon = CosInner - CosOuter;
    const float Factor = clamp((Theta - CosOuter) / Epsilon, 0, 1);

    const float d = length(Ldisp);
    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Factor * Falloff;

    const float NoL = max(dot(normal, L), 0);

#if defined(BLINN_PHONG)
	return BlinnPhong(mat, Li, L, normal, toEye, NoL);
#elif defined(COOK_TORRANCE)
	return CookTorrance(mat, Li, L, normal, toEye, NoL);
#else
    return 0;
#endif
}

float3 ComputeTubeLight(
        in Render::DX::Foundation::Light light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 L0 = light.Position - pos;
    const float3 L1 = light.Position1 - pos;

    const float d0 = length(L0);
    const float d1 = length(L1);

    const float NoL0 = dot(normal, L0) / (2 * d0);
    const float NoL1 = dot(normal, L1) / (2 * d1);
    const float NoL = (2 * saturate(NoL0 + NoL1)) / (d0 * d1 + dot(L0, L1) + 2);

    const float3 r = reflect(-toEye, normal);

    const float3 Ldisp = L1 - L0;
    const float RoLdisp = dot(r, Ldisp);
    const float Ld = length(Ldisp);
    const float t = (dot(r, L0) * RoLdisp - dot(L0, Ldisp)) / (Ld * Ld - RoLdisp * RoLdisp);

    float3 closestPoint = L0 + Ldisp * saturate(t);
    const float3 CenterToRay = dot(closestPoint, r) * r - closestPoint;
    closestPoint = closestPoint + CenterToRay * saturate(light.Radius / length(CenterToRay));

    const float3 L = normalize(closestPoint);
    const float d = length(closestPoint);
		
    const float Falloff = CalcLinearAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

#if defined(BLINN_PHONG)
	return 0;
#elif defined(COOK_TORRANCE)
	return CookTorrance_GGXModified(mat, Li, L, normal, toEye, NoL, d, light.Radius);
#else
    return 0;
#endif
}

float3 ComputeRectLight(
        in Render::DX::Foundation::Light light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    if (light.Size.x < FLT_EPSILON || light.Size.y < FLT_EPSILON) return 0;
    
    const float3 L0 = light.Position - pos;
    const float3 L1 = light.Position1 - pos;
    const float3 L2 = light.Position2 - pos;
    const float3 L3 = light.Position3 - pos;
	
    const float3 LightNormal = cross(light.Position1 - light.Position, light.Position2 - light.Position);
    if (dot(normalize(pos - light.Center), LightNormal) < 0) return 0;

    const float3 n0 = normalize(cross(L0, L1));
    const float3 n1 = normalize(cross(L1, L2));
    const float3 n2 = normalize(cross(L2, L3));
    const float3 n3 = normalize(cross(L3, L0));

    const float G0 = acos(dot(-n0, n1));
    const float G1 = acos(dot(-n1, n2));
    const float G2 = acos(dot(-n2, n3));            
    const float G3 = acos(dot(-n3, n0));

    const float SolidAngle = G0 + G1 + G2 + G3 - 2.f * 3.14159265359f;

    const float NoL = SolidAngle * 0.2f * (
		saturate(dot(normalize(L0), normal)) +
		saturate(dot(normalize(L1), normal)) +
		saturate(dot(normalize(L2), normal)) +
		saturate(dot(normalize(L3), normal)) +
		saturate(dot(normalize(light.Center - pos), normal)));

    const float3 r = reflect(-toEye, normal);
    const float3 IntersectPoint = CalcPlaneIntersection(pos, r, light.Direction, light.Center);

    const float3 IntersectionVector = IntersectPoint - light.Center;
    const float2 IntersectPlanePoint = float2(dot(IntersectionVector, light.Right), dot(IntersectionVector, light.Up));
    const float2 Nearest2DPoint = float2(clamp(IntersectPlanePoint.x, -light.Size.x, light.Size.x), clamp(IntersectPlanePoint.y, -light.Size.y, light.Size.y));
    
    const float3 NearestPoint = light.Center + (light.Right * Nearest2DPoint.x + light.Up * Nearest2DPoint.y);
    const float d = distance(pos, NearestPoint);

    const float Area = light.Size.x * light.Size.y;
    const float Falloff = CalcLinearAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff / Area;

    const float3 L = normalize(NearestPoint - pos);

#if defined(BLINN_PHONG)
	return 0;
#elif defined(COOK_TORRANCE)
	return CookTorrance(mat, Li, L, normal, toEye, NoL);
#else
    return 0;
#endif
}

float3 ComputeBRDF(
        in Render::DX::Foundation::Light Lights[MaxLights], 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye, 
        in float shadowFactor[MaxLights], 
        in uint lightCount) {
    float3 result = 0;

	[loop]
    for (uint i = 0; i < lightCount; ++i) {
        Render::DX::Foundation::Light light = Lights[i];
        
        const float factor = shadowFactor[i];
        
        if (light.Type == Common::Render::LightType::E_Directional)
            result += factor * ComputeDirectionalLight(light, mat, normal, toEye);
        else if (light.Type == Common::Render::LightType::E_Point)
            result += factor * ComputePointLight(light, mat, pos, normal, toEye);
        else if (light.Type == Common::Render::LightType::E_Spot)
            result += factor * ComputeSpotLight(light, mat, pos, normal, toEye);
        else if (light.Type == Common::Render::LightType::E_Rect)
            result += ComputeRectLight(light, mat, pos, normal, toEye);
        else if (light.Type == Common::Render::LightType::E_Tube)
            result += factor * ComputeTubeLight(light, mat, pos, normal, toEye);
    }

    return result;
}

#endif // __LIGHTINGUTIL_HLSLI__