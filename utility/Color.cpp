/*
	Color - simple color utilities

	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Color.h"

#include <algorithm>
#include <cmath>


void rgb2hsv(const Color3fv rgb, Color3fv hsv)
{
	float r = rgb[0];
	float g = rgb[1];
	float b = rgb[2];

	float h, s;

	float max = std::max(std::max(r, g), b);
	float min = std::min(std::min(r, g), b);
	float c = max - min;

	if (c == 0.0)
	{
		h = 0;
		s = 0;
	}
	else
	{
		if (max == r)
			h = fmod(((g - b) / c), 6);
		else if (max == g)
			h = (b - r) / c + 2;
		else
			h = (r - g) / c + 4;
		s = c / max;
	}

	hsv[0] = h * 60;
	hsv[1] = s;
	hsv[2] = max;
}


void hsv2rgb(const Color3fv hsv, Color3fv rgb)
{
	float h = hsv[0];
	float s = hsv[1];
	float v = hsv[2];

	float c = v * s;
	float x = c * (1.0 - std::abs(std::fmod(h / 60., 2) - 1));
	float m = v - c;

	if (h >= 0 && h < 60)
	{
		rgb[0] = c + m;
		rgb[1] = x + m;
		rgb[2] = m;
	}
	else if (h >= 60 && h < 120)
	{
		rgb[0] = x + m;
		rgb[1] = c + m;
		rgb[2] = m;
	}
	else if (h >= 120 && h < 180)
	{
		rgb[0] = m;
		rgb[1] = c + m;
		rgb[2] = x + m;
	}
	else if (h >= 180 && h < 240)
	{
		rgb[0] = m;
		rgb[1] = x + m;
		rgb[2] = c + m;
	}
	else if (h >= 240 && h < 300)
	{
		rgb[0] = x + m;
		rgb[1] = m;
		rgb[2] = c + m;
	}
	else if (h >= 300 && h < 360)
	{
		rgb[0] = c + m;
		rgb[1] = m;
		rgb[2] = x + m;
	}
	else
	{
		rgb[0] = m;
		rgb[1] = m;
		rgb[2] = m;
	}
}


void colorLerp(const Color3fv a, const Color3fv b, float v, Color3fv c)
{
	Color3fv hsv_a, hsv_b, hsv_c;

	if (v > 1)
		v = 1;
	else if (v < 0)
		v = 0;

	rgb2hsv(a, hsv_a);
	rgb2hsv(b, hsv_b);

	hsv_c[0] = hsv_a[0] + (hsv_b[0] - hsv_a[0]) * v;
	hsv_c[1] = hsv_a[1] + (hsv_b[1] - hsv_a[1]) * v;
	hsv_c[2] = hsv_a[2] + (hsv_b[2] - hsv_a[2]) * v;

	hsv2rgb(hsv_c, c);
}
