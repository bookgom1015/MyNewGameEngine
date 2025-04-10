#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;

namespace {
	void D3D12MessageCallback(
			D3D12_MESSAGE_CATEGORY category,
			D3D12_MESSAGE_SEVERITY severity,
			D3D12_MESSAGE_ID id,
			LPCSTR pDescription,
			void* pContext) {
		std::string str(pDescription);

		std::string sevStr;
		switch (severity) {
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			sevStr = "Corruption";
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
			sevStr = "Error";
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			sevStr = "Warning";
			break;
		case D3D12_MESSAGE_SEVERITY_INFO:
			return;
			sevStr = "Info";
			break;
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
			sevStr = "Message";
			break;
		}

		std::stringstream sstream;
		sstream << '[' << sevStr << "] " << pDescription;

		const auto pLogFile = reinterpret_cast<Common::Debug::LogFile*>(pContext);
		Logln(pLogFile, sstream.str());
	}
}

BOOL CommandObject::Initialize(Common::Debug::LogFile* const pLogFile, Device* const pDevice, UINT numThreads) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;
	mThreadCount = numThreads;

	mMultiCommandLists.resize(mThreadCount);

#ifdef _DEBUG
	CheckReturn(mpLogFile, CreateDebugObjects());
#endif
	CheckReturn(mpLogFile, CreateCommandQueue());
	CheckReturn(mpLogFile, CreateDirectCommandObjects());
	CheckReturn(mpLogFile, CreateMultiCommandObjects(mThreadCount));
	CheckReturn(mpLogFile, CreateFence());

	return TRUE;
}

BOOL CommandObject::FlushCommandQueue() {
	// Advance the fence value to mark commands up to this fence point.
	++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	CheckHRESULT(mpLogFile, mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has compledted commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		// Fire event when GPU hits current fence.
		CheckHRESULT(mpLogFile, mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	return TRUE;
}

BOOL CommandObject::ExecuteDirectCommandList() {
	const auto cmdList = mDirectCommandList.Get();
	CheckHRESULT(mpLogFile, cmdList->Close());
	mCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&cmdList));

	return TRUE;
}

BOOL CommandObject::ResetDirectCommandList(ID3D12PipelineState* const pPipelineState) {
	CheckHRESULT(mpLogFile, mDirectCommandList->Reset(mDirectCmdListAlloc.Get(), pPipelineState));

	return TRUE;
}

BOOL CommandObject::ExecuteCommandList(UINT index) {
	const auto cmdList = mMultiCommandLists[index].Get();
	CheckHRESULT(mpLogFile, cmdList->Close());
	mCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&cmdList));

	return TRUE;
}

BOOL CommandObject::ResetCommandList(ID3D12CommandAllocator* const pAlloc, UINT index, ID3D12PipelineState* const pPipelineState) {
	CheckHRESULT(mpLogFile, mMultiCommandLists[index]->Reset(pAlloc, pPipelineState));

	return TRUE;
}

BOOL CommandObject::ExecuteCommandLists() {
	std::vector<ID3D12CommandList*> cmdLists(mThreadCount);

	for (UINT i = 0; i < mThreadCount; ++i) {
		const auto cmdList = mMultiCommandLists[i].Get();
		CheckHRESULT(mpLogFile, cmdList->Close());

		cmdLists.push_back(cmdList);
	}
	
	mCommandQueue->ExecuteCommandLists(mThreadCount, cmdLists.data());

	return TRUE;
}

BOOL CommandObject::ResetCommandLists(ID3D12CommandAllocator* const allocs[], ID3D12PipelineState* const pPipelineState) {
	for (UINT i = 0; i < mThreadCount; ++i) 
		CheckHRESULT(mpLogFile, mMultiCommandLists[i]->Reset(allocs[i], pPipelineState));

	return TRUE;
}

BOOL CommandObject::WaitCompletion(UINT64 fence) {
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (fence != 0 && mFence->GetCompletedValue() < fence) {
		const HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHRESULT(mpLogFile, mFence->SetEventOnCompletion(fence, eventHandle));

		const auto status = WaitForSingleObject(eventHandle, INFINITE);
		if (status == WAIT_FAILED) ReturnFalse(mpLogFile, L"Calling \'WaitForSingleObject\' failed");

		if (!CloseHandle(eventHandle)) ReturnFalse(mpLogFile, L"Failed to close handle");
	}

	return TRUE;
}

UINT64 CommandObject::IncreaseFence() {
	return ++mCurrentFence;
}

BOOL CommandObject::Signal() {
	CheckHRESULT(mpLogFile, mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	return TRUE;
}

#ifdef _DEBUG
BOOL CommandObject::CreateDebugObjects() {
	CheckReturn(mpLogFile, mpDevice->QueryInterface(mInfoQueue));
	CheckHRESULT(mpLogFile, mInfoQueue->RegisterMessageCallback(D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, mpLogFile, &mCallbakCookie));

	return TRUE;
}
#endif

BOOL CommandObject::CreateCommandQueue() {
	CheckReturn(mpLogFile, mpDevice->CreateCommandQueue(mCommandQueue));

	return TRUE;
}

BOOL CommandObject::CreateDirectCommandObjects() {
	CheckReturn(mpLogFile, mpDevice->CreateCommandAllocator(mDirectCmdListAlloc));
	CheckReturn(mpLogFile, mpDevice->CreateCommandList(mDirectCmdListAlloc.Get(), mDirectCommandList));

	return TRUE;
}

BOOL CommandObject::CreateMultiCommandObjects(UINT numThreads) {
	for (UINT i = 0; i < numThreads; ++i) 
		CheckReturn(mpLogFile, mpDevice->CreateCommandList(mDirectCmdListAlloc.Get(), mMultiCommandLists[i]));

	return TRUE;
}

BOOL CommandObject::CreateFence() {
	CheckReturn(mpLogFile, mpDevice->CreateFence(mFence));

	return TRUE;
}