#ifndef __HLSLCOMPACTION_H__
#define __HLSLCOMPACTION_H__

#ifdef _HLSL
	#include "HlslCompaction.hlsli"
	#include "./../../../../inc/Common/Foundation/Mesh/Vertex.h"
	#include "./../../../../inc/Common/Foundation/Mesh/Material.h"
#else
	#include <dxgiformat.h>
#endif

#include "Render/DX/Foundation/ShadingConvention.h"
#include "Render/DX/Foundation/ConstantBuffer.h"

#endif // __HLSLCOMPACTION_H__