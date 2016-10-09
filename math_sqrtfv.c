/*
The MIT License (MIT)

Copyright (c) 2015 Lachlan Tychsen-Smith (lachlan.ts@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*
Test func : sqrtf(x)
Test Range: 0 < x < 1,000,000,000
Peak Error:	~0.0010%
RMS  Error: ~0.0005%
*/

#include "math.h"
#include "math_neon.h"

void sqrtfv_c(float *x, int n, float *r)
{

	float x0, x1;
	float b0, b1, c0, c1;
	int m0, m1;
	union {
		float 	f;
		int 	i;
	} a0, a1;


	if (n & 0x1){
		*r++ = sqrtf_c(*x++);
		n--;
	}

	while(n > 0){
	
		x0 = *x++;
		x1 = *x++;
	
		//fast invsqrt approx
		a0.f = x0;
		a1.f = x1;
		a0.i = 0x5F3759DF - (a0.i >> 1);		//VRSQRTE
		a1.i = 0x5F3759DF - (a1.i >> 1);		//VRSQRTE
		c0 = x0 * a0.f;
		c1 = x1 * a1.f;
		b0 = (3.0f - c0 * a0.f) * 0.5;		//VRSQRTS
		b1 = (3.0f - c1 * a1.f) * 0.5;		//VRSQRTS
		a0.f = a0.f * b0;		
		a1.f = a1.f * b1;		
		c0 = x0 * a0.f;
		c1 = x1 * a1.f;
		b0 = (3.0f - c0 * a0.f) * 0.5;		//VRSQRTS
		b1 = (3.0f - c1 * a1.f) * 0.5;		//VRSQRTS
		a0.f = a0.f * b0;		
		a1.f = a1.f * b1;		

		//fast inverse approx
		c0 = a0.f;
		c1 = a1.f;
		m0 = 0x3F800000 - (a0.i & 0x7F800000);
		m1 = 0x3F800000 - (a1.i & 0x7F800000);
		a0.i = a0.i + m0;
		a1.i = a1.i + m1;
		a0.f = 1.41176471f - 0.47058824f * a0.f;
		a1.f = 1.41176471f - 0.47058824f * a1.f;
		a0.i = a0.i + m0;
		a1.i = a1.i + m1;
		b0 = 2.0 - a0.f * c0;
		b1 = 2.0 - a1.f * c1;
		a0.f = a0.f * b0;	
		a1.f = a1.f * b1;	
		b0 = 2.0 - a0.f * c0;
		b1 = 2.0 - a1.f * c1;
		a0.f = a0.f * b0;
		a1.f = a1.f * b1;
		
		*r++ = a0.f;
		*r++ = a1.f;
		n -= 2;

	}
}

void sqrtfv_neon(float *x, int n, float *y)
{
	// take care of the % 4 values
	for(int k = 0; k < (n&3); ++k){
		*y++ = sqrtf_neon(*x++);
	}
	n ^= n&3;
	if(n == 0)
		return;
#ifdef __MATH_NEON
	//assumes hard_float, multiples of four
	asm volatile (
	"vld1.32 		{d0, d1}, [%0]! 				\n\t"	//q0 = (*x[0], *x[1]), x+=2;
	"lb: \n\t"
	
	//fast invsqrt approx
	"vmov.f32 		q1, q0					\n\t"	//q1 = q0
	"vrsqrte.f32 	q0, q0					\n\t"	//q0 = ~ 1.0 / sqrt(q0)
	"vmul.f32 		q2, q0, q1				\n\t"	//q3 = q0 * q2
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q4 = (3 - q0 * q3) / 2 	
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q4	
	"vmul.f32 		q2, q0, q1				\n\t"	//q3 = q0 * q2	
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q4 = (3 - q0 * q3) / 2	
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q4	
		
	"subs 			%1, %1, #4				\n\t"	//n = n - 2; update flags
	//fast reciporical approximation
	"vrecpe.f32		q1, q0					\n\t"	//q1 = ~ 1 / q0; 
	"vrecps.f32		q2, q1, q0				\n\t"	//q2 = 2.0 - q1 * q0; 
	"vmul.f32		q1, q1, q2				\n\t"	//q1 = q1 * q2; 
	"vrecps.f32		q2, q1, q0				\n\t"	//q2 = 2.0 - q1 * q0; 
	"it le \n\t"
	"ble preloaded\n\t"
	// preload next input UNLESS we are at the end of the loop
	"vld1.32 		{d0, d1}, [%0]! 				\n\t"	//q0 = (*x[0], *x[1]), x+=2;
	"preloaded:	 \n\t"
	"vmul.f32		q3, q1, q2				\n\t"	//q0 = q1 * q2; 

	"vst1.32 		{d6, d7}, [%2]!				\n\t"	//*r++ = q0;
	"bgt 			lb 						\n\t"	//

	: "+r"(x), "+r"(n), "+r"(y)  // outputs 
	:
	: "q0", "q1", "q2", "q3", "r0", "r1", "r2"
);
#else
	sqrtfv_c(x, n, r);
#endif
}
