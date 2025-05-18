#ifndef __SVGF_HLSLI__
#define __SVGF_HLSLI__

namespace SVGF {
    float3 BoxCubeMapLookup(in float3 rayOrigin, in float3 unitRayDir, in float3 boxCenter, in float3 boxExtents) {
	    // Set the origin to box of center.
        const float3 p = rayOrigin - boxCenter;

	    // The formula for AABB's i-th plate ray versus plane intersection is as follows.
	    //
	    // t1 = (-dot(n_i, p) + h_i) / dot(n_i, d) = (-p_i + h_i) / d_i
	    // t2 = (-dot(n_i, p) - h_i) / dot(n_i, d) = (-p_i - h_i) / d_i
        const float3 t1 = (-p + boxExtents) / unitRayDir;
        const float3 t2 = (-p - boxExtents) / unitRayDir;

	    // Find the maximum value for each coordinate component.
	    // We assume that ray is inside the box, so we only need to find the maximum value of the intersection parameter. 
        const float3 tmax = max(t1, t2);

	    // Find the minimum value of all components for tmax.
        const float t = min(min(tmax.x, tmax.y), tmax.z);

	    // To use a lookup vector for a cube map, 
	    // create coordinate relative to center of box.
        return p + t * unitRayDir;
    }

    uint GetIndexOfValueClosestToReference(in float ref, in float2 values) {
        const float2 delta = abs(ref - values);
    
        const uint index = delta[1] < delta[0] ? 1 : 0;
    
        return index;
    }

    uint GetIndexOfValueClosestToReference(in float ref, in float4 values) {
        const float4 delta = abs(ref - values);
    
        uint index = delta[1] < delta[0] ? 1 : 0;
        index = delta[2] < delta[index] ? 2 : index;
        index = delta[3] < delta[index] ? 3 : index;
    
        return index;
    }

    bool IsWithinBounds(in int2 index, in int2 dimensions) {
        return index.x >= 0 && index.y >= 0 && index.x < dimensions.x && index.y < dimensions.y;
    }

    // [ Descriptions ]
    //  Remap partial depth derivatives at z0 from [1,1] pixel offset to a new pixel offset.
    float2 RemapDdxy(in float z0, in float2 ddxy, in float2 pixelOffset) {
	    // Perspective correction for non-linear depth interpolation.
	    // Ref: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation
	    // Given a linear depth interpolation for finding z at offset q along z0 to z1
	    //           z =  1 / (1 / z0 * (1 - q) + 1 / z1 * q)
	    // and z1 = z0 + ddxy, where z1 is at a unit pixel offset [1, 1]
	    // z can be calculated via ddxy as
	    //
	    //           z = (z0 + ddxy) / (1 + (1-q) / z0 * ddxy) 
        const float2 z = (z0 + ddxy) / (1 + ((1 - pixelOffset) / z0) * ddxy);
    
        return sign(pixelOffset) * (z - z0);
    }

    bool IsInRange(in float val, in float min, in float max) {
        return (val >= min && val <= max);
    }

    // [ Descriptions ]
    //  Returns an approximate surface dimensions covered in a pixel. 
    //  This is a simplified model assuming pixel to pixel view angles are the same.
    // [ Parameters ]
    //  - z		: linear depth of the surface at the pixel
    //  - ddxy	: partial depth derivatives
    //  - tan_a : tangent of a per pixel view angle 
    float2 ApproximateProjectedSurfaceDimensionsPerPixel(in float z, in float2 ddxy, in float tan_a) {
	    // Surface dimensions for a surface parallel at z.
        const float2 dx = tan_a * z;
	    // Using Pythagorean theorem approximate the surface dimensions given the ddxy.
        const float2 w = sqrt(dx * dx + ddxy * ddxy);
    
        return w;
    }

    float ColorVariance(in float4 lval, in float4 rval) {
        const float3 diff = (lval - rval).rgb;
        const float variance = sqrt(dot(diff, diff)) * 0.577350269189;
    
        return variance;
    }
}

#endif // __SVGF_HLSLI__