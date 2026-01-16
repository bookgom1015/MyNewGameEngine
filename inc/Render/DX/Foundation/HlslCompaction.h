#ifndef __HLSLCOMPACTION_H__
#define __HLSLCOMPACTION_H__

#ifdef _HLSL
	#include "./../../../assets/Shaders/HLSL/HlslCompaction.hlsli" 
	#include "./../../../inc/Render/DX/Foundation/ConstantBuffer.h"
	#include "./../../../assets/Shaders/HLSL/HardCodedCoordinates.hlsli"
	#include "./../../../assets/Shaders/HLSL/ShaderConstants.hlsli"

	#include "./../../../inc/Render/DX/Foundation/ShadingConvention.h"
	#include "./../../../assets/Shaders/HLSL/ShaderUtil.hlsli"

	#include "./../../../assets/Shaders/HLSL/Random.hlsli"
	#include "./../../../inc/Common/Foundation/Mesh/Vertex.h"
	#include "./../../../inc/Common/Foundation/Mesh/Material.h"
#else
	#include <DirectXMath.h>
	#include <dxgiformat.h>

	#include "Render/DX/Foundation/ShadingConvention.h"
	#include "Render/DX/Foundation/ConstantBuffer.h"
#endif

#endif // __HLSLCOMPACTION_H__