#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Common/Debug/Logger.hpp"

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

BOOL CommandObject::Initialize(Common::Debug::LogFile* const pLogFile, ID3D12Device5* const pDevice, UINT numThreads) {
	mpLogFile = pLogFile;
	md3dDevice = pDevice;

	mMultiCommandLists.resize(numThreads);

#ifdef _DEBUG
	CheckReturn(mpLogFile, CreateDebugObjects());
#endif
	CheckReturn(mpLogFile, CreateCommandQueue());
	CheckReturn(mpLogFile, CreateDirectCommandObjects());
	CheckReturn(mpLogFile, CreateMultiCommandObjects(numThreads));
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

BOOL CommandObject::ResetDirectCommandList() {
	CheckHRESULT(mpLogFile, mDirectCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	return TRUE;
}

BOOL CommandObject::CreateDebugObjects() {
	CheckHRESULT(mpLogFile, md3dDevice->QueryInterface(IID_PPV_ARGS(&mInfoQueue)));
	CheckHRESULT(mpLogFile, mInfoQueue->RegisterMessageCallback(D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, mpLogFile, &mCallbakCookie));

	return TRUE;
}

BOOL CommandObject::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	return TRUE;
}

BOOL CommandObject::CreateDirectCommandObjects() {
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),	// Associated command allocator
		nullptr,					// Initial PipelineStateObject
		IID_PPV_ARGS(mDirectCommandList.GetAddressOf())
	));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mDirectCommandList->Close();

	return TRUE;
}

BOOL CommandObject::CreateMultiCommandObjects(UINT numThreads) {
	for (UINT i = 0; i < numThreads; ++i) {
		CheckHRESULT(mpLogFile, md3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mDirectCmdListAlloc.Get(),
			nullptr,
			IID_PPV_ARGS(mMultiCommandLists[i].GetAddressOf())
		));

		mMultiCommandLists[i]->Close();
	}

	return TRUE;
}

BOOL CommandObject::CreateFence() {
	CheckHRESULT(mpLogFile, md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	return TRUE;
}