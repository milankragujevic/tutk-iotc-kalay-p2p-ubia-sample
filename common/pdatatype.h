// --------------------------------------------------------- //
// Copyright (c) 2013, andon All rights reserved。
// 文件名称： pdatatype.h
// 接口说明：
//          NONE
// 功能描述:
//          描述数据类型的定义，统一格式，方便对字节数的控制
// 历史记录:
//     <作者>       <时间>       <修改记录>
//     wangfengbo   2013-04-03  创建该文件
// --------------------------------------------------------- //


#ifndef _PDATATYPE_H_
#define _PDATATYPE_H_

typedef char               int8;          //  8 bits
typedef short              int16;         // 16 bits
typedef long               int32;         // 32 bits
typedef unsigned char      uint8;         //  8 bits
typedef unsigned short     uint16;        // 16 bits
typedef unsigned long      uint32;        // 32 bits
typedef float              float32;       // 32 bits
typedef double             double64;      // 64 bits
typedef unsigned long      ubool;         // 32 bits


#ifndef FALSE
#define FALSE                           0x00
#define TRUE                            0x01
#endif


#ifndef INVALID_ID                       // 无效ID
    #define INVALID_ID                  -1
#endif

#endif
