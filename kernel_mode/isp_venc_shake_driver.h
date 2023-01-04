/*
 * Copyright (C) 2021 - 2022  Alibaba Group. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _ISP_VENC_SHAKE_DRIVER_H_
#define _ISP_VENC_SHAKE_DRIVER_H_
#include <linux/ioctl.h>

#define THEAD_IOC_MAGIC  't'

#define THEAD_IOCH_CONFIG_IVS    _IOR(THEAD_IOC_MAGIC, 3, unsigned long *)
#define THEAD_IOCH_START_IVS    _IOR(THEAD_IOC_MAGIC, 4, unsigned long *)
#define THEAD_IOCH_RESET_IVS    _IOR(THEAD_IOC_MAGIC, 5, unsigned long *)
#define THEAD_IOCH_GET_STATE    _IOR(THEAD_IOC_MAGIC, 6, int *)

#define THEAD_IOC_MAXNR 6

typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t   i16;
typedef uint16_t  u16;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef uint64_t  u64;

typedef enum _theadivs_state
{
    THEADIVS_IDLE,
    THEADIVS_READY,
    THEADIVS_RUNNING,
    THEADIVS_ERROR
} theadivs_state;

struct ivs_parameter
{
  u32 pic_width;
  u32 pic_height;
  u32 encode_width;
  u32 encode_height;
  u32 wid_y;
  u32 wid_uv;
  u32 sram_size;
  u32 encode_n;
  u32 stride_y;
  u32 stride_uv;
  //u32 clear;
  u32 encode_x;
  u32 encode_y;
  //u32 start;
  //u32 int_state;
  //u32 int_clean;
  u32 int_mask;
  //u32 signal_resv;
};

#endif /* !_ISP_VENC_SHAKE_DRIVER_H_ */
