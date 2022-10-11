#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include "BBox.h"
#include "Rnd.h"
#include <math.h>

using namespace std;

struct MediumInteraction
{
	vec3 p;

	float tMax = 0.0f;

	bool isValid = false;

	MediumInteraction() {}
};

struct Medium
{
	//vary per wavelength instead of constan
	vec3 sigma_a = vec3(0.5f, 0.6f, -0.7f);
	vec3 sigma_s = vec3(-0.2f, 0.2f, 0.1f);
	vec3 sigma_t;

	vec3 medium_color = vec3(1.0f, 1.0f, 1.0f);

	float max_Density = 0.6f;

	float inv_max_Density;

	BBox bound;

	//vec3 extension;

	vec3 inv_extension;

	Medium()
	{

		sigma_t = sigma_a + sigma_s;
		medium_color = sigma_s / sigma_t;

		inv_max_Density = 1.0f / max_Density;

		vec3 extension = bound.bbox[1] - bound.bbox[0];

		inv_extension = vec3(1.0f / extension.x, 1.0f / extension.y, 1.0f / extension.z);
	}

	float getDensity(vec3& p)
	{
		return max_Density * expf(-2.0f * maxf(0.0f, (p.z - bound.bbox[0].z) * inv_extension.z));
	}


	vec3 Homogeneous_Transmission(vec3& start, vec3& end)
	{
		float d = (end - start).length();

		return vec3(expf(-sigma_t.x * d), expf(-sigma_t.y * d), expf(-sigma_t.z * d));
	}

	vec3 Heterogeneous_Transimission(Ray& r, MediumInteraction& mi)
	{
		float t = 1e-4;

		vec3 T(1.0f);

		while (true)
		{
			t -= logf(1.0f - randf()) * inv_max_Density / sigma_t.maxc();

			if (t >= mi.tMax)
				break;

			vec3 p = r.o + r.d * t;

			float density = getDensity(p);

			T *= 1.0f - maxf(0.0f, density * inv_max_Density);
		}

		return T;
	}

	vec3 Heterogeneous_Sample(Ray& r, MediumInteraction& mi)
	{
		float t = 1e-4;

		while (true)
		{
			t -= logf(1.0f - randf()) * inv_max_Density / sigma_t.maxc();

			if (t >= mi.tMax)
			{
				mi.isValid = false;
				break;
			}

			vec3 p = r.o + r.d * t;
			
			float density = getDensity(p);

			if (density * inv_max_Density > randf())
			{
				mi.isValid = true;
				mi.p = p;
				return medium_color * density;
			}
		}

		return vec3(1.0f);
	}

};

#endif // !_MEDIUM_H_

