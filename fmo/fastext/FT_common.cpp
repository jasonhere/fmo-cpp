/* This is FAST corner detector, contributed to OpenCV by the author, Edward Rosten.
   Below is the original copyright and the references */

/*
Copyright (c) 2006, 2008 Edward Rosten
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    *Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

    *Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

    *Neither the name of the University of Cambridge nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
The references are:
 * Machine learning for high-speed corner detection,
   E. Rosten and T. Drummond, ECCV 2006
 * Faster and better: A machine learning approach to corner detection
   E. Rosten, R. Porter and T. Drummond, PAMI, 2009
*/

#include "FT_common.hpp"

#define VERIFY_CORNERS 0

namespace cmp {

void makeOffsets(int pixel[34], int* corners, int* cornersOut, int rowStride, int patternSize, int pixelIndex[34],  int pixelcheck[24], int pixelcheck16[16])
{
	static const int offsets24[][2] =
	{
		{0,  4}, { 1,  4}, { 2,  4}, { 3,  3}, { 4, 2}, { 4, 1}, { 4, 0}, { 4, -1},
		{ 4, -2}, { 3, -3}, { 2, -4}, { 1, -4}, {0, -4}, {-1, -4}, {-2, -4}, {-3, -3},
		{-4, -2}, {-4, -1}, {-4, 0}, {-4,  1}, {-4,  2}, {-3,  3}, {-2,  4}, {-1,  4}
	};

    static const int offsets16[][2] =
    {
        {0,  3}, { 1,  3}, { 2,  2}, { 3,  1}, { 3, 0}, { 3, -1}, { 2, -2}, { 1, -3},
        {0, -3}, {-1, -3}, {-2, -2}, {-3, -1}, {-3, 0}, {-3,  1}, {-2,  2}, {-1,  3}
    };

    static const int corners16[][2] =
    {
        { 3,  2}, { 2,  3}, { 3, -2}, { 2, -3},
		{-2, -3}, {-3, -2} , {-3,  2}, {-2,  3}
    };

    static const int offsets12[][2] =
    {
        {0,  2}, { 1,  2}, { 2,  1}, { 2, 0}, { 2, -1}, { 1, -2},
        {0, -2}, {-1, -2}, {-2, -1}, {-2, 0}, {-2,  1}, {-1,  2}
    };

    static const int corners12[][2] =
    {
    	{ 2,  2}, { 2, -2},
		{-2, -2}, {-2,  2}
    };
    static const int cornersOut12[][2] =
    {
    	{ 3,  3}, { 3, -3},
    	{-3, -3}, {-3,  3}
    };


    static const int offsets8[][2] =
    {
        {0,  1}, { 1,  1}, { 1, 0}, { 1, -1},
        {0, -1}, {-1, -1}, {-1, 0}, {-1,  1}
    };

    const int (*offsets)[2] = patternSize == 16 ? offsets16 :
                              patternSize == 12 ? offsets12 :
                              patternSize == 8  ? offsets8  : 0;

    CV_Assert(pixel && offsets);

    int k = 0;
    for( ; k < patternSize; k++ )
    {
        pixel[k] = offsets[k][0] + offsets[k][1] * rowStride;
        pixelIndex[k] = k;
    }
    for( ; k < 34; k++ )
    {
        pixel[k] = pixel[k - patternSize];
        pixelIndex[k] = k - patternSize;
    }
    if(patternSize == 16)
    {
    	for( k = 0; k < 8; k++ )
    		corners[k] = corners16[k][0] + corners16[k][1] * rowStride;
    }else{
    	for( k = 0; k < 4; k++ )
    	{
    		corners[k] = corners12[k][0] + corners12[k][1] * rowStride;
    		cornersOut[k] = cornersOut12[k][0] + cornersOut12[k][1] * rowStride;
    	}
    }
    for( k = 0; k < 24; k++ )
    	pixelcheck[k] =  offsets24[k][0] + offsets24[k][1] * rowStride;
    for( k = 0; k < 16; k++ )
    	pixelcheck16[k] =  offsets16[k][0] + offsets16[k][1] * rowStride;
}

void makeOffsetsC(int pixel[34], int pixelCounter[34], int corners[8], int rowStride, int patternSize, int pixelcheck[24], int pixelcheck16[16])
{
	static const int offsets24[][2] =
	{
		{0,  4}, { 1,  4}, { 2,  4}, { 3,  3}, { 4, 2}, { 4, 1}, { 4, 0}, { 4, -1},
		{ 4, -2}, { 3, -3}, { 2, -4}, { 1, -4}, {0, -4}, {-1, -4}, {-2, -4}, {-3, -3},
		{-4, -2}, {-4, -1}, {-4, 0}, {-4,  1}, {-4,  2}, {-3,  3}, {-2,  4}, {-1,  4}
	};

    static const int offsets16[][2] =
    {
        {0,  3}, { 1,  3}, { 2,  2}, { 3,  1}, { 3, 0}, { 3, -1}, { 2, -2}, { 1, -3},
        {0, -3}, {-1, -3}, {-2, -2}, {-3, -1}, {-3, 0}, {-3,  1}, {-2,  2}, {-1,  3}
    };

    static const int corners16[][2] =
    {
    	{ 3,  2}, { 2,  3}, { 3, -2}, { 2, -3},
		{-2, -3}, {-3, -2} , {-3,  2}, {-2,  3}
    };

    static const int offsets12[][2] =
    {
        {0,  2}, { 1,  2}, { 2,  1}, { 2, 0}, { 2, -1}, { 1, -2},
        {0, -2}, {-1, -2}, {-2, -1}, {-2, 0}, {-2,  1}, {-1,  2}
    };

    static const int corners12[][2] =
    {
    	{ 2,  2}, { 2, -2},
		{-2, -2}, {-2,  2}
    };

    static const int offsets8[][2] =
    {
        {0,  1}, { 1,  1}, { 1, 0}, { 1, -1},
        {0, -1}, {-1, -1}, {-1, 0}, {-1,  1}
    };

    const int (*offsets)[2] = patternSize == 16 ? offsets16 :
                              patternSize == 12 ? offsets12 :
                              patternSize == 8  ? offsets8  : 0;

    CV_Assert(pixel && offsets);

    int k = 0;
    for( ; k < patternSize; k++ )
    {
        pixel[k] = 3 * offsets[k][0] + offsets[k][1] * rowStride;
        pixelCounter[k] = k;
    }
    for( ; k < 34; k++ )
    {
        pixel[k] = pixel[k - patternSize];
        pixelCounter[k] = k - patternSize;
    }

    if(patternSize == 16)
    {
    	for( k = 0; k < 8; k++ )
    		corners[k] = 3 * corners16[k][0] + corners16[k][1] * rowStride;
    }else{
    	for( k = 0; k < 4; k++ )
    		corners[k] = 3 * corners12[k][0] + corners12[k][1] * rowStride;
    }
    for( k = 0; k < 24; k++ )
    	pixelcheck[k] =  3 * offsets24[k][0] + offsets24[k][1] * rowStride;
    for( k = 0; k < 16; k++ )
    	pixelcheck16[k] =  3 * offsets16[k][0] + offsets16[k][1] * rowStride;
}

#if VERIFY_CORNERS
static void testCorner(const uchar* ptr, const int pixel[], int K, int N, int threshold) {
    // check that with the computed "threshold" the pixel is still a corner
    // and that with the increased-by-1 "threshold" the pixel is not a corner anymore
    for( int delta = 0; delta <= 1; delta++ )
    {
        int v0 = std::min(ptr[0] + threshold + delta, 255);
        int v1 = std::max(ptr[0] - threshold - delta, 0);
        int c0 = 0, c1 = 0;

        for( int k = 0; k < N; k++ )
        {
            int x = ptr[pixel[k]];
            if(x > v0)
            {
                if( ++c0 > K )
                    break;
                c1 = 0;
            }
            else if( x < v1 )
            {
                if( ++c1 > K )
                    break;
                c0 = 0;
            }
            else
            {
                c0 = c1 = 0;
            }
        }
        CV_Assert( (delta == 0 && std::max(c0, c1) > K) ||
                   (delta == 1 && std::max(c0, c1) <= K) );
    }
}
#endif

template<>
int cornerScore<16>(const uchar* ptr, const int pixel[], int threshold)
{
    const int K = 8, N = K*3 + 1;
    int k, v = ptr[0];
    short d[N];
    for( k = 0; k < N; k++ )
        d[k] = (short)(v - ptr[pixel[k]]);

    int a0 = threshold;
    for( k = 0; k < 16; k += 2 )
    {
        int a = std::min((int)d[k+1], (int)d[k+2]);
        a = std::min(a, (int)d[k+3]);
        if( a <= a0 )
            continue;
        a = std::min(a, (int)d[k+4]);
        a = std::min(a, (int)d[k+5]);
        a = std::min(a, (int)d[k+6]);
        a = std::min(a, (int)d[k+7]);
        a = std::min(a, (int)d[k+8]);
        a0 = std::max(a0, std::min(a, (int)d[k]));
        a0 = std::max(a0, std::min(a, (int)d[k+9]));
    }

    int b0 = -a0;
    for( k = 0; k < 16; k += 2 )
    {
        int b = std::max((int)d[k+1], (int)d[k+2]);
        b = std::max(b, (int)d[k+3]);
        b = std::max(b, (int)d[k+4]);
        b = std::max(b, (int)d[k+5]);
        if( b >= b0 )
            continue;
        b = std::max(b, (int)d[k+6]);
        b = std::max(b, (int)d[k+7]);
        b = std::max(b, (int)d[k+8]);

        b0 = std::min(b0, std::max(b, (int)d[k]));
        b0 = std::min(b0, std::max(b, (int)d[k+9]));
    }

    threshold = -b0-1;

#if VERIFY_CORNERS
    testCorner(ptr, pixel, K, N, threshold);
#endif
    return threshold;
}

template<>
int cornerScore<12>(const uchar* ptr, const int pixel[], int threshold)
{
    const int K = 6, N = K*3 + 1;
    int k, v = ptr[0];
    short d[N + 4];
    for( k = 0; k < N; k++ )
        d[k] = (short)(v - ptr[pixel[k]]);


    int a0 = threshold;
    for( k = 0; k < 12; k += 2 )
    {
        int a = std::min((int)d[k+1], (int)d[k+2]);
        if( a <= a0 )
            continue;
        a = std::min(a, (int)d[k+3]);
        a = std::min(a, (int)d[k+4]);
        a = std::min(a, (int)d[k+5]);
        a = std::min(a, (int)d[k+6]);
        a0 = std::max(a0, std::min(a, (int)d[k]));
        a0 = std::max(a0, std::min(a, (int)d[k+7]));
    }

    int b0 = -a0;
    for( k = 0; k < 12; k += 2 )
    {
        int b = std::max((int)d[k+1], (int)d[k+2]);
        b = std::max(b, (int)d[k+3]);
        b = std::max(b, (int)d[k+4]);
        if( b >= b0 )
            continue;
        b = std::max(b, (int)d[k+5]);
        b = std::max(b, (int)d[k+6]);

        b0 = std::min(b0, std::max(b, (int)d[k]));
        b0 = std::min(b0, std::max(b, (int)d[k+7]));
    }

    threshold = -b0-1;

#if VERIFY_CORNERS
    testCorner(ptr, pixel, K, N, threshold);
#endif
    return threshold;
}

template<>
int cornerScore<8>(const uchar* ptr, const int pixel[], int threshold)
{
    const int K = 4, N = K*3 + 1;
    int k, v = ptr[0];
    short d[N];
    for( k = 0; k < N; k++ )
        d[k] = (short)(v - ptr[pixel[k]]);

    int a0 = threshold;
    for( k = 0; k < 8; k += 2 )
    {
        int a = std::min((int)d[k+1], (int)d[k+2]);
        if( a <= a0 )
            continue;
        a = std::min(a, (int)d[k+3]);
        a = std::min(a, (int)d[k+4]);
        a0 = std::max(a0, std::min(a, (int)d[k]));
        a0 = std::max(a0, std::min(a, (int)d[k+5]));
    }

    int b0 = -a0;
    for( k = 0; k < 8; k += 2 )
    {
        int b = std::max((int)d[k+1], (int)d[k+2]);
        b = std::max(b, (int)d[k+3]);
        if( b >= b0 )
            continue;
        b = std::max(b, (int)d[k+4]);

        b0 = std::min(b0, std::max(b, (int)d[k]));
        b0 = std::min(b0, std::max(b, (int)d[k+5]));
    }

    threshold = -b0-1;

#if VERIFY_CORNERS
    testCorner(ptr, pixel, K, N, threshold);
#endif
    return threshold;
}

} // namespace cmp