/*
 * Copyright (C) 2019 Samsung Electronics Co.,LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <log/log.h>

#include "ThumbnailScaler.h"
#include "LibScalerForJpeg.h"
#include "GiantThumbnailScaler.h"
#include "G2dThumbnailScaler.h"

ThumbnailScaler *ThumbnailScaler::createInstance()
{
#ifdef USE_G2D_SCALER
    G2dThumbnailScaler *scaler = new G2dThumbnailScaler();
    if (scaler->available()) {
        ALOGI("Created thumbnail scaler: G2D Scaler");
        return scaler;
    }
#else
    GiantThumbnailScaler *scaler = new GiantThumbnailScaler();
    if (scaler->available()) {
        ALOGI("Created thumbnail scaler: GiantMscl");
        return scaler;
    }
#endif

    delete scaler;

    ALOGI("Created thumbnail scaler: legacy V4L2 Scaler");
    return new LibScalerForJpeg();
}
