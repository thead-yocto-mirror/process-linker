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

#include <stdio.h>
#include <unistd.h>
#include "isp_venc_shake_hal.h"

#ifndef NULL
#define NULL    ((void *)0)
#endif


int main(void)
{
    void *ivs = NULL;
    IvsConfig config;
    IvsStatus status = IVS_STATUS_OK;

    do
    {
        status = createIspVencShake(&ivs);
        if (status != IVS_STATUS_OK)
            break;

        status = resetIspVencShake(ivs);
        
        config.mode = IVS_BUFFER_MODE_FRAME;
        config.encode = IVS_ENCODER_FORMAT_H264;
        config.pic_width = 640;
        config.pic_height = 480;
        config.encode_width = 640;
        config.encode_height = 480;
        config.buffer_height = 240;
        config.stride_y = 640;
        config.stride_uv = 640;
        config.encode_x = 0;
        config.encode_y = 0;
        status = configIspVencShake(ivs, &config);
        if (status != IVS_STATUS_OK)
            break;
        
        status = startIspVencShake(ivs);
        if (status != IVS_STATUS_OK)
            break;

        sleep(90);

        status = resetIspVencShake(ivs);
        if (status != IVS_STATUS_OK)
            break;
    } while (0);
    
    destroyIspVencShake(ivs);

    if (status != IVS_STATUS_OK)
        printf("ERROR: ivs_test exit with error!\n");
    else
        printf("ivs_test done!\n");

    return (status == IVS_STATUS_OK ? 0 : 1);
}

