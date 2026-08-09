#pragma once
#include "encoder_info.h"

using namespace IOPlugin;

void InitializeVaapiPresets(EncoderInfo& encoderInfo);
void InitializeVulkanPresets(EncoderInfo& encoderInfo);

bool IsVulkanLoaderSupported(const AVBufferRef* hwDeviceCtx);
