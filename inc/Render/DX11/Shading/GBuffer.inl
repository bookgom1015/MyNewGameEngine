#ifndef __GBUFFER_INL__
#define __GBUFFER_INL__

namespace Render::DX11::Shading::GBuffer {
	ID3D11ShaderResourceView* GBufferClass::AlbedoMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Albedo].Get();
	}

	ID3D11ShaderResourceView* GBufferClass::NormalMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Normal].Get();
	}

	ID3D11ShaderResourceView* GBufferClass::PositionMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Position].Get();
	}

	ID3D11ShaderResourceView* GBufferClass::RoughnessMetalnessMapSrv() {
		return mhSrvs[Descriptor::Srv::E_RoughnessMetalness].Get();
	}

	ID3D11ShaderResourceView* GBufferClass::VelocityMapSrv() {
		return mhSrvs[Descriptor::Srv::E_Velocity].Get();
	}
}

#endif // __GBUFFER_INL__