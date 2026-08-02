#include "hw_encoder_capabilities.h"

#include <cstring>

extern "C" {
#include <libavutil/hwcontext.h>

#if HAVE_VA_LIBRARY
#include <libavutil/hwcontext_vaapi.h>
#include <va/va.h>
#endif
}

#if HAVE_VA_LIBRARY
static const AVVAAPIDeviceContext* GetVaapiContext(const AVBufferRef* hwDeviceCtx) {
    if (!hwDeviceCtx) return nullptr;
    auto* hwCtx = reinterpret_cast<const AVHWDeviceContext*>(hwDeviceCtx->data);
    if (!hwCtx || hwCtx->type != AV_HWDEVICE_TYPE_VAAPI) return nullptr;
    return static_cast<const AVVAAPIDeviceContext*>(hwCtx->hwctx);
}

static bool IsMesaRadeonSi(const AVBufferRef* hwDeviceCtx) {
    const AVVAAPIDeviceContext* vaapi = GetVaapiContext(hwDeviceCtx);
    if (!vaapi || !vaapi->display) return false;

    const char* vendor = vaQueryVendorString(vaapi->display);
    if (!vendor) return false;

    return std::strstr(vendor, "radeonsi") != nullptr;
}

static uint32_t GetVaapiQualityLevelCount(const AVBufferRef* hwDeviceCtx, const VAProfile profile) {
    const AVVAAPIDeviceContext* vaapi = GetVaapiContext(hwDeviceCtx);
    if (!vaapi || !vaapi->display || !profile) return 0;

    VAConfigAttrib attr{};
    attr.type = VAConfigAttribEncQualityRange;

    if (const VAStatus status = vaGetConfigAttributes(vaapi->display, profile, VAEntrypointEncSlice, &attr, 1);
        status != VA_STATUS_SUCCESS)
        return 0;

    if (attr.value == VA_ATTRIB_NOT_SUPPORTED) return 0;

    return attr.value;
}
#endif

void InitializeVaapiPresets(EncoderInfo& encoderInfo) {
#if HAVE_VA_LIBRARY
    AVBufferRef* hwDeviceCtx = nullptr;
    if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0) < 0) goto end;

    if (IsMesaRadeonSi(hwDeviceCtx)) {
        encoderInfo.presets = {{0, "Speed"}, {1, "Balanced"}, {2, "Quality"}};
        encoderInfo.defaultPreset = 1;
        encoderInfo.isVaapiRadeonSi = true;
    } else {
        VAProfile profile;

        switch (encoderInfo.fourCC) {
            case 'avc1':
                profile = VAProfileH264Main;
                break;
            case 'hvc1':
                profile = VAProfileHEVCMain;
                break;
            case 'av01':
                profile = VAProfileAV1Profile0;
                break;
            default:
                goto end;
        }

        const uint32_t qualityLevels = GetVaapiQualityLevelCount(hwDeviceCtx, profile);
        for (uint32_t i = 1; i <= qualityLevels; ++i) {
            encoderInfo.presets.emplace(i, std::to_string(i));
        }
    }

end:
    av_buffer_unref(&hwDeviceCtx);
#endif
}
