#include "C_functions.h"

#include <stdint.h>

// Normal Intra Filter
#define F3(a,b,c) (((int)(a)+((b)<<1)+(c)+2)>>2)
#define DEFINE_INTRA_NORMAL_FILTER(s) \
void lent_intra_normal_filter_##s##x##s##_c(pixel *src, pixel *dst) \
{ \
	int i = 0; \
	const int i_size2 = (s) << 1; \
	pixel topLeft = src[0]; \
	pixel topLast = src[i_size2]; \
	pixel leftLast = src[i_size2 << 1]; \
	for (i = 1; i < i_size2; i++) \
		dst[i] = ((src[i] << 1) + src[i - 1] + src[i + 1] + 2) >> 2; \
	dst[i_size2] = topLast; \
	dst[0] = ((topLeft << 1) + src[1] + src[i_size2 + 1] + 2) >> 2; \
	dst[i_size2 + 1] = ((src[i_size2 + 1] << 1) + topLeft + src[i_size2 + 2] + 2) >> 2; \
	for (i = i_size2 + 2; i < i_size2 + i_size2; i++) \
		dst[i] = ((src[i] << 1) + src[i - 1] + src[i + 1] + 2) >> 2; \
	dst[i_size2 + i_size2] = leftLast; \
}
DEFINE_INTRA_NORMAL_FILTER(4)
DEFINE_INTRA_NORMAL_FILTER(8)
DEFINE_INTRA_NORMAL_FILTER(16)
DEFINE_INTRA_NORMAL_FILTER(32)
#undef DEFINE_INTRA_NORMAL_FILTER
#undef F3

// Normal Intra Predict Functions
// Mode 0: Planar
#define DEFINE_PLANAR_0_FUNC(ss, ll) \
void lent_intra_pred_planar_0_##ss##x##ss##_c(pixel *dst, int dst_stride, const pixel *ref_pix) \
{ \
	int x, y; \
	const int i_log2_size = ll; \
	const int i_size = ss; \
	const pixel* above = ref_pix + 1; \
	const pixel* left = ref_pix + (2 * i_size + 1); \
	pixel topRight = above[i_size]; \
	pixel bottomLeft = left[i_size]; \
	for (y = 0; y < i_size; y++) \
		for (x = 0; x < i_size; x++) \
			dst[y * dst_stride + x] = (pixel)(((i_size - 1 - x) * left[y] + (i_size - 1 - y) * above[x] + (x + 1) * topRight + (y + 1) * bottomLeft + i_size) >> (i_log2_size + 1)); \
}

#define DEFINE_PLANAR_0_FUNCS \
	DEFINE_PLANAR_0_FUNC(4, 2) \
	DEFINE_PLANAR_0_FUNC(8, 3) \
	DEFINE_PLANAR_0_FUNC(16, 4) \
	DEFINE_PLANAR_0_FUNC(32, 5)
DEFINE_PLANAR_0_FUNCS
#undef DEFINE_PLANAR_0_FUNC
#undef DEFINE_PLANAR_0_FUNCS

// Mode 1: DC
#define DEFINE_DC_NORMAL_1_FUNC(ss) \
void lent_intra_pred_dc_1_##ss##x##ss##_c(pixel *dst, int dst_stride, const pixel *ref_pix) \
{ \
    int x, y, i; \
	const int i_size = ss; \
	int i_dc_val = i_size; \
	for (i = 0; i < i_size; i++) \
		i_dc_val += ref_pix[1 + i] + ref_pix[2 * i_size + 1 + i]; \
	i_dc_val /= (i_size << 1); \
	for (y = 0; y < i_size; y++) \
		for (x = 0; x < i_size; x++) \
			dst[y * dst_stride + x] = (pixel)i_dc_val; \
}
#define DEFINE_DC_NORMAL_1_FUNCS \
	DEFINE_DC_NORMAL_1_FUNC(4) \
	DEFINE_DC_NORMAL_1_FUNC(8) \
	DEFINE_DC_NORMAL_1_FUNC(16)
void lent_intra_pred_dc_1_32x32_c(pixel *dst, int dst_stride, const pixel *ref_pix)
{
	int x, y, i;
	const int i_size = 32;
	int i_dc_val = i_size;
	for (i = 0; i < i_size; i++)
		i_dc_val += ref_pix[1 + i] + ref_pix[2 * i_size + 1 + i];
	i_dc_val /= (i_size << 1);
	for (y = 0; y < i_size; y++)
		for (x = 0; x < i_size; x++)
			dst[y * dst_stride + x] = (pixel)i_dc_val;
}
DEFINE_DC_NORMAL_1_FUNCS
#undef DEFINE_DC_NORMAL_1_FUNC
#undef DEFINE_DC_NORMAL_1_FUNCS

//Mode 2~34: Angular
void lent_intra_pred_ang_c(pixel *dst, int dst_stride, const pixel *ref_pix, int i_mode, const int i_width, int b_edge) 
{
	const int i_width2 = i_width << 1;
	// Flip the neighbours in the horizontal case.
	int b_hor_mode = i_mode < 18;
	pixel p_neighbour_buf[129];
	const pixel *p_src_pix = ref_pix;

	if (b_hor_mode)
	{
		p_neighbour_buf[0] = p_src_pix[0];
		for (int i = 0; i < i_width2; i++)
		{
			p_neighbour_buf[1 + i] = p_src_pix[i_width2 + 1 + i];
			p_neighbour_buf[i_width2 + 1 + i] = p_src_pix[1 + i];
		}
		p_src_pix = p_neighbour_buf;
	}

	// Intra prediction angle and inverse angle tables.
	const int8_t g_angle_table[17] = { -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32 };
	const int16_t g_inv_angle_table[8] = { 4096, 1638, 910, 630, 482, 390, 315, 256 };

	// Get the prediction angle.
	int i_angle_offset = b_hor_mode ? 10 - i_mode : i_mode - 26;
	int angle = g_angle_table[8 + i_angle_offset];

	// Vertical Prediction.
	if (!angle)
	{
		for (int y = 0; y < i_width; y++)
			for (int x = 0; x < i_width; x++)
				dst[y * dst_stride + x] = p_src_pix[1 + x];

		if (b_edge)
		{
			int i_top_left = p_src_pix[0], top = p_src_pix[1];
			for (int y = 0; y < i_width; y++)
				dst[y * dst_stride] = CLIP_PIXEL((int16_t)(top + ((p_src_pix[i_width2 + 1 + y] - i_top_left) >> 1)));
		}
	}
	else // Angular prediction.
	{
		// Get the reference pixels. The reference base is the first pixel to the top (neighbourBuf[1]).
		pixel p_ref_buf[64];
		const pixel *ref;

		// Use the projected left neighbours and the top neighbours.
		if (angle < 0)
		{
			// Number of neighbours projected. 
			int nb_projected = -((i_width * angle) >> 5) - 1;
			pixel *local_ref_pix = p_ref_buf + nb_projected + 1;

			// Project the neighbours.
			int i_inv_angle = g_inv_angle_table[-i_angle_offset - 1];
			int i_inv_angle_sum = 128;
			for (int i = 0; i < nb_projected; i++)
			{
				i_inv_angle_sum += i_inv_angle;
				local_ref_pix[-2 - i] = p_src_pix[i_width2 + (i_inv_angle_sum >> 8)];
			}

			// Copy the top-left and top pixels.
			for (int i = 0; i < i_width + 1; i++)
				local_ref_pix[-1 + i] = p_src_pix[i];
			ref = local_ref_pix;
		}
		else // Use the top and top-right neighbours.
			ref = p_src_pix + 1;

		// Pass every row.
		int i_angle_sum = 0;
		for (int y = 0; y < i_width; y++)
		{
			i_angle_sum += angle;
			int offset = i_angle_sum >> 5;
			int fraction = i_angle_sum & 31;

			if (fraction) // Interpolate
				for (int x = 0; x < i_width; x++)
					dst[y * dst_stride + x] = (pixel)(((32 - fraction) * ref[offset + x] + fraction * ref[offset + x + 1] + 16) >> 5);
			else // Copy.
				for (int x = 0; x < i_width; x++)
					dst[y * dst_stride + x] = ref[offset + x];
		}
	}

	// Flip for horizontal.
	if (b_hor_mode)
	{
		for (int y = 0; y < i_width - 1; y++)
		{
			for (int x = y + 1; x < i_width; x++)
			{
				pixel tmp = dst[y * dst_stride + x];
				dst[y * dst_stride + x] = dst[x * dst_stride + y];
				dst[x * dst_stride + y] = tmp;
			}
		}
	}
}

#define DEFINE_SINGLE_ANG_FUNC(m, s) \
void lent_intra_pred_ang_##m##_##s##x##s##_c(pixel *dst, int dst_stride, const pixel *ref_pix) { \
	lent_intra_pred_ang_c(dst, dst_stride, ref_pix, m, s, 0); \
}

#define DEFINE_NORMAL_ANG_FUNCS(m) \
	DEFINE_SINGLE_ANG_FUNC(m, 4) \
	DEFINE_SINGLE_ANG_FUNC(m, 8) \
	DEFINE_SINGLE_ANG_FUNC(m, 16) \
	DEFINE_SINGLE_ANG_FUNC(m, 32)

DEFINE_NORMAL_ANG_FUNCS(2)
DEFINE_NORMAL_ANG_FUNCS(3)
DEFINE_NORMAL_ANG_FUNCS(4)
DEFINE_NORMAL_ANG_FUNCS(5)
DEFINE_NORMAL_ANG_FUNCS(6)
DEFINE_NORMAL_ANG_FUNCS(7)
DEFINE_NORMAL_ANG_FUNCS(8)
DEFINE_NORMAL_ANG_FUNCS(9)
DEFINE_NORMAL_ANG_FUNCS(10)
DEFINE_NORMAL_ANG_FUNCS(11)
DEFINE_NORMAL_ANG_FUNCS(12)
DEFINE_NORMAL_ANG_FUNCS(13)
DEFINE_NORMAL_ANG_FUNCS(14)
DEFINE_NORMAL_ANG_FUNCS(15)
DEFINE_NORMAL_ANG_FUNCS(16)
DEFINE_NORMAL_ANG_FUNCS(17)
DEFINE_NORMAL_ANG_FUNCS(18)
DEFINE_NORMAL_ANG_FUNCS(19)
DEFINE_NORMAL_ANG_FUNCS(20)
DEFINE_NORMAL_ANG_FUNCS(21)
DEFINE_NORMAL_ANG_FUNCS(22)
DEFINE_NORMAL_ANG_FUNCS(23)
DEFINE_NORMAL_ANG_FUNCS(24)
DEFINE_NORMAL_ANG_FUNCS(25)
DEFINE_NORMAL_ANG_FUNCS(26)
DEFINE_NORMAL_ANG_FUNCS(27)
DEFINE_NORMAL_ANG_FUNCS(28)
DEFINE_NORMAL_ANG_FUNCS(29)
DEFINE_NORMAL_ANG_FUNCS(30)
DEFINE_NORMAL_ANG_FUNCS(31)
DEFINE_NORMAL_ANG_FUNCS(32)
DEFINE_NORMAL_ANG_FUNCS(33)
DEFINE_NORMAL_ANG_FUNCS(34)
DEFINE_NORMAL_ANG_FUNCS(35)

#undef DEFINE_SINGLE_ANG_FUNC
#undef DEFINE_NORMAL_ANG_FUNCS

// Edge Intra Pred Functions
// Mode 1: DC Edge Functions
#define DEFINE_DC_EDGE_1_FUNC(ss) \
void lent_intra_pred_dc_edge_1_##ss##x##ss##_c(pixel *dst, int dst_stride, const pixel *ref_pix) \
{ \
    int x, y, i; \
	const int i_size = ss; \
	const pixel* above = ref_pix + 1; \
	const pixel* left = ref_pix + (2 * i_size + 1); \
	int i_dc_val = i_size; \
	for (i = 0; i < i_size; i++) \
		i_dc_val += ref_pix[1 + i] + ref_pix[2 * i_size + 1 + i]; \
	i_dc_val /= (i_size << 1); \
	for (y = 0; y < i_size; y++) \
		for (x = 0; x < i_size; x++) \
			dst[y * dst_stride + x] = (pixel)i_dc_val; \
    dst[0] = (pixel)((above[0] + left[0] + 2 * dst[0] + 2) >> 2); \
	for (x = 1; x < i_size; x++) \
		dst[x] = (pixel)((above[x] + 3 * dst[x] + 2) >> 2); \
	dst += dst_stride; \
	for (y = 1; y < i_size; y++) \
	{ \
		*dst = (pixel)((left[y] + 3 * *dst + 2) >> 2); \
		dst += dst_stride; \
	} \
}
#define DEFINE_DC_EDGE_1_FUNCS \
	DEFINE_DC_EDGE_1_FUNC(4) \
	DEFINE_DC_EDGE_1_FUNC(8) \
	DEFINE_DC_EDGE_1_FUNC(16)
	DEFINE_DC_EDGE_1_FUNCS

	// Mode 10, 26: Horizontal/Vertical Edge Functions
#define DEFINE_SINGLE_ANG_FUNC(m, s) \
void lent_intra_pred_ang_edge_##m##_##s##x##s##_c(pixel *dst, int dst_stride, const pixel *ref_pix) { \
	lent_intra_pred_ang_c(dst, dst_stride, ref_pix, m, s, 1); \
}

#define DEFINE_EDGE_ANG_FUNCS(m) \
	DEFINE_SINGLE_ANG_FUNC(m, 4) \
	DEFINE_SINGLE_ANG_FUNC(m, 8) \
	DEFINE_SINGLE_ANG_FUNC(m, 16)
	DEFINE_EDGE_ANG_FUNCS(10)
	DEFINE_EDGE_ANG_FUNCS(26)
#undef DEFINE_SINGLE_ANG_FUNC
#undef DEFINE_EDGE_ANG_FUNCS

void init_pred_funcs(predict_function_t *pf) {
	pf->intra_normal_filter[0] = lent_intra_normal_filter_4x4_c;
	pf->intra_normal_filter[1] = lent_intra_normal_filter_8x8_c;
	pf->intra_normal_filter[2] = lent_intra_normal_filter_16x16_c;
	pf->intra_normal_filter[3] = lent_intra_normal_filter_32x32_c;

	pf->intra_pred[0][0] = lent_intra_pred_planar_0_4x4_c;
	pf->intra_pred[0][1] = lent_intra_pred_planar_0_8x8_c;
	pf->intra_pred[0][2] = lent_intra_pred_planar_0_16x16_c;
	pf->intra_pred[0][3] = lent_intra_pred_planar_0_32x32_c;
	pf->intra_pred[0][4] = lent_intra_pred_planar_0_4x4_c;
	pf->intra_pred[0][5] = lent_intra_pred_planar_0_8x8_c;
	pf->intra_pred[0][6] = lent_intra_pred_planar_0_16x16_c;
	pf->intra_pred[0][7] = lent_intra_pred_planar_0_32x32_c;

	pf->intra_pred[1][0] = lent_intra_pred_dc_1_4x4_c;
	pf->intra_pred[1][1] = lent_intra_pred_dc_1_8x8_c;
	pf->intra_pred[1][2] = lent_intra_pred_dc_1_16x16_c;
	pf->intra_pred[1][3] = lent_intra_pred_dc_1_32x32_c;
	pf->intra_pred[1][4] = lent_intra_pred_dc_edge_1_4x4_c;
	pf->intra_pred[1][5] = lent_intra_pred_dc_edge_1_8x8_c;
	pf->intra_pred[1][6] = lent_intra_pred_dc_edge_1_16x16_c;
	pf->intra_pred[1][7] = lent_intra_pred_dc_1_32x32_c;

#define DECLAIR_NORMAL_C_ANG_PRED_FUNCS(mode) \
		pf->intra_pred[mode][0] = lent_intra_pred_ang_##mode##_4x4_c; \
		pf->intra_pred[mode][1] = lent_intra_pred_ang_##mode##_8x8_c; \
		pf->intra_pred[mode][2] = lent_intra_pred_ang_##mode##_16x16_c; \
		pf->intra_pred[mode][3] = lent_intra_pred_ang_##mode##_32x32_c; \
		pf->intra_pred[mode][4] = lent_intra_pred_ang_##mode##_4x4_c; \
		pf->intra_pred[mode][5] = lent_intra_pred_ang_##mode##_8x8_c; \
		pf->intra_pred[mode][6] = lent_intra_pred_ang_##mode##_16x16_c; \
		pf->intra_pred[mode][7] = lent_intra_pred_ang_##mode##_32x32_c

	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(2);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(3);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(4);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(5);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(6);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(7);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(8);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(9);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(10);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(11);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(12);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(13);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(14);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(15);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(16);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(17);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(18);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(19);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(20);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(21);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(22);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(23);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(24);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(25);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(26);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(27);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(28);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(29);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(30);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(31);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(32);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(33);
	DECLAIR_NORMAL_C_ANG_PRED_FUNCS(34);
#undef DECLAIR_NORMAL_C_ANG_PRED_FUNCS

	pf->intra_pred[10][4] = lent_intra_pred_ang_edge_10_4x4_c;
	pf->intra_pred[10][5] = lent_intra_pred_ang_edge_10_8x8_c;
	pf->intra_pred[10][6] = lent_intra_pred_ang_edge_10_16x16_c;

	pf->intra_pred[26][4] = lent_intra_pred_ang_edge_26_4x4_c;
	pf->intra_pred[26][5] = lent_intra_pred_ang_edge_26_8x8_c;
	pf->intra_pred[26][6] = lent_intra_pred_ang_edge_26_16x16_c;
}

