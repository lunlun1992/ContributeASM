#ifndef _C_FUNCTIONS_H_
#define _C_FUNCTIONS_H_

#include <stdint.h>

typedef uint8_t pixel;

typedef struct  
{
	// intra_normal_filter[i_log2_size - 2] -> intra filter function 
	void (*intra_normal_filter[4])(pixel *src, pixel *dst);

	// 35 modes
	// 0~3: intra_pred[i][i_log2_size - 2] -> intra normal pred function
	// 4~7: intra_pred[i][i_log2_size + 2] -> edge filter after intra normal pred function
	void (*intra_pred[35][8])(pixel *dst, int dst_stride, const pixel *ref_pix);
} predict_function_t;

static inline pixel CLIP_PIXEL( int x )
{
	return ( (x & ~255) ? (-x)>>31 & 255 : x );
}

void init_pred_funcs(predict_function_t *predf);

#endif  // _C_FUNCTIONS_H_