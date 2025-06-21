#ifndef __D3D12UTIL_INL__
#define __D3D12UTIL_INL__

UINT Render::DX::Foundation::Util::D3D12Util::CeilDivide(UINT value, UINT divisor) {
	return (value + divisor - 1) / divisor;
}

template <typename T>
void Render::DX::Foundation::Util::D3D12Util::SetRoot32BitConstants(
	UINT RootParameterIndex,
	UINT Num32BitValuesToSet,
	const void* pSrcData,
	UINT DestOffsetIn32BitValues,
	ID3D12GraphicsCommandList6* const pCmdList,
	BOOL isCompute) {
	std::vector<UINT> consts(Num32BitValuesToSet);
	std::memcpy(consts.data(), pSrcData, sizeof(T));

	if (isCompute) pCmdList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, consts.data(), DestOffsetIn32BitValues);
	else pCmdList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, consts.data(), DestOffsetIn32BitValues);
}

#endif // __D3D12UTIL_INL__