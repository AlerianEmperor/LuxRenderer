#ifndef _TONE_unordered_mapPING_
#define _TONE_unordered_mapPING_
#include "vec3.h"


static float clamp(const float& x)
{
	return x < 0 ? 0 : x > 1 ? 1 : x;
}

static vec3 clamp_vector(const vec3& v)
{
	return vec3(clamp(v.x), clamp(v.y), clamp(v.z));
}

static vec3 F(const vec3& x)
{
	//version 2

	double A = 0.15;
	double B = 0.50;
	double C = 0.10;
	double D = 0.20;
	double E = 0.02;
	double F = 0.30;

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

static vec3 Filmmic_Toneunordered_mapping(const vec3& c)
{
	return clamp_vector(F(16 * c) / F(11.2));
}


static float F2(const float& x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

/*static float maxf(const float& a, const float& b)
{
	return a > b ? a : b;
}*/

vec3 max_vector(const vec3& v1, const vec3& v2)
{
	return vec3(maxf(v1.x, v2.x), maxf(v1.y, v2.y), maxf(v1.z, v2.z));
}

vec3 Filmic_unchart2(const vec3& v)
{
	vec3 x = max_vector(v - vec3(0.004f), vec3(0.0f));
	return(x * (6.2f * x + 0.5f)) / ( x * (6.2f * x + 1.7f) + 0.06f);
}

static vec3 ACES_Input_Mat(const vec3& c)
{
	float x = 0.59719f * c.x + 0.35458f * c.y + 0.04823f * c.z;
	float y = 0.07600f * c.x + 0.90834f * c.y + 0.01566f * c.z;
	float z = 0.02840f * c.x + 0.13383f * c.y + 0.83777f * c.z;

	return vec3(x, y, z);
}

static vec3 RRTAndODTFit(const vec3& c)
{
	vec3 a(c * (c + 0.0245786f) - 0.000090537f);
	vec3 b(c * (0.983729f * c + 0.4329510f) + 0.238081f);
	return a / b;
}

static vec3 ACESOutputMat(const vec3& c)
{
	float x =  1.60475f * c.x + -0.53108f * c.y + -0.07367f * c.z;
	float y = -0.10208f * c.x +  1.10813f * c.y + -0.00605f * c.z;
	float z = -0.00327f * c.x + -0.07276f * c.y +  1.0760f  * c.z;

	return vec3(x, y, z);
};

//ACES Tone unordered_mapping

vec3 ACES_Tone_unordered_mapping(const vec3& color)
{
	vec3 c(color);

	c = ACES_Input_Mat(c);

	c = RRTAndODTFit(c);

	c = ACESOutputMat(c);

	return c;
}

vec3 ACESFilm(const vec3& x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	//float x = min(max(0.0f, v.x), 1.0f);

	vec3 value = ((x*(a*x + b)) / (x*(c*x + d) + e));

	float u = min(max(0.0f, value.x), 1.0f);
	float v = min(max(0.0f, value.y), 1.0f);
	float w = min(max(0.0f, value.z), 1.0f);

	return vec3(u, v, w);
}

//https://64.github.io/toneunordered_mapping/

/*
float luminance(vec3 v)
{
	return v.dot(vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out)
{
	float l_in = luminance(c_in);
	return c_in * (l_out / l_in);
}

vec3 reinhard_extended_luminance(vec3 v, float max_white_l)
{
	float l_old = luminance(v);
	float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
	float l_new = numerator / (1.0f + l_old);
	return change_luminance(v, l_new);
}
*/
#endif // !_TONE_unordered_mapPING_

