#ifndef __DRAWZDEPTH_HLSL__
#define __DRAWZDEPTH_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/Shadow.hlsli"

ConstantBuffer<ConstantBuffers::PassCB>     cbPass      : register(b0);
ConstantBuffer<ConstantBuffers::ObjectCB>   cbObject    : register(b1);
ConstantBuffer<ConstantBuffers::MaterialCB> cbMaterial  : register(b2);

Shadow_DrawZDepth_RootConstants(b3)

Texture2D<float4> gi_Textures[ShadingConvention::GBuffer::MaxNumTextures] : register(t0, space1);

VERTEX_IN

struct VertexOut {
    float4 PosW : SV_POSITION;
    float2 TexC : TEXCOORD;
};

struct GeoOut {
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut) 0;

    vout.PosW = mul(float4(vin.PosL, 1.f), cbObject.World);

    const float4 TexC = mul(float4(vin.TexC, 0.f, 1.f), cbObject.TexTransform);
    vout.TexC = mul(TexC, cbMaterial.MatTransform).xy;

    return vout;
}

[maxvertexcount(18)]
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    GeoOut gout = (GeoOut) 0;
	
    Render::DX::Foundation::Light light = cbPass.Lights[gLightIndex];

	// Directional light
    if (light.Type == Common::Render::LightType::E_Directional) {
		[unroll]
        for (uint i = 0; i < 3; ++i) {
            gout.PosH = mul(gin[i].PosW, light.Mat0);

            const float4 TexC = mul(float4(gin[i].TexC, 0.f, 1.f), cbObject.TexTransform);
            gout.TexC = mul(TexC, cbMaterial.MatTransform).xy;

            triStream.Append(gout);
        }
    }
	// Point light
    else if (light.Type == Common::Render::LightType::E_Point) {
		[loop]
        for (uint face = 0; face < 6; ++face) {
            gout.ArrayIndex = face;

            const float4x4 ViewProj = Shadow::GetViewProjMatrix(light, face);

			[unroll]
            for (uint i = 0; i < 3; ++i) {
                gout.PosH = mul(gin[i].PosW, ViewProj);
                gout.TexC = gin[i].TexC;

                triStream.Append(gout);
            }

            triStream.RestartStrip();
        }
    }
}

void PS(in GeoOut pin) {
    float4 albedo = cbMaterial.Albedo;
    if (cbMaterial.AlbedoMapIndex != -1) albedo *= gi_Textures[cbMaterial.AlbedoMapIndex].SampleLevel(gsamLinearClamp, pin.TexC, 0);

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(albedo.a - 0.1f);
#endif    
}

#endif // __DRAWZDEPTH_HLSL__