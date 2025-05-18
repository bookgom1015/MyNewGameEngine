#ifndef __VALUEPACKAGING_HLSLI__
#define __VALUEPACKAGING_HLSLI__

namespace ValuePackaging {
    uint Float2ToHalf(in float2 val) {
        uint result = 0;
        
        result = f32tof16(val.x);
        result |= f32tof16(val.y) << 16;
        
        return result;
    }

    float2 HalfToFloat2(in uint val) {
        float2 result = (float2)0;
        
        result.x = f16tof32(val);
        result.y = f16tof32(val >> 16);
        
        return result;
    }

    uint Float4ToUint(in float4 val) {
        uint result = 0;
        
        result = asuint(val.x);
        result |= asuint(val.y) << 8;
        result |= asuint(val.z) << 16;
        result |= asuint(val.w) << 24;
        
        return result;
    }

    float4 UintToFloat4(in uint val) {
        float4 result = (float4)0;
        
        result.x = val;
        result.y = val >> 8;
        result.z = val >> 16;
        result.w = val >> 24;
        
        return result;
    }

    /***************************************************************/
    // Normal encoding
    // Ref: https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
    float2 OctWrap(in float2 v) {
        return (1.f - abs(v.yx)) * select(v.xy >= 0.f, 1.f, -1.f);
    }

    // Converts a 3D unit vector to a 2D vector with <0,1> range. 
    float2 EncodeNormal(in float3 n) {
        n /= (abs(n.x) + abs(n.y) + abs(n.z));
        n.xy = n.z >= 0.f ? n.xy : OctWrap(n.xy);
        n.xy = n.xy * 0.5f + 0.5f;
        
        return n.xy;
    }

    float3 DecodeNormal(in float2 f) {
        f = f * 2.f - 1.f;
        
	    // https://twitter.com/Stubbesaurus/status/937994790553227264
        float3 n = float3(f.x, f.y, 1.f - abs(f.x) - abs(f.y));        
        
        const float t = saturate(-n.z);        
        n.xy += select(n.xy >= 0.f, -t, t);
        
        return normalize(n);
    }

    // Pack [0.0, 1.0] float to 8 bit uint. 
    uint Pack_R8_FLOAT(in float r) {
        return clamp(round(r * 255), 0, 255);
    }

    float Unpack_R8_FLOAT(in uint r) {
        return (r & 0xFF) / 255.f;
    }

    // pack two 8 bit uint2 into a 16 bit uint.
    uint Pack_R8G8_to_R16_UINT(in uint r, in uint g) {
        return (r & 0xFF) | ((g & 0xFF) << 8);
    }

    void Unpack_R16_to_R8G8_UINT(in uint v, out uint r, out uint g) {
        r = v & 0xFF;
        g = (v >> 8) & 0xFF;
    }

    // Pack unsigned floating point, where 
    // - rgb.rg are in [0, 1] range stored as two 8 bit uints.
    // - rgb.b in [0, FLT_16_BIT_MAX] range stored as a 16bit float.
    uint Pack_R8G8B16_FLOAT(in float3 rgb) {
        const uint r = Pack_R8_FLOAT(rgb.r);
        const uint g = Pack_R8_FLOAT(rgb.g) << 8;
        const uint b = f32tof16(rgb.b) << 16;
        
        return r | g | b;
    }

    float3 Unpack_R8G8B16_FLOAT(in uint rgb) {
        const float r = Unpack_R8_FLOAT(rgb);
        const float g = Unpack_R8_FLOAT(rgb >> 8);
        const float b = f16tof32(rgb >> 16);
        
        return float3(r, g, b);
    }

    uint NormalizedFloat3ToByte3(in float3 v) {
        return (uint(v.x * 255) << 16) + (uint(v.y * 255) << 8) + uint(v.z * 255);
    }

    float3 Byte3ToNormalizedFloat3(in uint v) {
        return float3((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF) / 255;
    }

    // Encode normal and depth with 16 bits allocated for each.
    uint EncodeNormalDepth_N16D16(in float3 normal, in float depth) {
        const float3 encodedNormalDepth = float3(EncodeNormal(normal), depth);
        
        return Pack_R8G8B16_FLOAT(encodedNormalDepth);
    }

    // Decoded 16 bit normal and 16bit depth.
    void DecodeNormalDepth_N16D16(in uint packedEncodedNormalAndDepth, out float3 normal, out float depth) {
        const float3 encodedNormalDepth = Unpack_R8G8B16_FLOAT(packedEncodedNormalAndDepth);
        
        normal = DecodeNormal(encodedNormalDepth.xy);
        depth = encodedNormalDepth.z;
    }

    uint EncodeNormalDepth(in float3 normal, in float depth) {
        return EncodeNormalDepth_N16D16(normal, depth);
    }

    void DecodeNormalDepth(in uint encodedNormalDepth, out float3 normal, out float depth) {
        DecodeNormalDepth_N16D16(encodedNormalDepth, normal, depth);
    }

    void DecodeNormal(in uint encodedNormalDepth, out float3 normal) {
        float depthDummy;
        DecodeNormalDepth_N16D16(encodedNormalDepth, normal, depthDummy);
    }

    void DecodeDepth(in uint encodedNormalDepth, out float depth) {
        float3 normalDummy;
        DecodeNormalDepth_N16D16(encodedNormalDepth, normalDummy, depth);
    }

    void UnpackEncodedNormalDepth(in uint packedEncodedNormalDepth, out float2 encodedNormal, out float depth) {
        const float3 encodedNormalDepth = Unpack_R8G8B16_FLOAT(packedEncodedNormalDepth);
        
        encodedNormal = encodedNormalDepth.xy;
        depth = encodedNormalDepth.z;
    }
}

#endif // __VALUEPACKAGING_HLSLI__