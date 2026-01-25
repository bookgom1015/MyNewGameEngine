#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

using namespace Render::DX11;

Microsoft::WRL::ComPtr<ID3D11SamplerState> Shading::Util::SamplerUtil::msSamplers[SamplerState::Type::Count]{};

ID3D11SamplerState* Shading::Util::SamplerUtil::msSamplerPtrs[SamplerState::Type::Count]{};

BOOL Shading::Util::SamplerUtil::Initialize(
        Common::Debug::LogFile* const pLogFile, Foundation::Core::Device* const pDevice) {
    // PointWrap
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        CheckReturn(pLogFile, pDevice->CreateSamplerState(&desc, &msSamplers[SamplerState::E_PointWrap]));
    }
    // PointClamp
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        CheckReturn(pLogFile, pDevice->CreateSamplerState(&desc, &msSamplers[SamplerState::E_PointClamp]));
    }
    // LinearWrap
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        CheckReturn(pLogFile, pDevice->CreateSamplerState(&desc, &msSamplers[SamplerState::E_LinearWrap]));
    }
    // LinearClamp
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        CheckReturn(pLogFile, pDevice->CreateSamplerState(&desc, &msSamplers[SamplerState::E_LinearClamp]));
    }
    // Shadow
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        CheckReturn(pLogFile, pDevice->CreateSamplerState(&desc, &msSamplers[SamplerState::E_Shadow]));
    }

    for (size_t i = 0; i < SamplerState::Type::Count; ++i)
        msSamplerPtrs[i] = msSamplers[i].Get();

    return true;
}

void Shading::Util::SamplerUtil::CleanUp() {
    for (UINT i = 0; i < SamplerState::Type::Count; ++i)
        msSamplers[i].Reset();
}