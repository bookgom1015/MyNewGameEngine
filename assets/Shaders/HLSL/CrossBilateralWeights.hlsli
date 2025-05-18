#ifndef __CROSSBILATERALWEIGHTS_HLSLI__
#define __CROSSBILATERALWEIGHTS_HLSLI__

#include "./../../../../assets/Shaders/HLSL/FloatPrecision.hlsli"
#include "./../../../../assets/Shaders/HLSL/SVGF.hlsli"

namespace CrossBilateral {
    namespace Normal {
        struct Parameters {
            float Sigma;
            float SigmaExponent;
        };

		// Get cross bilateral normal based weights.
        float4 GetWeights(
				in float3 targetNormal,
				in float3 sampleNormals[4],
				in Parameters params) {
            float4 NdotSampleN = float4(
				dot(targetNormal, sampleNormals[0]),
				dot(targetNormal, sampleNormals[1]),
				dot(targetNormal, sampleNormals[2]),
				dot(targetNormal, sampleNormals[3])
			);
			// Apply adjustment scale to the dot product. 
			// Values greater than 1 increase tolerance scale 
			// for unwanted inflated normal differences,
			// such as due to low-precision normal quantization.
            NdotSampleN *= params.Sigma;

            const float4 NormalWeights = pow(saturate(NdotSampleN), params.SigmaExponent);

            return NormalWeights;
        }
    }

	// Linear depth.
    namespace Depth {
        struct Parameters {
            float Sigma;
            float WeightCutoff;
            uint NumMantissaBits;
        };

        float4 GetWeights(
				in float targetDepth,
				in float2 ddxy,
				in float4 sampleDepths,
				in Parameters params) {
            const float DepthThreshold = dot(1, abs(ddxy));
            const float DepthFloatPrecision = FloatPrecision::FloatPrecision(targetDepth, params.NumMantissaBits);

            const float DepthTolerance = params.Sigma * DepthThreshold + DepthFloatPrecision;
            
            float4 depthWeights = min(DepthTolerance / (abs(sampleDepths - targetDepth) + DepthFloatPrecision), 1);
            depthWeights *= depthWeights >= params.WeightCutoff;

            return depthWeights;
        }

        float4 GetWeights(
				in float targetDepth,
				in float2 ddxy,
				in float4 sampleDepths,
				in float2 sampleOffset, // offset in-between the samples to remap ddxy for.
				in Parameters params) {
            const float2 RemappedDdxy = SVGF::RemapDdxy(targetDepth, ddxy, sampleOffset);
            
            return GetWeights(targetDepth, RemappedDdxy, sampleDepths, params);
        }
    }

    namespace Bilinear {
		// TargetOffset - offset from the top left ([0,0]) sample of the quad samples.
        float4 GetWeights(in float2 targetOffset) {
            const float4 BilinearWeights = float4(
				(1 - targetOffset.x) * (1 - targetOffset.y),
				targetOffset.x * (1 - targetOffset.y),
				(1 - targetOffset.x) * targetOffset.y,
				targetOffset.x * targetOffset.y
			);
            
            return BilinearWeights;
        }
    }

    namespace BilinearDepthNormal {
        struct Parameters {
            Normal::Parameters Normal;
            Depth::Parameters Depth;
        };

        float4 GetWeights(
				in float targetDepth,
				in float3 targetNormal,
				in float2 targetOffset,
				in float2 ddxy,
				in float4 sampleDepths,
				in float3 sampleNormals[4],
				in float2 samplesOffset,
				Parameters params) {
            const float4 BilinearWeights = Bilinear::GetWeights(targetOffset);
            const float4 DepthWeights = Depth::GetWeights(targetDepth, ddxy, sampleDepths, samplesOffset, params.Depth);
            const float4 NormalWeights = Normal::GetWeights(targetNormal, sampleNormals, params.Normal);

            return BilinearWeights * DepthWeights * NormalWeights;
        }

        float4 GetWeights(
				in float targetDepth,
				in float3 targetNormal,
				in float2 targetOffset,
				in float2 ddxy,
				in float4 sampleDepths,
				in float3 sampleNormals[4],
				Parameters params) {
            const float4 BilinearWeights = Bilinear::GetWeights(targetOffset);
            const float4 DepthWeights = Depth::GetWeights(targetDepth, ddxy, sampleDepths, params.Depth);
            const float4 NormalWeights = Normal::GetWeights(targetNormal, sampleNormals, params.Normal);

            return BilinearWeights * DepthWeights * NormalWeights;
        }
    }
}

#endif // __CROSSBILATERALWEIGHTS_HLSLI__