// [ Dependencies ]
//  - ShaderUtil.hlsli
//
// [ References ]
//  - https://xtozero.tistory.com/5
//   -- https://github.com/diharaw/volumetric-fog
//   -- https://github.com/bartwronski/CSharpRenderer
//   -- https://github.com/Unity-Technologies/VolumetricLighting/tree/master/Assets/VolumetricFog

#ifndef __VOLUMETRICLIGHT_HLSLI__
#define __VOLUMETRICLIGHT_HLSLI__

namespace VolumetricLight {
    // [ Parameters ]
	//  - wi : the direction which light comes in
	//  - wo : the direction which light goes out
	//  - g  : anisotropic coefficient
	//
	// [ Return ]
	//  - Amount of 1ight leaving
	float HenyeyGreensteinPhaseFunction(float3 wi, float3 wo, float g) {
		const float CosTheta = dot(wi, wo);
		const float g2 = g * g;
		const float denom = pow(1.f + g2 - 2.f * g * CosTheta, 3.f / 2.f);
		return (1.f / (4.f * PI)) * ((1.f - g2) / max(denom, FLT_EPSILON));
	}
	
	float SliceTickness(float z_ndc, float z_exp, float near, float far, uint dimZ) {
		const float CurrDepthV = ShaderUtil::NdcDepthToExpViewDepth(z_ndc, z_exp, near, far);
		const float NextDepthV = ShaderUtil::NdcDepthToExpViewDepth(z_ndc + 1.f / (float)dimZ, z_exp, near, far);
		return NextDepthV - CurrDepthV;
	}
	
	// [ Parameters ]
	//  - accumLight		 : accumlated lights
	//  - accumTransmittance : accumlated transmittance
	//  - sliceLight		 : light of current position
	//  - sliceDensity		 : density of current position
	//  - tickness			 : difference between current slice's depth and next slice's depth
	//  - densityScale		 : scale for density(a number appropriately adjusted to use a larger value
	//							for the uniform density parameter rather than a decimal point 
	//
	// [ Return ]
	//  - Accumlated lights(rgb) and transmittance(a)
	float4 ScatterStep(
			inout float3 accumLight, 
			inout float accumTransmittance, 
			in float3 sliceLight,
			in float sliceDensity, 
			in float tickness,
			in float densityScale) {
		sliceDensity = max(sliceDensity, 0.0000001f);
		sliceDensity *= densityScale;
		const float sliceTransmittance = exp(-sliceDensity * tickness);
	
		// The equation used in Frostbite
		const float3 sliceLightIntegral = sliceLight * (1.f - sliceTransmittance) / sliceDensity;
	
		accumLight += sliceLightIntegral * accumTransmittance;
		accumTransmittance *= sliceTransmittance;
	
		return float4(accumLight, accumTransmittance);
	}
	
	float3 ConvertPositionToUV(in float3 pos, in float4x4 viewProj) {
		float4 posH = mul(float4(pos, 1), viewProj);
		posH /= posH.w;
		return float3(posH.xy * float2(0.5f, -0.5f) + (float2)0.5f, posH.z);
	}
	
	float4 Cubic(float v) {
		const float4 n = float4(1.f, 2.f, 3.f, 4.f) - v;
		const float4 s = n * n * n;
		const float x = s.x;
		const float y = s.y - 4.f * s.x;
		const float z = s.z - 4.f * s.y + 6.f * s.x;
		const float w = 6.f - x - y - z;
		return float4(x, y, z, w) * (1.f / 6.f);
	}
	
	float4 Tex3DTricubic(Texture3D tex, SamplerState samp, float3 texCoords, float3 texSize) {
		const float4 TexelSize = float4(1.f / texSize.xz, texSize.xz);
		
		texCoords = texCoords * texSize - 0.5f;
		
		float3 f = frac(texCoords);
		texCoords -= f;
		
		const float4 xCubic = Cubic(f.x); 
		const float4 yCubic = Cubic(f.y);
		const float4 zCubic = Cubic(f.z);
	   
		const float2 cx = texCoords.xx + float2(-0.5f, 1.5f);
		const float2 cy = texCoords.yy + float2(-0.5f, 1.5f);
		const float2 cz = texCoords.zz + float2(-0.5f, 1.5f);
		const float2 sx = xCubic.xz + xCubic.yw;
		const float2 sy = yCubic.xz + yCubic.yw;
		const float2 sz = zCubic.xz + zCubic.yw;
		
		float2 offsetx = cx + xCubic.yw / sx;
		float2 offsety = cy + yCubic.yw / sy;
		float2 offsetz = cz + zCubic.yw / sz;
		offsetx /= texSize.xx;
		offsety /= texSize.yy;
		offsetz /= texSize.zz;
		
		const float4 sample0 = tex.Sample(samp, float3(offsetx.x, offsety.x, offsetz.x));
		const float4 sample1 = tex.Sample(samp, float3(offsetx.y, offsety.x, offsetz.x));
		const float4 sample2 = tex.Sample(samp, float3(offsetx.x, offsety.y, offsetz.x));	
		const float4 sample3 = tex.Sample(samp, float3(offsetx.y, offsety.y, offsetz.x));
		const float4 sample4 = tex.Sample(samp, float3(offsetx.x, offsety.x, offsetz.y));
		const float4 sample5 = tex.Sample(samp, float3(offsetx.y, offsety.x, offsetz.y));
		const float4 sample6 = tex.Sample(samp, float3(offsetx.x, offsety.y, offsetz.y));
		const float4 sample7 = tex.Sample(samp, float3(offsetx.y, offsety.y, offsetz.y));
		
		const float gx = sx.x / (sx.x + sx.y);
		const float gy = sy.x / (sy.x + sy.y);
		const float gz = sz.x / (sz.x + sz.y);
		
		const float4 x0 = lerp(sample1, sample0, gx);
		const float4 x1 = lerp(sample3, sample2, gx);
		const float4 x2 = lerp(sample5, sample4, gx);
		const float4 x3 = lerp(sample7, sample6, gx);
		const float4 y0 = lerp(x1, x0, gy);
		const float4 y1 = lerp(x3, x2, gy);
		
		return lerp(y1, y0, gz);
	}
}

#endif // __VOLUMETRICLIGHT_HLSLI__