#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Util/ShaderTable.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Shading::Util;

GpuUploadBuffer::~GpuUploadBuffer() {
	if (mResource.Get()) mResource->Unmap(0, nullptr);
}

GpuUploadBuffer::GpuUploadBuffer(Common::Debug::LogFile* const pLogFile) 
	: mpLogFile(pLogFile) {}

BOOL GpuUploadBuffer::Allocate(Foundation::Core::Device* const pDevice, UINT bufferSize, LPCWSTR resourceName) {
	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateUploadBuffer(pDevice, bufferSize, IID_PPV_ARGS(&mResource)));	
	//mResource->SetName(resourceName);

	return TRUE;
}

BOOL GpuUploadBuffer::MapCpuWriteOnly(std::uint8_t*& pData) {
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	// We do not intend to read from this resource on the CPU.
	CheckHRESULT(mpLogFile, mResource->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&pData)));

	return TRUE;
}

ShaderRecord::ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize) 
	: mShaderIdentifier(pShaderIdentifier, shaderIdentifierSize) {}

ShaderRecord::ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize, void* const pLocalRootArguments, UINT localRootArgumentsSize) 
	: mShaderIdentifier(pShaderIdentifier, shaderIdentifierSize), mLocalRootArguments(pLocalRootArguments, localRootArgumentsSize) {}

void ShaderRecord::CopyTo(void* const dest) const {
	uint8_t* byteDest = static_cast<uint8_t*>(dest);

	std::memcpy(byteDest, mShaderIdentifier.Ptr, mShaderIdentifier.Size);
	if (mLocalRootArguments.Ptr) std::memcpy(byteDest + mShaderIdentifier.Size, mLocalRootArguments.Ptr, mLocalRootArguments.Size);
}

ShaderRecord::PointerWithSize::PointerWithSize() 
	: Ptr(nullptr), Size(0) {}

ShaderRecord::PointerWithSize::PointerWithSize(void* const ptr, UINT size) 
	: Ptr(ptr), Size(size) {}

ShaderTable::ShaderTable(Common::Debug::LogFile* const pLogFile, Foundation::Core::Device* const pDevice, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName)
	: GpuUploadBuffer(pLogFile), mpDevice(pDevice) {
	mName = resourceName != nullptr ? std::wstring(resourceName) : L"";

	mShaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	mShaderRecords.reserve(numShaderRecords);

	mBufferSize = numShaderRecords * mShaderRecordSize;
}

BOOL ShaderTable::Initialze() {
	CheckReturn(mpLogFile, Allocate(mpDevice, mBufferSize, mName.length() > 0 ? mName.c_str() : nullptr));
	CheckReturn(mpLogFile, MapCpuWriteOnly(mMappedShaderRecords));

	return TRUE;
}

BOOL ShaderTable::push_back(const ShaderRecord& shaderRecord) {
	CheckReturn(mpLogFile, mShaderRecords.size() < mShaderRecords.capacity());

	mShaderRecords.push_back(shaderRecord);

	shaderRecord.CopyTo(mMappedShaderRecords);

	mMappedShaderRecords += mShaderRecordSize;

	return TRUE;
}

void ShaderTable::DebugPrint(std::unordered_map<void*, std::wstring>& shaderIdToStringMap) {
	std::wstringstream wsstream;
	wsstream << L"|--------------------------------------------------------------------" << std::endl;
	wsstream << L"|Shader table - " << mName << L": " << mShaderRecordSize << L" | " << mShaderRecords.size() * mShaderRecordSize << L" bytes" << std::endl;
	for (UINT i = 0; i < mShaderRecords.size(); ++i) {
		wsstream << L"| [" << i << L"]: " << shaderIdToStringMap[mShaderRecords[i].mShaderIdentifier.Ptr] << L", ";
		wsstream << mShaderRecords[i].mShaderIdentifier.Size << L" + " << mShaderRecords[i].mLocalRootArguments.Size << L" bytes" << std::endl;
	}
	wsstream << L"|--------------------------------------------------------------------";
	WLogln(mpLogFile, wsstream.str());
}