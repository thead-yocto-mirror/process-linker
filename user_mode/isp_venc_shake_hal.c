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

#include <stdint.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include "isp_venc_shake_driver.h"
#include "isp_venc_shake_hal.h"
#include <sys/ioctl.h>

#ifndef NULL
#define NULL    ((void *)0)
#endif


#ifndef IVS_MODULE_PATH
#define IVS_MODULE_PATH             "/dev/shake"
#endif

typedef struct _IspVencShake
{
    unsigned int fd;
    struct ivs_parameter params;
} IspVencShake;

IvsStatus createIspVencShake(void **ivs)
{
    IvsStatus status = IVS_STATUS_OK;
    IspVencShake *shake = NULL;
    unsigned int fd;
    
    shake = (IspVencShake *)malloc(sizeof(IspVencShake));
    if (NULL == shake)
        return IVS_STATUS_ERROR;

    fd = open("/dev/shake", O_RDONLY);
    if (fd == -1)
    {
        free(shake);
        return IVS_STATUS_ERROR;
    }

    shake->fd = fd;
    *ivs = shake;

    return status;
}

IvsStatus configIspVencShake(void *ivs, IvsConfig *config)
{
    IvsStatus status = IVS_STATUS_OK;
    IspVencShake *shake = (IspVencShake *)ivs;
    struct ivs_parameter *params = NULL;
    unsigned int buffer_height;

    if (ivs == NULL || config == NULL)
        return IVS_STATUS_ERROR;

    params = &shake->params;
    buffer_height = config->mode = IVS_BUFFER_MODE_SLICE ? 
        config->buffer_height : config->pic_height * 3 / 2;

    params->pic_width = config->pic_width;
    params->pic_height = config->pic_height;
    params->encode_width = config->encode_width;
    params->encode_height = config->encode_height;
    params->wid_y = 1;
    params->wid_uv = 2;
    params->sram_size = config->pic_width * buffer_height;
    params->encode_n = config->encode == IVS_ENCODER_FORMAT_H264 ? 16 : 64;
    params->stride_y = config->stride_y;
    params->stride_uv = config->stride_uv;
    params->encode_x = config->encode_x;
    params->encode_y = config->encode_y;
    params->int_mask = 0;

    if (ioctl(shake->fd, THEAD_IOCH_CONFIG_IVS, params) == -1)
    {
        status = IVS_STATUS_ERROR;
    }

    return status;
}

IvsStatus startIspVencShake(void *ivs)
{
    IvsStatus status = IVS_STATUS_OK;
    IspVencShake *shake = (IspVencShake *)ivs;

    if (ivs == NULL)
        return IVS_STATUS_ERROR;

    if (ioctl(shake->fd, THEAD_IOCH_START_IVS, NULL) == -1)
    {
        status = IVS_STATUS_ERROR;
    }

    return status;
}

IvsStatus resetIspVencShake(void *ivs)
{
    IvsStatus status = IVS_STATUS_OK;
    IspVencShake *shake = (IspVencShake *)ivs;

    if (ivs == NULL)
        return IVS_STATUS_ERROR;

    if (ioctl(shake->fd, THEAD_IOCH_RESET_IVS, NULL) == -1)
    {
        status = IVS_STATUS_ERROR;
    }

    return status;
}

IvsStatus getIspVencShakeState(void *ivs, int *state)
{
    IvsStatus status = IVS_STATUS_OK;
    IspVencShake *shake = (IspVencShake *)ivs;

    if (ivs == NULL || state == NULL)
        return IVS_STATUS_ERROR;

    if (ioctl(shake->fd, THEAD_IOCH_GET_STATE, state) == -1)
    {
        status = IVS_STATUS_ERROR;
    }

    return status;
}

void destroyIspVencShake(void *ivs)
{
    IspVencShake *shake = (IspVencShake *)ivs;

    if (NULL == ivs)
        return;

    if (shake->fd != -1)
        close(shake->fd);

    free(ivs);
}

