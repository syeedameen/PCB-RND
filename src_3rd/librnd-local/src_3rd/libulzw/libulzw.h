/*//////////////////////////////////////////////////////////////////////////
//                            **** LIBULZW ****                           //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                     Copyright (c) 2016 David Bryant                    //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
//////////////////////////////////////////////////////////////////////////*/

#ifndef LIBULZW_H_
#define LIBULZW_H_

int ulzw_compress(void *ctx, void (*dst)(void *, int), int(*src)(void *), int maxbits);
int ulzw_decompress(void *ctx, void (*dst)(void *, int), int(*src)(void *));

#endif /* LIBULZWLIB_H_ */
