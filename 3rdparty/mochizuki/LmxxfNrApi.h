#pragma once

/* Versioned C ABI for LmxxfNrRuntime.dll.
 * MSVC host and MinGW runtime must not share a C++ ABI. No STL, exceptions, or
 * CRT-allocated objects cross this boundary. x64 stdcall is the Windows default. */

#include <stdint.h>
#include <stddef.h> /* wchar_t in C hosts */

#ifdef __cplusplus
extern "C"
{
#endif

#define LMXXF_NR_ABI_VERSION 1u

/* Optional recovery when HIP enqueue or the session queue contract fails.
 * On recovery, EnqueueHip returns OK only after the private neural output was fully zeroed;
 * GetLastError then contains a recovery diagnostic. The caller must submit its
 * consumer before Retire, Drain, or Destroy. The runtime drains a changed
 * consumer queue before output reuse or destruction. A failed/uncertain clear
 * returns FAILED and the session must be rebuilt. */
#define LMXXF_NR_CREATE_FLAG_ZERO_OUTPUT_FALLBACK (1u << 0)
/* mochizuki only (lmxxf rejects it): run the network as ACO's machine code from dlssnr-amd\aco\ (upstream's
   [Network] ACO Mode), where the driver offers VK_KHR_pipeline_binary; else, or without that folder, as before.
   Decided once a process, by the first session that starts. */
#define LMXXF_NR_CREATE_FLAG_ACO (1u << 1)

    enum LmxxfNrStatus
    {
        LMXXF_NR_OK = 0,
        LMXXF_NR_UNSUPPORTED_ABI = 1,
        LMXXF_NR_INVALID_ARGUMENT = 2,
        LMXXF_NR_NOT_IMPLEMENTED = 3,
        LMXXF_NR_UNAVAILABLE = 4,
        LMXXF_NR_FAILED = 5
    };

    enum LmxxfNrJobState
    {
        LMXXF_NR_JOB_NONE = 0,
        LMXXF_NR_JOB_PREPARED = 1,           /* Set by PrepareFrame; ready for RecordInputs */
        LMXXF_NR_JOB_PRODUCER_SUBMITTED = 2, /* Set by RecordInputs; producer recorded/submitted */
        LMXXF_NR_JOB_NR_ENQUEUED = 3,        /* EnqueueHip scheduled on queue */
        LMXXF_NR_JOB_NR_COMPLETE = 4,        /* EnqueueHip executed or completed */
        LMXXF_NR_JOB_CONSUMER_COMPLETE = 5,  /* Set by RecordOutputs; consumer recorded */
        LMXXF_NR_JOB_RETIRED = 6             /* Set by Retire or CancelUnsubmitted */
    };

    typedef struct LmxxfNrCapabilities
    {
        uint32_t struct_size;
        uint32_t abi_version;
        uint32_t max_input_width;
        uint32_t max_input_height;
        uint32_t history_supported; /* first product version: 0 */
        uint32_t overlap_supported; /* first product version: 0 */
        uint32_t graph_supported;   /* first product version: 0; EnqueueHip must not graph-wait */
        uint32_t hip_ready;         /* 1 once host+hsaco are loaded */
        uint32_t gfx1201_target;    /* 1 = this binary is for gfx1201 */
    } LmxxfNrCapabilities;

    typedef struct LmxxfNrCreateInfo
    {
        uint32_t struct_size;
        void* device; /* ID3D12Device*; not dereferenced until HIP is wired */
        void* queue;  /* ID3D12CommandQueue*; must match device when HIP is wired */
        const wchar_t* assets_directory;
        uint32_t flags; /* LMXXF_NR_CREATE_FLAG_*; unknown bits are rejected */
    } LmxxfNrCreateInfo;

#define LMXXF_NR_FRAME_FLAG_STRENGTH (1u << 0)
#define LMXXF_NR_FRAME_FLAG_DEBUG_VIEW (1u << 1)
#define LMXXF_NR_FRAME_FLAG_CODEC_PASSTHROUGH (1u << 2)
/* Each pass reads its own output from the previous frame, warped by the motion fields below. */
#define LMXXF_NR_FRAME_FLAG_TEMPORAL (1u << 3)
/* With TEMPORAL: the smoothing below acts on the network's change to the frame, not on its whole output. */
#define LMXXF_NR_FRAME_FLAG_SMOOTH_RESIDUAL (1u << 4)
/* The model's NGX Style is in `style`. Without it lmxxf keeps Style 1, the one its kernels have built in. */
#define LMXXF_NR_FRAME_FLAG_STYLE (1u << 5)
/* Reuse the ViT part of the network where the picture has not changed (upstream's DLSS5_VIT_ADAPTIVE=1): faster on a
   still scene, recomputed in motion, slightly lossy. One pass only; with more it stays off. */
#define LMXXF_NR_FRAME_FLAG_VIT_REUSE (1u << 6)
/* The 1080 tier on 1920x1088 instead of NVIDIA's 1920x1152 (DLSS5_NETWORK_1080_ROWS=1088): about 4% faster, slightly
   lossy at the bottom rows. A change rebuilds the network. */
#define LMXXF_NR_FRAME_FLAG_COMPACT_1080 (1u << 7)
/* mochizuki only: the int4 mixed network (upstream's [Int4Mixed]), faster, the picture slightly different. Needs the
   session's ACO and dlssnr-amd\int4\ with dlssnr-int4.bin; else the default network, and the log says why. A change
   rebuilds the network, the frames keeping the current one meanwhile while VRAM allows. */
#define LMXXF_NR_FRAME_FLAG_INT4 (1u << 8)
/* lmxxf, three passes: run two and extrapolate the third from them (upstream's DLSS5_MULTI_PASS_PREDICT): about a
   third less time, close to a real third pass. */
#define LMXXF_NR_FRAME_FLAG_PASS3_ESTIMATE (1u << 9)
/* lmxxf, two or three passes: where the frame looks like skin, the result goes back toward pass 1's (upstream's
   DLSS5_MULTI_PASS_SKIN_PROTECT), so faces do not take the extra passes. */
#define LMXXF_NR_FRAME_FLAG_SKIN_PROTECT (1u << 10)
/* mochizuki, two or three passes: a later pass sees the first pass's input with only the previous pass's local
   structure (upstream's cascade_detail_only), so passes add detail and do not stack the network's tone and colour
   shift. A change rebuilds the network. */
#define LMXXF_NR_FRAME_FLAG_CASCADE_DETAIL (1u << 11)
/* mochizuki, two or three passes: after the last, each pixel's luminance is held inside the range the first pass's
   answer has around it, so thin edges do not gain a rim that grows every pass. Per frame, no rebuild. */
#define LMXXF_NR_FRAME_FLAG_ANTI_HALO (1u << 12)
/* mochizuki: the 310.8 model (dlssnr-amd\dlssnr.bin) instead of 310.9's new one (dlssnr-small.bin, about a third
   faster, the default when it is there). int4 mixed exists for the 310.8 model only. A change rebuilds the network. */
#define LMXXF_NR_FRAME_FLAG_ORIGINAL_MODEL (1u << 13)
/* Two or more passes: the first pass's answer as well, in LmxxfNrJob::first_pass_output, for person protection. A
   texture of the frame's size, readable after RecordOutputs as private_output is. Needs the job's full size; a runtime
   that has none this frame leaves it null. */
#define LMXXF_NR_FRAME_FLAG_FIRST_PASS (1u << 14)
/* lmxxf: the codec's white point measured from the frame's mean luminance (auto_white) instead of a fixed 1.0, on the
   encode and every decode, for games that hand very dark linear colour. Per frame, no rebuild. */
#define LMXXF_NR_FRAME_FLAG_AUTO_WHITE (1u << 15)

    typedef struct LmxxfNrFrameInfo
    {
        uint32_t struct_size;
        uint64_t session_id;
        uint64_t frame_id;
        uint64_t list_generation;
        void* command_list; /* ID3D12GraphicsCommandList*; Record* do not Execute */
        uint32_t color_width;
        uint32_t color_height;
        void* color;             /* ID3D12Resource*; required for RecordInputs */
        uint32_t color_state;    /* D3D12_RESOURCE_STATES at RecordInputs */
        uint32_t flags;          /* LMXXF_NR_FRAME_FLAG_* (0 in legacy ABI v1) */
        float transfer_strength; /* Detail strength: 0..2, default 1.0; above 1 extrapolates */
        float color_strength;    /* Colour strength: lmxxf 0..2, mochizuki 0..4, default 1.0; above 1 extrapolates */
        uint32_t debug_view;     /* 0=normal, 1=proxy, 2=neural solo, 3=diff 20x, 4=tint */
        float model_scale;       /* 0.25..1.0, default 1.0 */
        uint32_t passes;         /* network runs per frame, each on the previous output: 1..3 (mochizuki 1..30), 0 = 1 */
        void* motion;            /* ID3D12Resource*, the game's motion vectors; required for TEMPORAL */
        uint32_t motion_state;   /* D3D12_RESOURCE_STATES at RecordInputs */
        uint32_t motion_width;   /* extent the vectors cover; 0 = color_width x color_height */
        uint32_t motion_height;
        float motion_scale_x; /* value * scale = pixels of that extent (the NGX and FFX convention) */
        float motion_scale_y;
        uint32_t reset; /* nonzero drops every pass's history this frame */
        /* TEMPORAL: where the last pass's output differs from its warped history by less than the threshold
           (colour units, 0..1), it is pulled toward the history, by the strength at no difference. 0 = off.
           SMOOTH_RESIDUAL applies the same rule to the output minus the frame's input, against that difference
           from the previous frame, and adds the result back to the input. */
        float smooth_threshold;
        float smooth_strength;
        /* With STYLE: NGX Style 0 Standard (NVIDIA's default), 1 Natural, 2 Cinematic. It is written into the
           kernels as they load, so a change rebuilds the network (a hitch of a second or two). */
        uint32_t style;
        /* mochizuki, TEMPORAL: the game's depth at the colour's extent. The history reads the motion vector at the
           nearest of five depth taps, so a silhouette takes the foreground's vector. Null: read at the pixel. A host
           talking to lmxxf sends struct_size without these (offsetof depth), as lmxxf takes only the sizes it knows. */
        void* depth;             /* ID3D12Resource*: D32, D32S8, R32 or D16 formats; others are left unread */
        uint32_t depth_state;    /* D3D12_RESOURCE_STATES at RecordInputs */
        uint32_t depth_inverted; /* nonzero: 1 is near (NGX DepthInverted) */
    } LmxxfNrFrameInfo;

    typedef struct LmxxfNrJob
    {
        uint32_t struct_size;
        void* handle;
        void* private_output; /* ID3D12Resource* for SR; null until PrepareFrame succeeds */
        /* ID3D12Resource*, with LMXXF_NR_FRAME_FLAG_FIRST_PASS. A host without it sends the struct_size up to here. */
        void* first_pass_output;
    } LmxxfNrJob;

    typedef struct LmxxfNrApi
    {
        uint32_t struct_size;
        uint32_t abi_version;
        int32_t (*QueryCapabilities)(LmxxfNrCapabilities* out);
        int32_t (*Create)(const LmxxfNrCreateInfo* info, void** context);
        int32_t (*Destroy)(void* context);
        int32_t (*PrepareSession)(void* context);
        int32_t (*PrepareFrame)(void* context, const LmxxfNrFrameInfo* info, LmxxfNrJob* job);
        int32_t (*RecordInputs)(void* context, void* job, void* command_list);
        int32_t (*EnqueueHip)(void* context, void* job, void* command_queue);
        int32_t (*RecordOutputs)(void* context, void* job, void* command_list);
        int32_t (*ExecuteAfterProducer)(void* context, void* job, void* command_queue);
        int32_t (*CancelUnsubmitted)(void* context, void* job);
        int32_t (*Poll)(void* context, void* job, uint32_t* state);
        int32_t (*Retire)(void* context, void* job);
        int32_t (*ResetHistory)(void* context);
        int32_t (*Drain)(void* context);
        int32_t (*GetStatus)(void* context, char* buf, uint32_t buf_chars);
        int32_t (*GetLastError)(char* buf, uint32_t buf_chars);
    } LmxxfNrApi;

#ifdef _WIN32
#ifdef LMXXF_NR_RUNTIME_EXPORTS
#define LMXXF_NR_EXPORT __declspec(dllexport)
#else
#define LMXXF_NR_EXPORT __declspec(dllimport)
#endif
#else
#define LMXXF_NR_EXPORT
#endif

    /* Sole export. Caller sets out->struct_size = sizeof(LmxxfNrApi) before the call. */
    LMXXF_NR_EXPORT int32_t LmxxfNrGetApi(uint32_t abi_version, LmxxfNrApi* out);

#ifdef __cplusplus
}
#endif
