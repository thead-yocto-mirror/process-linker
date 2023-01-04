/*
 * Copyright (c) 2021-2022 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_VENC_SHAKE_HAL_H_
#define _ISP_VENC_SHAKE_HAL_H_

#ifndef IVS_MODULE_PATH
#define IVS_MODULE_PATH "/tmp/dev/theadivs"
#endif

typedef enum _IvsStatus
{
    IVS_STATUS_OK = 0,
    IVS_STATUS_ERROR = -1,
} IvsStatus;

typedef enum _IvsState
{
    IVS_STATE_IDLE,
    IVS_STATE_READY,
    IVS_STATE_RUNNING,
    IVS_STATE_ERROR
} IvsState;

typedef enum _IvsBufferMode
{
    IVS_BUFFER_MODE_FRAME = 0,
    IVS_BUFFER_MODE_SLICE,
    IVS_BUFFER_MODE_MAX
} IvsBufferMode;

typedef enum _IvsEncoderFormat
{
    IVS_ENCODER_FORMAT_H264 = 0,
    IVS_ENCODER_FORMAT_H265,
    IVS_ENCODER_FORMAT_MAX
} IvsEncoderFormat;

typedef struct _IvsConfig
{
    IvsBufferMode mode;
    IvsEncoderFormat encode;
    unsigned int pic_width;
    unsigned int pic_height;
    unsigned int encode_width;
    unsigned int encode_height;
    unsigned int buffer_height;   // valid in slice buffer mode only
    unsigned int stride_y;
    unsigned int stride_uv;
    unsigned int encode_x;
    unsigned int encode_y;
} IvsConfig;

IvsStatus createIspVencShake(void **ivs);
IvsStatus configIspVencShake(void *ivs, IvsConfig *config);
IvsStatus startIspVencShake(void *ivs);
IvsStatus resetIspVencShake(void *ivs);
IvsStatus getIspVencShakeState(void *ivs, int *state);
void destroyIspVencShake(void *ivs);

#endif /* !_ISP_VENC_SHAKE_HAL_H_ */
