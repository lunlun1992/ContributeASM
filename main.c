#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "C_functions.h"

pixel ref_pix_unfiltered[144];
pixel ref_pix_filtered[144];

pixel pred_pixs[32 * 128];

predict_function_t predf;

void random_ref_pix(pixel *src, int total_size) {
	for (int i = 0; i < total_size; i++)
		ref_pix_unfiltered[i] = rand() % 256;
}

void test_filter_func(void (*func_name)(pixel *, pixel *) , int size) {
	random_ref_pix(ref_pix_unfiltered, 1 + (size << 2));
	pixel tmp[144];
	int total_size = 1 + (size << 1) + (size << 1);
	switch (size)
	{
	case 8:
		predf.intra_normal_filter[1](ref_pix_unfiltered, ref_pix_filtered);
		func_name(ref_pix_unfiltered, tmp);
		break;
	case 16:
		predf.intra_normal_filter[1](ref_pix_unfiltered, ref_pix_filtered);
		func_name(ref_pix_unfiltered, tmp);
		break;
	case 32:
		predf.intra_normal_filter[1](ref_pix_unfiltered, ref_pix_filtered);
		func_name(ref_pix_unfiltered, tmp);
		break;
	default:
		assert(0);
	}

	for (int i = 0; i < total_size; i++)
		assert(ref_pix_filtered[i] == tmp[i]);
}

void test_pred_func(void(*func_name)(pixel *, int, pixel *), int mode, int size, int b_edge) {
	random_ref_pix(ref_pix_filtered, 1 + (size << 2));
	pixel tmp[32 * 128];
	int total_size = 1 + (size << 1) + (size << 1);

	if (b_edge)
	{
		switch (size)
		{
		case 4:
			predf.intra_pred[mode][4](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 8:
			predf.intra_pred[mode][5](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 16:
			predf.intra_pred[mode][6](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 32:
			predf.intra_pred[mode][7](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		default:
			assert(0);
		}
	}
	else
	{
		switch (size)
		{
		case 4:
			predf.intra_pred[mode][0](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 8:
			predf.intra_pred[mode][1](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 16:
			predf.intra_pred[mode][2](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		case 32:
			predf.intra_pred[mode][3](pred_pixs, 128, ref_pix_filtered);
			func_name(tmp, 128, ref_pix_filtered);
			break;
		default:
			assert(0);
		}
	}

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			assert(pred_pixs[i * 128 + j] == tmp[i * 128 + j]);
}

void lent_intra_filter_8x8_sse4(pixel *, pixel *);
void lent_intra_pred_ang16_15_avx2(pixel *, int, pixel *);
int main (int argc, char **argv) {
	srand((unsigned)time(NULL));
	init_pred_funcs(&predf);
	
	test_filter_func(lent_intra_filter_8x8_sse4, 8);
	test_pred_func(lent_intra_pred_ang16_15_avx2, 15, 16, 0);

    return 0;
}