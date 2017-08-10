#include <stdio.h>
#include <stdlib.h>
#include <math_neon.h>
#include <math.h>
#define USE_XENOMAI

#ifdef USE_XENOMAI
#include <native/task.h>
#include <native/timer.h>
#endif /* USE_XENOMAI */

#define numFuncs 40*2
float (*funcs[numFuncs])(float) = {
	sinf, sinf_c, sinf_neon,
	cosf, cosf_c, cosf_neon,
	//sincosf, sincosf_c, sincosf_neon,
	tanf, tanf_c, tanf_neon,
	atanf, atanf_c, atanf_neon,
	atan2f, atan2f_c, atan2f_neon,
	asinf, asinf_c, asinf_neon,
	acosf, acosf_c, acosf_neon,
	sinhf, sinhf_c, sinhf_neon,
	coshf, coshf_c, coshf_neon,
	tanhf, tanhf_c, tanhf_neon,
	expf, expf_c, expf_neon,
	logf, logf_c, logf_neon,
	log10f, log10f_c, log10f_neon,
	powf, powf_c, powf_neon,
	floorf, floorf_c, floorf_neon,
	ceilf, ceilf_c, ceilf_neon,
	fabsf, fabsf_c, fabsf_neon,
	ldexpf, ldexpf_c, ldexpf_neon,
	//frexpf, frexpf_c, frexpf_neon,
	fmodf, fmodf_c, fmodf_neon,
	modf, modf_c, modf_neon,
	sqrtf, sqrtf_c, sqrtf_neon
	//invsqrtf, invsqrtf_c, invsqrtf_neon,
};

char* funcNames[numFuncs]={
	"sinf",
	"cosf",
	//"sincosf",
	"tanf",
	"atanf",
	"atan2f",
	"asinf",
	"acosf",
	"sinhf",
	"coshf",
	"tanhf",
	"expf",
	"logf",
	"log10f",
	"powf",
	"floorf",
	"ceilf",
	"fabsf",
	"ldexpf",
	//"frexpf",
	"fmodf",
	"modf",
	"sqrtf",
	//"invsqrtf",
};

float max(float first, float second){
	return first > second ? first : second;
}
//maximum absolute, maximum relative and root mean squared 
float maxAbsErr(float** y, unsigned int length){
	float maxAbsErr = 0;
	for(unsigned int n = 0; n < length; ++n){
		float absErr = fabsf(y[0][n] - y[1][n]);
		maxAbsErr = max(absErr, maxAbsErr);
	}
	return maxAbsErr;
}

float maxRelErr(float** y, unsigned int length){
	float maxRelErr = 0;
	for(unsigned int n = 0; n < length; ++n){
		float ref = y[0][n] != 0 ? y[0][n] : y[1][n];
		float relErr;
		if(ref == 0){
			relErr = 0;
		} else {
			relErr = fabsf(y[0][n] - y[1][n]) / ref;
		}
		maxRelErr = max(relErr, maxRelErr);
	}
	return maxRelErr;
}

float rmsErr(float** y, unsigned int length){
	float sum = 0;
	for(unsigned int n = 0; n < length; ++n){
		float err = (y[0][n] - y[1][n]);
		sum += err * err;
	}
	return sqrt(sum / length);
}

#define numMetrics 3
float (*metrics[numMetrics])(float**, unsigned int) = {
	maxAbsErr,
	maxRelErr,
	rmsErr,
};

int main(){
#ifdef USE_XENOMAI
	rt_task_shadow(NULL, "math-neon test", 95, T_FPU);
#endif /* USE_XENOMAI */
	enable_runfast();
	unsigned int numTestTypes = 3; // libmath, _, _neon
	const unsigned int testLength = 500000;
	float x[testLength];
	float* y[numTestTypes];
	for(unsigned int n = 0; n < numTestTypes; ++n)
		y[n] = (float*)malloc(testLength * sizeof(float));
	for(unsigned int n = 0; n < testLength; ++n){
		x[n] = 0.0001 + n / (float)testLength * (1 - 0.0001);
	}
	printf( "     func        maxAbs          maxRel          rms        t-lib  t-c    t-neon\n");
	for(unsigned int n = 0; n < numFuncs; n += numTestTypes){
		float (*math[numTestTypes])(float);
		int skip = 0;
		RTIME elapsed[numTestTypes];
		for(unsigned int j = 0; j < numTestTypes; ++j){
			math[n] = funcs[n + j];
			if(math[n] == NULL){
				skip = 1;
				break;
			}
			RTIME time = rt_timer_read();
			for(unsigned int k = 0; k < testLength; ++k){
				y[j][k] = math[n](x[k]);
			}
			elapsed[j] = rt_timer_read() - time;
#ifdef USE_XENOMAI
			rt_task_sleep(1000000);//just to avoid SIGXCPU
#endif /* USE_XENOMAI */
		}
		if(skip == 1)
			break;
		printf("%10s ", funcNames[n/numTestTypes]);
		for(unsigned int j = 0; j < numMetrics; ++j){
			printf(" %15.8f",  metrics[j](y, testLength));
		}
		for(unsigned int j = 0; j < numTestTypes; ++j){
			printf(" %.3fs", (elapsed[j] / 1e6) / (float)1000);
		}
		printf("\n");
	}
	for(unsigned int n = 0; n < numTestTypes; ++n)
		free(y[n]);
}

