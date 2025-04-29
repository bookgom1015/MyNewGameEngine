#ifndef __COOKTORRANCE_HLSLI__
#define __COOKTORRANCE_HLSLI__

float3 CookTorrance(in Material mat, in float3 Li, in float3 L, in float3 N, in float3 V, in float NoL) {
    const float3 H = normalize(V + L);
    const float Roughness = 1.f - mat.Shininess;

    const float NDF = DistributionGGX(N, H, Roughness);
    const float G = GeometrySmith(N, V, L, Roughness);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 Diffuse = mat.Albedo.rgb / PI;

    const float3 Numerator = NDF * G * F;
    const float Denominator = 4.f * max(dot(N, V), 0.f) * NoL + 0.0001f;
    const float3 Specular = Numerator / Denominator;

    const float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= (1.f - mat.Metalness);

    return (kD * Diffuse + Specular) * Li * NoL;
}

float3 CookTorrance_GGXModified(in Material mat, in float3 Li, in float3 L, in float3 N, in float3 V, in float NoL, in float d, in float radius) {
    const float3 H = normalize(V + L);
    const float Roughness = 1.f - mat.Shininess;

    const float NDF = DistributionGGX_Modified(N, H, Roughness, d, radius);
    const float G = GeometrySmith(N, V, L, Roughness);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 Diffuse = mat.Albedo.rgb / PI;

    const float3 Numerator = NDF * G * F;
    const float Denominator = 4.f * max(dot(N, V), 0.f) * NoL + 0.0001f;
    const float3 Specular = Numerator / Denominator;

    const float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= (1.f - mat.Metalness);

    return (kD * Diffuse + Specular) * Li * NoL;
}

#endif // __COOKTORRANCE_HLSLI__