#pragma once

#ifndef _HLSL
	#include <type_traits>
	
	#include <Windows.h>

	#include <DirectXMath.h>
	
	#include "Common/Foundation/Util/HashUtil.hpp"
#endif

struct Vertex {
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;

	BOOL operator==(const Vertex& other) const;
};

#ifdef _HLSL
	#ifndef VERTEX_IN
	#define VERTEX_IN
		struct VertexIn {
			float3 Position : POSITION;
			float3 Normal	: NORMAL;
			float2 TexCoord : TEXCOORD;
		};
	#endif
#else
	namespace std {
		template<> 
		struct hash<Vertex> {
			Common::Foundation::Hash operator()(const Vertex& vert) const {
				Common::Foundation::Hash pos = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.Position.x));
				pos = Common::Foundation::Util::HashUtil::HashCombine(pos, static_cast<Common::Foundation::Hash>(vert.Position.y));
				pos = Common::Foundation::Util::HashUtil::HashCombine(pos, static_cast<Common::Foundation::Hash>(vert.Position.z));
	
				Common::Foundation::Hash normal = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.Normal.x));
				normal = Common::Foundation::Util::HashUtil::HashCombine(normal, static_cast<Common::Foundation::Hash>(vert.Normal.y));
				normal = Common::Foundation::Util::HashUtil::HashCombine(normal, static_cast<Common::Foundation::Hash>(vert.Normal.z));
	
				Common::Foundation::Hash texc = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(vert.TexCoord.x));
				texc = Common::Foundation::Util::HashUtil::HashCombine(texc, static_cast<Common::Foundation::Hash>(vert.TexCoord.y));
	
				return Common::Foundation::Util::HashUtil::HashCombine(Common::Foundation::Util::HashUtil::HashCombine(pos, normal), texc);
			}
		};
	}
#endif // #ifdef _HLSL