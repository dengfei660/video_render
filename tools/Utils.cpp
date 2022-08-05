/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>

//namespace Tls {
unsigned int getU32( unsigned char *p )
{
   unsigned n;

   n= (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]);

   return n;
}

int putU32( unsigned char *p, unsigned n )
{
   p[0]= (n>>24);
   p[1]= (n>>16);
   p[2]= (n>>8);
   p[3]= (n&0xFF);

   return 4;
}

int64_t getS64( unsigned char *p )
{
    int64_t n;

    n= ((((int64_t)(p[0]))<<56) |
       (((int64_t)(p[1]))<<48) |
       (((int64_t)(p[2]))<<40) |
       (((int64_t)(p[3]))<<32) |
       (((int64_t)(p[4]))<<24) |
       (((int64_t)(p[5]))<<16) |
       (((int64_t)(p[6]))<<8) |
       (p[7]) );

   return n;
}

int putS64( unsigned char *p,  int64_t n )
{
   p[0]= (((uint64_t)n)>>56);
   p[1]= (((uint64_t)n)>>48);
   p[2]= (((uint64_t)n)>>40);
   p[3]= (((uint64_t)n)>>32);
   p[4]= (((uint64_t)n)>>24);
   p[5]= (((uint64_t)n)>>16);
   p[6]= (((uint64_t)n)>>8);
   p[7]= (((uint64_t)n)&0xFF);

   return 8;
}
//}