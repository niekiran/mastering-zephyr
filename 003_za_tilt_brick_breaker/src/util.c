#include <stdint.h>
#include <stdbool.h>

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((uint16_t)(r & 0xF8) << 8) |
	       ((uint16_t)(g & 0xFC) << 3) |
	       ((uint16_t)b >> 3);
}

int clamp_i(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

int isqrt_i(int n)
{
	if (n <= 0) return 0;
	int x = n;
	int y = (x + 1) / 2;
	while (y < x) {
		x = y;
		y = (x + n / x) / 2;
	}
	return x;
}

bool point_in_circle(int x, int y, int cx, int cy, int r)
{
	int dx = x - cx;
	int dy = y - cy;
	return (dx * dx + dy * dy) <= (r * r);
}
