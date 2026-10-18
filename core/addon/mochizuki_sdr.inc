// mochizuki.inc's 8-bit leg, included by it after MzSendControls.

// netColour is a float texture, which on Encoding 0 holds sRGB values: the RenoDX stage reads a float
// frame as HDR whatever it is told, and an SDR game's frame takes its SDR codec. So on Encoding 0 the
// frame goes to the runtime as 8-bit sRGB, as a game's own SDR frame does in the OptiScaler fork, and
// the answer comes back through the same shader. Slots 48 and 52 (optical_flow.inc's table range).
inline constexpr char kMzConvertShader[] = R"(
Texture2D<float4> src : register(t0);
RWTexture2D<float4> dst : register(u0);
cbuffer C : register(b0) { uint w; uint h; };
[numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID) {
 if (p.x >= w || p.y >= h) return;
 dst[p.xy] = float4(saturate(src.Load(int3(p.xy,0)).rgb), 1.0);
})";

void DispatchOn(ID3D12GraphicsCommandList* cmd, ID3D12PipelineState* pso, UINT base,
                ID3D12Resource* out, const UINT* k, UINT count, UINT dw, UINT dh);
void ViewAt(UINT i, ID3D12Resource* res, DXGI_FORMAT fmt, bool uav);

void MzConvert(ID3D12GraphicsCommandList* cmd, UINT base, ID3D12Resource* from,
               ID3D12Resource* to) {
    const auto typed = [](ID3D12Resource* r) {
        const DXGI_FORMAT f = r->GetDesc().Format;
        return f == DXGI_FORMAT_R8G8B8A8_TYPELESS ? DXGI_FORMAT_R8G8B8A8_UNORM : f;
    };
    ViewAt(base, from, typed(from), false);
    ViewAt(base + 3, to, typed(to), true);
    const UINT k[2]{g.netWidth, g.netHeight};
    DispatchOn(cmd, mz.convert.Get(), base, to, k, 2, g.netWidth, g.netHeight);
}

// The 8-bit copy of netColour, made at its size; the one a resize replaces is parked, as netColour is.
ID3D12Resource* MzToSdr(ID3D12GraphicsCommandList* cmd) {
    if (mz.convert == nullptr &&
        !CompileShader(kMzConvertShader, sizeof(kMzConvertShader), "mochizuki convert", mz.convert))
        return nullptr;
    if (mz.sdr == nullptr || mz.sdr->GetDesc().Width != g.netWidth ||
        mz.sdr->GetDesc().Height != g.netHeight) {
        if (mz.sdr != nullptr)
            g.parked.push_back(mz.sdr);
        if (!CreateTexture(g.netWidth, g.netHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mz.sdr,
                           "mochizuki sdr"))
            return nullptr;
    }
    MzConvert(cmd, 48, g.netColour.Get(), mz.sdr.Get());
    return mz.sdr.Get();
}
