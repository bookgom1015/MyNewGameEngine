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
			float3 Normal : NORMAL;
			float2 TexCoord : TEXCOORD;
		};
	#endif
#else
	namespace std {
		template<> 
		struct hash<Vertex> {
			size_t operator()(const Vertex& vert) const {
				size_t pos = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<size_t>(vert.Position.x));
				pos = Common::Foundation::Util::HashUtil::HashCombine(pos, static_cast<size_t>(vert.Position.y));
				pos = Common::Foundation::Util::HashUtil::HashCombine(pos, static_cast<size_t>(vert.Position.z));
	
				size_t normal = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<size_t>(vert.Normal.x));
				normal = Common::Foundation::Util::HashUtil::HashCombine(normal, static_cast<size_t>(vert.Normal.y));
				normal = Common::Foundation::Util::HashUtil::HashCombine(normal, static_cast<size_t>(vert.Normal.z));
	
				size_t texc = Common::Foundation::Util::HashUtil::HashCombine(0, static_cast<size_t>(vert.TexCoord.x));
				texc = Common::Foundation::Util::HashUtil::HashCombine(texc, static_cast<size_t>(vert.TexCoord.y));
	
				return Common::Foundation::Util::HashUtil::HashCombine(Common::Foundation::Util::HashUtil::HashCombine(pos, normal), texc);
			}
		};
	}
#endif // #ifdef _HLSL