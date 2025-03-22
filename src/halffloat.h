#ifndef __HALFFLOAT_H__
#define __HALFFLOAT_H__

static uint16_t float16(float f)
{
	uint32_t local = *((uint32_t * )&f);
	uint32_t exp = (local & 0x7f800000u);
	uint32_t out;
	/// non-sign part
	out  = (uint32_t)((local & 0x7fffffffu) >> 13);
	/// adjust bias
	out  -= 0x1c000u;
	/// clamb the borders
	out  = (exp < 0x38800000u)? 0 : out;
	out  = (exp > 0x8e000000u)? 0x7bff : out;
	out  = (exp == 0)? 0 : out;
	/// move signe
	out |= (uint32_t)((local & 0x80000000u) >> 16);
	return (uint16_t)out;
}

#endif
