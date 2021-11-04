#ifndef _FILTER_H_
#define _FILTER_H_

#include <vector>
#include "vec2.h"
#include "vec3.h"

using namespace std;

//Implementation of 
//Edge-Avoiding À-TrousWavelet Transform for denoising
//https://www.shadertoy.com/view/ldKBzG

//All useful link
//https://www.shadertoy.com/view/ldKBzG
//https://www.shadertoy.com/view/ldKBRV
//https://www.shadertoy.com/view/MsXfz4

//post that led to these links
//https://github.com/hoverinc/ray-tracing-renderer/issues/2

struct vec2i
{
	int x;
	int y;

	vec2i() {};
	vec2i(int x_, int y_) : x(x_), y(y_) {}

	vec2i operator +(vec2i v) {return vec2i(x + v.x, y + v.y); }
	vec2i operator *(const float& v) { return vec2i(x * v, y * v); }
};

vec2i offset[25] = { 
					vec2i(-2, -2),
				    vec2i(-1, -2),
				    vec2i(0, -2),
				    vec2i(1, -2),
				    vec2i(2, -2),

				    vec2i(-2, -1),
				    vec2i(-1, -1),
				    vec2i(0, -1),
				    vec2i(1, -1),
				    vec2i(2, -1),

				    vec2i(-2, 0),
				    vec2i(-1, 0),
				    vec2i(0, 0),
				    vec2i(1, 0),
					vec2i(2, 0),

					vec2i(-2, 1),
					vec2i(-1, 1),
					vec2i(0, 1),
					vec2i(1, 1),
					vec2i(2, 1),

					vec2i(-2, 2),
					vec2i(-1, 2),
					vec2i(0, 2),
					vec2i(1, 2),
					vec2i(2, 2) };


float kernel[25] = {
					1.0f / 256.0f,
					1.0f / 64.0f,
					3.0f / 128.0f,
					1.0f / 64.0f,
					1.0f / 256.0f,

					1.0f / 64.0f,
					1.0f / 16.0f,
					3.0f / 32.0f,
					1.0f / 16.0f,
					1.0f / 64.0f,

					3.0f / 128.0f,
					3.0f / 32.0f,
					9.0f / 64.0f,
					3.0f / 32.0f,
					3.0f / 128.0f,

					1.0f / 64.0f,
					1.0f / 16.0f,
					3.0f / 32.0f,
					1.0f / 16.0f,
					1.0f / 64.0f,

					1.0f / 256.0f,
					1.0f / 64.0f,
					3.0f / 128.0f,
					1.0f / 64.0f,
					1.0f / 256.0f
};

bool isSafe(const vec2i& v, const int& width, const int& height)
{
	return v.x >= 0 && v.x <= width - 1 && v.y >= 0 && v.y <= height - 1;
}

const float Denoise_Strength = 0.5f;

vec3 Edge_Avoiding_A_TrouseWavelet(const vector<vec3>& base_color, const vector<vec3>& color, const vector<vec3>& normal, const int& i, const int& j, const int& width, const int& height)
{
	float c_phi = 1.0f;
	float n_phi = 0.5f;

	vec3 real_color = base_color[j * width + i];
	vec3 real_normal = normal[j * width + i];

	vec3 sum(0.0f);
	float accumulate_weight = 0.0f;

	for (int k = 0; k < 25; ++k)
	{
		vec2i uv = vec2i(i, j) + offset[k] * Denoise_Strength;

		if (isSafe(uv, width, height))
		{
			int index = uv.y * width + uv.x;
		
			vec3 current_color = color[index];
			vec3 color_diff = real_color - current_color;
			float color_dist2 = color_diff.dot(color_diff);
			float color_weight = min(exp(-color_dist2 / c_phi), 1.0f);


			vec3 current_normal = normal[index];
			vec3 normal_diff = real_normal - current_normal;
			float normal_dist2 = max(normal_diff.dot(normal_diff), 0.0f);
			float normal_weight = min(exp(-normal_dist2 / n_phi), 1.0f);

			float weight = color_weight * normal_weight;
			sum += current_color * weight * kernel[k];
			accumulate_weight += weight * kernel[k];
		}
	}

	return sum / accumulate_weight;
}

#endif // !_FILTER_H_

