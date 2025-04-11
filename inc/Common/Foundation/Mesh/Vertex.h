#pragma once

#ifndef _HLSL
	#include <type_traits>
	
	#include <Windows.h>

	#include <d3d12.h>
	#include <DirectXMath.h>
	
	#include "Common/Util/HashUtil.hpp"
#endif

namespace Common {
	namespace Foundation {
		namespace Mesh {
			struct Vertex {
				DirectX::XMFLOAT3 Position;
				DirectX::XMFLOAT3 Normal;
				DirectX::XMFLOAT2 TexCoord;

#ifndef _HLSL
				BOOL operator==(const Vertex& other) const;

				static D3D12_INPUT_LAYOUT_DESC InputLayoutDesc();
#endif
			};
		}
	}
}

#ifdef _HLSL
	#ifndef VERTEX_IN
	#define VERTEX_IN
		struct VertexIn {
			float3 PosL		: POSITION;
			float3 Normal	: NORMAL;
			float2 TexC		: TEXCOORD;
		};
	#endif
#else
	namespace std {
		template<> 
		struct hash<Common::Foundation::Mesh::Vertex> {
			Common::Foundation::Hash operator()(const Common::Foundation::Mesh::Vertex& vert) const {
				Common::Foundation::Hash pos = Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.Position.x));
				pos = Common::Util::HashUtil::HashCombine(pos, static_cast<Common::Foundation::Hash>(vert.Position.y));
				pos = Common::Util::HashUtil::HashCombine(pos, static_cast<Common::Foundation::Hash>(vert.Position.z));
	
				Common::Foundation::Hash normal = Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.Normal.x));
				normal = Common::Util::HashUtil::HashCombine(normal, static_cast<Common::Foundation::Hash>(vert.Normal.y));
				normal = Common::Util::HashUtil::HashCombine(normal, static_cast<Common::Foundation::Hash>(vert.Normal.z));
	
				Common::Foundation::Hash texc = Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.TexCoord.x));
				texc = Common::Util::HashUtil::HashCombine(texc, static_cast<Common::Foundation::Hash>(vert.TexCoord.y));
	
				return Common::Util::HashUtil::HashCombine(Common::Util::HashUtil::HashCombine(pos, normal), texc);
			}
		};
	}
#endif // #ifdef _HLSL