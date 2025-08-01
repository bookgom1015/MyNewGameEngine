#include "Render/DX/Shading/SVGF.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_CalcPartialDepthDerivative = L"CalcPartialDepthDerivative.hlsl";
	const WCHAR* const HLSL_CalcLocalMeanVariance = L"CalcLocalMeanVariance.hlsl";
	const WCHAR* const HLSL_FillInCheckboard = L"FillInCheckerboard_CrossBox4TapFilter.hlsl";
	const WCHAR* const HLSL_TemporalSupersamplingReverseReproject = L"TemporalSupersamplingReverseReproject.hlsl";
	const WCHAR* const HLSL_TemporalSupersamplingBlendWithCurrentFrame = L"TemporalSupersamplingBlendWithCurrentFrame.hlsl";
	const WCHAR* const HLSL_EdgeStoppingFilterGaussian3x3 = L"EdgeStoppingFilter_Gaussian3x3.hlsl";
	const WCHAR* const HLSL_DisocclusionBlur3x3 = L"DisocclusionBlur3x3.hlsl";
}

SVGF::InitDataPtr SVGF::MakeInitData() {
	return std::unique_ptr<SVGFClass::InitData>(new SVGFClass::InitData());
}

SVGF::SVGFClass::SVGFClass() {
	for (UINT i = 0; i < Resource::Count; ++i)
		mResources[i] = std::make_unique<Foundation::Resource::GpuResource>();

	for (UINT i = 0; i < 2; ++i)
		mDebugMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT SVGF::SVGFClass::CbvSrvUavDescCount() const { return 0 
	+ Descriptor::Count
	+ 2 // DebugMap
	; 
}

UINT SVGF::SVGFClass::RtvDescCount() const { return 0; }

UINT SVGF::SVGFClass::DsvDescCount() const { return 0; }

BOOL SVGF::SVGFClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL SVGF::SVGFClass::CompileShaders() {
	// CalcParticalDepthDerivative
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CalcPartialDepthDerivative, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CalcParticalDepthDerivative]));
	}
	// CalcLocalMeanVariance_Contrast
	{
		DxcDefine defines[] = {
			{ L"ValueType_Contrast", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Contrast" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast" },
			{ L"InvalidValue", L"-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CalcLocalMeanVariance, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CalcLocalMeanVariance_Contrast]));
	}
	// CalcLocalMeanVariance_Color
	{
		DxcDefine defines[] = {
			{ L"ValueType_Color", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Color" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color" },
			{ L"InvalidValue", L"(float4)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CalcLocalMeanVariance, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CalcLocalMeanVariance_Color]));
	}
	// FillInCheckerboard
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_FillInCheckboard, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_FillinCheckerboard]));
	}
	// TemporalSupersamplingReverseReproject_Contrast
	{
		DxcDefine defines[] = {
			{ L"ValueType_Contrast", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Contrast" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast" },
			{ L"InvalidValue", L"(float)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TemporalSupersamplingReverseReproject, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_TemporalSupersamplingReverseReproject_Contrast]));
	}
	// TemporalSupersamplingReverseReproject_Color
	{
		DxcDefine defines[] = {
			{ L"ValueType_Color", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Color" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color" },
			{ L"InvalidValue", L"(float4)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TemporalSupersamplingReverseReproject, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_TemporalSupersamplingReverseReproject_Color]));
	}
	// TemporalSupersamplingBlendWithCurrentFrame_Contrast
	{
		DxcDefine defines[] = {
			{ L"ValueType_Contrast", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Contrast" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast" },
			{ L"InvalidValue", L"(float)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TemporalSupersamplingBlendWithCurrentFrame, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_TemporalSupersamplingBlendWithCurrentFrame_Contrast]));
	}
	// TemporalSupersamplingBlendWithCurrentFrame_Color
	{
		DxcDefine defines[] = {
			{ L"ValueType_Color", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Color" },
			{ L"ValueSquaredMeanType", L"ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color" },
			{ L"InvalidValue", L"(float4)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TemporalSupersamplingBlendWithCurrentFrame, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_TemporalSupersamplingBlendWithCurrentFrame_Color]));
	}
	// EdgeStoppingFilterGaussian3x3_Contrast
	{
		DxcDefine defines[] = {
			{ L"ValueType_Contrast", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Contrast" },
			{ L"InvalidValue", L"(float)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_EdgeStoppingFilterGaussian3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_EdgeStoppingFilterGaussian3x3_Contrast]));
	}
	// EdgeStoppingFilterGaussian3x3_Color
	{
		DxcDefine defines[] = {
			{ L"ValueType_Color", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Color" },
			{ L"InvalidValue", L"(float4)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_EdgeStoppingFilterGaussian3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_EdgeStoppingFilterGaussian3x3_Color]));
	}
	// DisocclusionBlur3x3_Contrast
	{
		DxcDefine defines[] = {
			{ L"ValueType_Contrast", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Contrast" },
			{ L"InvalidValue", L"(float)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DisocclusionBlur3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DisocclusionBlur3x3_Contrast]));
	}
	// DisocclusionBlur3x3_Color
	{
		DxcDefine defines[] = {
			{ L"ValueType_Color", L"1" },
			{ L"ValueType", L"ShadingConvention::SVGF::ValueMapFormat_Color" },
			{ L"InvalidValue", L"(float4)-1.f" }
		};
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DisocclusionBlur3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DisocclusionBlur3x3_Color]));
	}
	
	return TRUE;
}

BOOL SVGF::SVGFClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	// TemporalSupersamplingReverseReproject
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[15] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::Count] = {};
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::CB_CrossBilateralFilter].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::RC_Consts].InitAsConstants(ShadingConvention::SVGF::RootConstant::TemporalSupersamplingReverseReproject::Count, 1);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_NormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_ReprojectedNormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_Velocity].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_CachedNormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValueSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_CachedTSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::SI_CachedRayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_CachedTSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_CachedValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_CachedSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_TSPPSquaredMeanRayHitDistacne].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_DebugMap0].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingReverseReproject::UO_DebugMap1].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc, 
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_TemporalSupersamplingReverseReproject]), 
			L"SVGF_GR_TemporalSupersamplingReverseReproject"));
	}
	// TemporalSupersamplingBlendWithCurrentFrame
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[12] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::Count] = {};
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::CB_TSPPBlendWithCurrentFrame].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_LocalMeanVaraince].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_RayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_CachedValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_CachedSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_TSPPSquaredMeanRayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TemporalAOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_AOCoefficientSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_RayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_VarianceMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_BlurStrength].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_TemporalSupersamplingBlendWithCurrentFrame]),
			L"SVGF_GR_TemporalSupersamplingBlendWithCurrentFrame"));
	}
	// CalculateDepthPartialDerivative
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CalcDepthPartialDerivative::Count] = {};
		slotRootParameter[RootSignature::CalcDepthPartialDerivative::RC_Consts].InitAsConstants(ShadingConvention::SVGF::RootConstant::CalcDepthPartialDerivative::Count, 0, 0);
		slotRootParameter[RootSignature::CalcDepthPartialDerivative::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CalcDepthPartialDerivative::UO_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_CalcDepthPartialDerivative]),
			L"SVGF_GR_CalcDepthPartialDerivative"));
	}
	// CalculateMeanVariance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CalcLocalMeanVariance::Count] = {};
		slotRootParameter[RootSignature::CalcLocalMeanVariance::CB_LocalMeanVariance].InitAsConstantBufferView(0, 0);
		slotRootParameter[RootSignature::CalcLocalMeanVariance::SI_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CalcLocalMeanVariance::UO_LocalMeanVariance].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_CalcLocalMeanVariance]),
			L"SVGF_GR_CalcLocalMeanVariance"));
	}
	// FillInCheckerboard
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::FillInCheckerboard::Count] = {};
		slotRootParameter[RootSignature::FillInCheckerboard::CB_LocalMeanVariance].InitAsConstantBufferView(0, 0);
		slotRootParameter[RootSignature::FillInCheckerboard::UIO_LocalMeanVariance].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_FillInCheckerboard]),
			L"SVGF_GR_FillInCheckerboard"));
	}
	// Atrous Wavelet transform filter
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[7] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::AtrousWaveletTransformFilter::Count] = {};
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::CB_AtrousFilter].InitAsConstantBufferView(0, 0);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::RC_Consts].InitAsConstants(ShadingConvention::SVGF::RootConstant::AtrousWaveletTransformFilter::Count, 1);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_TemporalValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_NormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_Variance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_HitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::SI_TSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::AtrousWaveletTransformFilter::UO_TemporalValue].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_AtrousWaveletTransformFilter]),
			L"SVGF_GR_AtrousWaveletTransformFilter"));
	}
	// Disocclusion blur
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[4] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DisocclusionBlur::Count] = {};
		slotRootParameter[RootSignature::DisocclusionBlur::RC_Consts].InitAsConstants(ShadingConvention::SVGF::RootConstant::DisocclusionBlur::Count, 0);
		slotRootParameter[RootSignature::DisocclusionBlur::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DisocclusionBlur::SI_BlurStrength].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DisocclusionBlur::SI_RoughnessMetalnessMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DisocclusionBlur::UIO_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(mpLogFile, mInitData.Device->CreateRootSignature(
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_DisocclusionBlur]),
			L"SVGF_GR_DisocclusionBlur"));
	}

	return TRUE;
}

BOOL SVGF::SVGFClass::BuildPipelineStates() {
	// CalcDepthPartialDerivative
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_CalcDepthPartialDerivative].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CalcParticalDepthDerivative]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_CalcDepthPartialDerivative]),
			L"SVGF_CP_CalcDepthPartialDerivative"));
	}
	// CalcLocalMeanVariance_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_CalcLocalMeanVariance].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CalcLocalMeanVariance_Contrast]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_CalcLocalMeanVariance_Contrast]),
			L"SVGF_CP_CalcLocalMeanVariance_Contrast"));
	}
	// CalcLocalMeanVariance_Color
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_CalcLocalMeanVariance].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CalcLocalMeanVariance_Color]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_CalcLocalMeanVariance_Color]),
			L"SVGF_CP_CalcLocalMeanVariance_Color"));
	}
	// FillinCheckerboard
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_FillInCheckerboard].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_FillinCheckerboard]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_FillInCheckerboard]),
			L"SVGF_CP_FillInCheckerboard"));
	}
	// TemporalSupersamplingReverseReproject_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_TemporalSupersamplingReverseReproject].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_TemporalSupersamplingReverseReproject_Contrast]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_TemporalSupersamplingReverseReproject_Contrast]),
			L"SVGF_CP_TemporalSupersamplingReverseReproject_Contrast"));
	}
	// TemporalSupersamplingReverseReproject_Color
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_TemporalSupersamplingReverseReproject].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_TemporalSupersamplingReverseReproject_Color]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_TemporalSupersamplingReverseReproject_Color]),
			L"SVGF_CP_TemporalSupersamplingReverseReproject_Color"));
	}
	// TemporalSupersamplingBlendWithCurrentFrame_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_TemporalSupersamplingBlendWithCurrentFrame].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_TemporalSupersamplingBlendWithCurrentFrame_Contrast]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_TemporalSupersamplingBlendWithCurrentFrame_Contrast]),
			L"SVGF_CP_TemporalSupersamplingBlendWithCurrentFrame_Contrast"));
	}
	// TemporalSupersamplingBlendWithCurrentFrame_Color
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_TemporalSupersamplingBlendWithCurrentFrame].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_TemporalSupersamplingBlendWithCurrentFrame_Color]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_TemporalSupersamplingBlendWithCurrentFrame_Color]),
			L"SVGF_CP_TemporalSupersamplingBlendWithCurrentFrame_Color"));
	}
	// E_EdgeStoppingFilterGaussian3x3_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_AtrousWaveletTransformFilter].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_EdgeStoppingFilterGaussian3x3_Contrast]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_EdgeStoppingFilterGaussian3x3_Contrast]),
			L"SVGF_CP_EdgeStoppingFilterGaussian3x3_Color"));
	}
	// E_EdgeStoppingFilterGaussian3x3_Color
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_AtrousWaveletTransformFilter].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_EdgeStoppingFilterGaussian3x3_Color]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_EdgeStoppingFilterGaussian3x3_Color]),
			L"SVGF_CP_EdgeStoppingFilterGaussian3x3_Color"));
	}
	// DisocclusionBlur3x3_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_DisocclusionBlur].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DisocclusionBlur3x3_Contrast]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_DisocclusionBlur_Contrast]),
			L"SVGF_CP_DisocclusionBlur3x3_Color"));
	}
	// DisocclusionBlur3x3_Color
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_DisocclusionBlur].Get();
		{
			const auto cs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DisocclusionBlur3x3_Color]);
			psoDesc.CS = { reinterpret_cast<BYTE*>(cs->GetBufferPointer()), cs->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(mpLogFile, mInitData.Device->CreateComputePipelineState(
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::E_DisocclusionBlur_Color]),
			L"SVGF_CP_DisocclusionBlur3x3_Color"));
	}
	return TRUE;
}

BOOL SVGF::SVGFClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	for (UINT i = 0; i < Descriptor::Count; ++i) {
		mhCpuDecs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhGpuDecs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
	}

	for (UINT i = 0; i < 2; ++i) {
		mhDebugMapCpuUavs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhDebugMapGpuUavs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
	}

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SVGF::SVGFClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SVGF::SVGFClass::CalculateDepthParticalDerivative(
		Foundation::Resource::FrameResource* const pFrameResource, 
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::E_CalcDepthPartialDerivative].Get()));
	
	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);
	
	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_CalcDepthPartialDerivative].Get());
	
		const auto DepthPartialDerivative = mResources[Resource::E_DepthPartialDerivative].get();
	
		DepthPartialDerivative->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, DepthPartialDerivative);
	
		ShadingConvention::SVGF::RootConstant::CalcDepthPartialDerivative::Struct rc;
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.ClientWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.ClientHeight);
	
		std::array<std::uint32_t, ShadingConvention::SVGF::RootConstant::CalcDepthPartialDerivative::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::SVGF::RootConstant::CalcDepthPartialDerivative::Struct));
	
		CmdList->SetComputeRoot32BitConstants(RootSignature::CalcDepthPartialDerivative::RC_Consts, ShadingConvention::SVGF::RootConstant::CalcDepthPartialDerivative::Count, consts.data(), 0);
		CmdList->SetComputeRootDescriptorTable(RootSignature::CalcDepthPartialDerivative::SI_DepthMap, si_depthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::CalcDepthPartialDerivative::UO_DepthPartialDerivative, mhGpuDecs[Descriptor::EU_DepthPartialDerivative]);
	
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Default::Height), 
			ShadingConvention::SVGF::ThreadGroup::Default::Depth);
	}
	
	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::CalculateLocalMeanVariance(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pValueMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap,
		Value::Type type,
		BOOL bCheckerboardSamplingEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type == Value::Type::E_Contrast ?
		PipelineState::E_CalcLocalMeanVariance_Contrast:
		PipelineState::E_CalcLocalMeanVariance_Color].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_CalcLocalMeanVariance].Get());
	
		const auto RawLocalMeanVariance = mResources[SVGF::Resource::LocalMeanVariance::E_Raw].get();
		RawLocalMeanVariance->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, RawLocalMeanVariance);
	
		const auto uo_localMeanVariance = mhGpuDecs[SVGF::Descriptor::LocalMeanVariance::EU_Raw];	

		CmdList->SetComputeRootConstantBufferView(RootSignature::CalcLocalMeanVariance::CB_LocalMeanVariance, pFrameResource->CalcLocalMeanVarianceCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::CalcLocalMeanVariance::SI_AOCoefficient, si_valueMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::CalcLocalMeanVariance::UO_LocalMeanVariance, uo_localMeanVariance);
	
		const INT PixelStepY = bCheckerboardSamplingEnabled ? 2 : 1;
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Default::Height * PixelStepY),
			ShadingConvention::SVGF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::FillInCheckerboard(
		Foundation::Resource::FrameResource* const pFrameResource,
		BOOL bCheckerboardSamplingEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::E_FillInCheckerboard].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_FillInCheckerboard].Get());

		const auto pLocalMeanVarMap = mResources[Resource::LocalMeanVariance::E_Raw].get();
		pLocalMeanVarMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pLocalMeanVarMap);

		CmdList->SetComputeRootConstantBufferView(RootSignature::FillInCheckerboard::CB_LocalMeanVariance, pFrameResource->CalcLocalMeanVarianceCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::FillInCheckerboard::UIO_LocalMeanVariance, mhGpuDecs[Descriptor::LocalMeanVariance::EU_Raw]);

		const INT PixelStepY = bCheckerboardSamplingEnabled ? 2 : 1;
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Default::Height * PixelStepY),
			ShadingConvention::SVGF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::ReverseReprojectPreviousFrame(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
		Foundation::Resource::GpuResource* const pReprojNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_reprojNormalDepthMap,
		Foundation::Resource::GpuResource* const pCachedNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cachedNormalDepthMap,
		Foundation::Resource::GpuResource* const pVelocityMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
		Foundation::Resource::GpuResource* const pCachedValueMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueMap,
		Foundation::Resource::GpuResource* const pCachedValueSquaredMeanMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueSquaredMeanMap,
		Foundation::Resource::GpuResource* const pCachedRayHitDistMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cachedRayHitDistMap,
		Foundation::Resource::GpuResource* const pCachedTSPPMap0,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cachedTSPPMap,
		Foundation::Resource::GpuResource* const pCachedTSPPMap1,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_cachedTSPPMap,
		Value::Type type) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type == Value::Type::E_Contrast ? 
		PipelineState::E_TemporalSupersamplingReverseReproject_Contrast : 
		PipelineState::E_TemporalSupersamplingReverseReproject_Color].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_TemporalSupersamplingReverseReproject].Get());

		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pReprojNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pVelocityMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedValueSquaredMeanMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedRayHitDistMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedTSPPMap0->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pCachedTSPPMap0->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pCachedTSPPMap0);

		const auto pTSPPSquaredMeanRayHitDist = mResources[Resource::E_TSPPSquaredMeanRayHitDistance].get();
		pTSPPSquaredMeanRayHitDist->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTSPPSquaredMeanRayHitDist);

		for (UINT i = 0; i < 2; ++i) {
			mDebugMaps[i]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Foundation::Util::D3D12Util::UavBarrier(CmdList, mDebugMaps[i].get());
		}

		Foundation::Resource::GpuResource* pCachedValue;
		Foundation::Resource::GpuResource* pCachedSquaredMean;
		D3D12_GPU_DESCRIPTOR_HANDLE hCachedValue;
		D3D12_GPU_DESCRIPTOR_HANDLE hCachedSquaredMean;
		if (type == Value::Type::E_Contrast) {
			pCachedValue = mResources[Resource::CachedValue::E_Contrast].get();
			hCachedValue = mhGpuDecs[Descriptor::CachedValue::EU_Contrast];

			pCachedSquaredMean = mResources[Resource::CachedSquaredMean::E_Contrast].get();
			hCachedSquaredMean = mhGpuDecs[Descriptor::CachedSquaredMean::EU_Contrast];
		}
		else {
			pCachedValue = mResources[Resource::CachedValue::E_Color].get();
			hCachedValue = mhGpuDecs[Descriptor::CachedValue::EU_Color];

			pCachedSquaredMean = mResources[Resource::CachedSquaredMean::E_Color].get();
			hCachedSquaredMean = mhGpuDecs[Descriptor::CachedSquaredMean::EU_Color];
		}

		pCachedValue->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pCachedValue);

		pCachedSquaredMean->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pCachedSquaredMean);

		CmdList->SetComputeRootConstantBufferView(RootSignature::TemporalSupersamplingReverseReproject::CB_CrossBilateralFilter, pFrameResource->CrossBilateralFilterCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_NormalDepth, si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_ReprojectedNormalDepth, si_reprojNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_Velocity, si_velocityMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_DepthPartialDerivative, mhGpuDecs[Descriptor::ES_DepthPartialDerivative]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_CachedNormalDepth, si_cachedNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValue, si_cachedValueMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValueSquaredMean, si_cachedValueSquaredMeanMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_CachedTSPP, si_cachedTSPPMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::SI_CachedRayHitDistance, si_cachedRayHitDistMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_CachedTSPP, uo_cachedTSPPMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_CachedValue, hCachedValue);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_CachedSquaredMean, hCachedSquaredMean);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_TSPPSquaredMeanRayHitDistacne, mhGpuDecs[Descriptor::EU_TSPPSquaredMeanRayHitDistance]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_DebugMap0, mhDebugMapGpuUavs[0]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingReverseReproject::UO_DebugMap1, mhDebugMapGpuUavs[1]);

		ShadingConvention::SVGF::RootConstant::TemporalSupersamplingReverseReproject::Struct rc;
		rc.gTexDim = { static_cast<FLOAT>(mInitData.ClientWidth), static_cast<FLOAT>(mInitData.ClientHeight) };
		rc.gInvTexDim = { 1.f / static_cast<FLOAT>(mInitData.ClientWidth), 1.f / static_cast<FLOAT>(mInitData.ClientHeight) };

		std::array<std::uint32_t, ShadingConvention::SVGF::RootConstant::TemporalSupersamplingReverseReproject::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::SVGF::RootConstant::TemporalSupersamplingReverseReproject::Struct));

		CmdList->SetComputeRoot32BitConstants(
			RootSignature::TemporalSupersamplingReverseReproject::RC_Consts, 
			ShadingConvention::SVGF::RootConstant::TemporalSupersamplingReverseReproject::Count,
			consts.data(), 
			0);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Default::Height),
			ShadingConvention::SVGF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::BlendWithCurrentFrame(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pValueMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap,
		Foundation::Resource::GpuResource* const pRayHitDistanceMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_rayHitDistanceMap,
		Foundation::Resource::GpuResource* const pTemporalCacheValueMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueMap,
		Foundation::Resource::GpuResource* const pTemporalCacheValueSquaredMeanMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueSquaredMeanMap,
		Foundation::Resource::GpuResource* const pTemporalCacheRayHitDistanceMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheRayHitDistanceMap,
		Foundation::Resource::GpuResource* const pTemporalTSPPMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheTSPPMap,
		Value::Type type) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type == Value::Type::E_Contrast ?
		PipelineState::E_TemporalSupersamplingBlendWithCurrentFrame_Contrast:
		PipelineState::E_TemporalSupersamplingBlendWithCurrentFrame_Color].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_TemporalSupersamplingBlendWithCurrentFrame].Get());

		pValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRayHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pTemporalCacheValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalCacheValueMap);

		pTemporalCacheValueSquaredMeanMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalCacheValueSquaredMeanMap);

		pTemporalCacheRayHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalCacheRayHitDistanceMap);

		pTemporalTSPPMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalTSPPMap);

		D3D12_GPU_DESCRIPTOR_HANDLE hCachedValue;
		D3D12_GPU_DESCRIPTOR_HANDLE hCachedSquaredMean;
		if (type == Value::E_Contrast) {
			hCachedValue = mhGpuDecs[Descriptor::CachedValue::ES_Contrast];
			hCachedSquaredMean = mhGpuDecs[Descriptor::CachedSquaredMean::ES_Contrast];
		}
		else {
			hCachedValue = mhGpuDecs[Descriptor::CachedValue::ES_Color];
			hCachedSquaredMean = mhGpuDecs[Descriptor::CachedSquaredMean::ES_Color];
		}
		
		CmdList->SetComputeRootConstantBufferView(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::CB_TSPPBlendWithCurrentFrame, pFrameResource->BlendWithCurrentFrameCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_AOCoefficient, si_valueMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_LocalMeanVaraince, mhGpuDecs[Descriptor::LocalMeanVariance::ES_Raw]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_RayHitDistance, si_rayHitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_CachedValue, hCachedValue);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_CachedSquaredMean, hCachedSquaredMean);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_TSPPSquaredMeanRayHitDistance, mhGpuDecs[Descriptor::ES_TSPPSquaredMeanRayHitDistance]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TemporalAOCoefficient, uo_temporalCacheValueMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TSPP, uo_temporalCacheTSPPMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_AOCoefficientSquaredMean, uo_temporalCacheValueSquaredMeanMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_RayHitDistance, uo_temporalCacheRayHitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_VarianceMap, mhGpuDecs[Descriptor::Variance::EU_Raw]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_BlurStrength, mhGpuDecs[Descriptor::EU_DisocclusionBlurStrength]);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Default::Height),
			ShadingConvention::SVGF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::ApplyAtrousWaveletTransformFilter(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
		Foundation::Resource::GpuResource* const pTemporalCacheHitDistanceMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCachehitDistanceMap,
		Foundation::Resource::GpuResource* const pTemporalCacheTSPPMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCacheTSPPMap,
		Foundation::Resource::GpuResource* const pTemporalValueMap_Input,
		D3D12_GPU_DESCRIPTOR_HANDLE si_temporalValueMap,
		Foundation::Resource::GpuResource* const pTemporalValueMap_Output,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_TemporalValueMap,
		Value::Type type,
		FLOAT rayHitDistToKernelWidthScale,
		FLOAT rayHitDistToKernelSizeScaleExp) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type == Value::Type::E_Contrast ?
		PipelineState::E_EdgeStoppingFilterGaussian3x3_Contrast:
		PipelineState::E_EdgeStoppingFilterGaussian3x3_Color].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_AtrousWaveletTransformFilter].Get());

		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pTemporalCacheHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pTemporalCacheTSPPMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pTemporalValueMap_Input->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pTemporalValueMap_Output->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalValueMap_Output);

		ShadingConvention::SVGF::RootConstant::AtrousWaveletTransformFilter::Struct rc;
		rc.gRayHitDistanceToKernelWidthScale = rayHitDistToKernelWidthScale;
		rc.gRayHitDistanceToKernelSizeScaleExponent = rayHitDistToKernelSizeScaleExp;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::SVGF::RootConstant::AtrousWaveletTransformFilter::Struct>(
			RootSignature::AtrousWaveletTransformFilter::RC_Consts,
			ShadingConvention::SVGF::RootConstant::AtrousWaveletTransformFilter::Count,
			&rc,
			0,
			CmdList,
			TRUE);
		
		CmdList->SetComputeRootConstantBufferView(RootSignature::AtrousWaveletTransformFilter::CB_AtrousFilter, pFrameResource->AtrousWaveletTransformFilterCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::SI_TemporalValue, si_temporalValueMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::SI_NormalDepth, si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::SI_Variance, mhGpuDecs[Descriptor::Variance::ES_Raw]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::SI_HitDistance, si_temporalCachehitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::SI_DepthPartialDerivative, mhGpuDecs[Descriptor::ES_DepthPartialDerivative]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::AtrousWaveletTransformFilter::UO_TemporalValue, uo_TemporalValueMap);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SVGF::ThreadGroup::Atrous::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SVGF::ThreadGroup::Atrous::Height),
			ShadingConvention::SVGF::ThreadGroup::Atrous::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::BlurDisocclusion(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pRoughnessMetalnessMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
		Foundation::Resource::GpuResource* const pTemporalValueMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uio_temporalValueMap,
		Value::Type type,
		UINT numLowTSPPBlurPasses) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type == Value::Type::E_Contrast ?
		PipelineState::E_DisocclusionBlur_Contrast:
		PipelineState::E_DisocclusionBlur_Color].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::E_DisocclusionBlur].Get());

		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pTemporalValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pTemporalValueMap);
		
		CmdList->SetComputeRootDescriptorTable(RootSignature::DisocclusionBlur::SI_DepthMap, si_depthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::DisocclusionBlur::SI_RoughnessMetalnessMap, si_roughnessMetalnessMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::DisocclusionBlur::SI_BlurStrength, mhGpuDecs[Descriptor::ES_DisocclusionBlurStrength]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::DisocclusionBlur::UIO_AOCoefficient, uio_temporalValueMap);

		ShadingConvention::SVGF::RootConstant::DisocclusionBlur::Struct rc;
		rc.gTextureDim.x = mInitData.ClientWidth;
		rc.gTextureDim.y = mInitData.ClientHeight;
		rc.gMaxStep = numLowTSPPBlurPasses;

		const UINT ThreadGroupX = ShadingConvention::SVGF::ThreadGroup::Default::Width;
		const UINT ThreadGroupY = ShadingConvention::SVGF::ThreadGroup::Default::Height;
		UINT filterStep = 1;
		for (UINT i = 0; i < numLowTSPPBlurPasses; ++i) {
			rc.gStep = i;

			Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::SVGF::RootConstant::DisocclusionBlur::Struct>(
				RootSignature::DisocclusionBlur::RC_Consts,
				ShadingConvention::SVGF::RootConstant::DisocclusionBlur::Count,
				&rc,
				0,
				CmdList,
				TRUE);

			// Account for interleaved Group execution
			const UINT WidthCS = filterStep * ThreadGroupX * Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, filterStep * ThreadGroupX);
			const UINT HeightCS = filterStep * ThreadGroupY * Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, filterStep * ThreadGroupY);

			CmdList->Dispatch(
				Foundation::Util::D3D12Util::D3D12Util::CeilDivide(WidthCS, ThreadGroupX),
				Foundation::Util::D3D12Util::D3D12Util::CeilDivide(HeightCS, ThreadGroupY),
				ShadingConvention::SVGF::ThreadGroup::Default::Depth);

			filterStep *= 2;
		}
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SVGF::SVGFClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// LocalMeanVarianceMap
	{
		texDesc.Format = ShadingConvention::SVGF::LocalMeanVarianceMapFormat;

		CheckReturn(mpLogFile, mResources[Resource::LocalMeanVariance::E_Raw]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_LocalMeanVarianceMap_Raw"));
		CheckReturn(mpLogFile, mResources[Resource::LocalMeanVariance::E_Smoothed]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_LocalMeanVarianceMap_Smoothed"));
	}
	// VarianceMap
	{
		texDesc.Format = ShadingConvention::SVGF::VarianceMapFormat;

		CheckReturn(mpLogFile, mResources[Resource::Variance::E_Raw]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_VarianceMap_Raw"));
		CheckReturn(mpLogFile, mResources[Resource::Variance::E_Smoothed]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_VarianceMap_Smoothed"));
	}
	// ValueMap
	{
		// Contrast
		{
			texDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Contrast;

			CheckReturn(mpLogFile, mResources[Resource::CachedValue::E_Contrast]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SVGF_CachedValueMap_Contrast"));
		}
		// Color
		{
			texDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Color;

			CheckReturn(mpLogFile, mResources[Resource::CachedValue::E_Color]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SVGF_CachedValueMap_Color"));
		}
	}
	// CachedValueSquaredMeanMap
	{
		// Contrast
		{
			texDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast;

			CheckReturn(mpLogFile, mResources[Resource::CachedSquaredMean::E_Contrast]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SVGF_CachedValueSquaredMeanMap_Contrast"));
		}
		// Color
		{
			texDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color;

			CheckReturn(mpLogFile, mResources[Resource::CachedSquaredMean::E_Color]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SVGF_CachedValueSquaredMeanMap_Color"));
		}
	}
	// DepthPartialDerivativeMap
	{
		texDesc.Format = ShadingConvention::SVGF::DepthPartialDerivativeMapFormat;

		CheckReturn(mpLogFile, mResources[Resource::E_DepthPartialDerivative]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_DepthPartialDerivativeMap"));
	}
	// DisocclusionBlurStrengthMap
	{
		texDesc.Format = ShadingConvention::SVGF::DisocclusionBlurStrengthMapFormat;

		CheckReturn(mpLogFile, mResources[Resource::E_DisocclusionBlurStrength]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_DisocclusionBlurStrengthMap"));
	}
	// TSPPSquaredMeanRayHitDistanceMap
	{
		texDesc.Format = ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat;

		CheckReturn(mpLogFile, mResources[Resource::E_TSPPSquaredMeanRayHitDistance]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_TSPPSquaredMeanRayHitDistanceMap"));
	}
	// DebugMap
	{
		texDesc.Format = ShadingConvention::SVGF::DebugMapFormat;

		for (UINT i = 0; i < 2; ++i) {
			std::wstringstream wsstream(L"SVGF_DebugMap_");

			wsstream << i;

			CheckReturn(mpLogFile, mDebugMaps[i]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				wsstream.str().c_str()));
		}
	}

	return TRUE;
}

BOOL SVGF::SVGFClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	// LocalMeanVarianceMap
	{
		srvDesc.Format = ShadingConvention::SVGF::LocalMeanVarianceMapFormat;
		uavDesc.Format = ShadingConvention::SVGF::LocalMeanVarianceMapFormat;

		// Raw
		{
			const auto resource = mResources[Resource::LocalMeanVariance::E_Raw]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::LocalMeanVariance::ES_Raw]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::LocalMeanVariance::EU_Raw]);
		}
		// Smoothed
		{
			const auto resource = mResources[Resource::LocalMeanVariance::E_Smoothed]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::LocalMeanVariance::ES_Smoothed]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::LocalMeanVariance::EU_Smoothed]);
		}
	}
	// VarianceMap
	{
		srvDesc.Format = ShadingConvention::SVGF::VarianceMapFormat;
		uavDesc.Format = ShadingConvention::SVGF::VarianceMapFormat;

		// Raw
		{
			const auto resource = mResources[Resource::Variance::E_Raw]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::Variance::ES_Raw]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::Variance::EU_Raw]);
		}
		// Smoothed
		{
			const auto resource = mResources[Resource::Variance::E_Smoothed]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::Variance::ES_Smoothed]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::Variance::EU_Smoothed]);
		}
	}
	// ValueMap
	{
		// Contrast
		{
			srvDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Contrast;
			uavDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Contrast;

			const auto resource = mResources[Resource::CachedValue::E_Contrast]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::CachedValue::ES_Contrast]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::CachedValue::EU_Contrast]);
		}
		// Color
		{
			srvDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Color;
			uavDesc.Format = ShadingConvention::SVGF::ValueMapFormat_Color;

			const auto resource = mResources[Resource::CachedValue::E_Color]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::CachedValue::ES_Color]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::CachedValue::EU_Color]);
		}
	}
	// ValueSquaredMeanMap
	{
		// Contrast
		{
			srvDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast;
			uavDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Contrast;

			const auto resource = mResources[Resource::CachedSquaredMean::E_Contrast]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::CachedSquaredMean::ES_Contrast]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::CachedSquaredMean::EU_Contrast]);
		}
		// Color
		{
			srvDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color;
			uavDesc.Format = ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color;

			const auto resource = mResources[Resource::CachedSquaredMean::E_Color]->Resource();
			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::CachedSquaredMean::ES_Color]);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::CachedSquaredMean::EU_Color]);
		}
	}
	// DepthPartialDerivativeMap
	{
		srvDesc.Format = ShadingConvention::SVGF::DepthPartialDerivativeMapFormat;
		uavDesc.Format = ShadingConvention::SVGF::DepthPartialDerivativeMapFormat;

		const auto resource = mResources[Resource::E_DepthPartialDerivative]->Resource();
		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::ES_DepthPartialDerivative]);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::EU_DepthPartialDerivative]);
	}
	// DisocclusionBlurStrengthMap
	{
		srvDesc.Format = ShadingConvention::SVGF::DisocclusionBlurStrengthMapFormat;
		uavDesc.Format = ShadingConvention::SVGF::DisocclusionBlurStrengthMapFormat;

		const auto resource = mResources[Resource::E_DisocclusionBlurStrength]->Resource();
		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::ES_DisocclusionBlurStrength]);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::EU_DisocclusionBlurStrength]);
	}
	// TSPPSquaredMeanRayHitDistanceMap
	{
		srvDesc.Format = ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat;
		uavDesc.Format = ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat;

		const auto resource = mResources[Resource::E_TSPPSquaredMeanRayHitDistance]->Resource();
		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhCpuDecs[Descriptor::ES_TSPPSquaredMeanRayHitDistance]);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhCpuDecs[Descriptor::EU_TSPPSquaredMeanRayHitDistance]);
	}
	// DebugMap
	{
		uavDesc.Format = ShadingConvention::SVGF::DebugMapFormat;

		for (UINT i = 0; i < 2; ++i) {
			const auto resource = mDebugMaps[i]->Resource();
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhDebugMapCpuUavs[i]);
		}
	}

	return TRUE;
}

