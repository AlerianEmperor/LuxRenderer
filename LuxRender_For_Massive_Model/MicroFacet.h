#ifndef _MICRO_FACET_H_
#define _MICRO_FACET_H_

#include "onb.h"
enum Microfacet_Distribution
{
	Beckmann,
	Phong,
	GGX
};


float Roughness_To_Alpha(const Microfacet_Distribution& dist, const float& roughness)
{
	float minAlpha = 0.001f;
	float roughness_value = max(roughness, minAlpha);

	if (dist == Phong)
		return 2.0f / (roughness_value * roughness) - 2.0f;
	else
		return roughness_value;
}

// m = micro surface normal
// n = macro surface normal

float Microfacet_D(const Microfacet_Distribution& dist, const vec3& m, const vec3& n, const float& alpha)
{
	float cos_theta = m.dot(n);

	if (cos_theta <= 0.0f)
		return 0.0f;

	switch (dist)
	{
		case GGX:
		{
			/*float alpha2 = alpha * alpha;
			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta2 = max(1.0f - cos_theta2, 0.0f) / cos_theta2;
			float cos_theta4 = cos_theta2 * cos_theta2;

			return alpha2 * ipi / (cos_theta4 * sqr(alpha2 + tan_theta2));*/

			float cos_theta2 = cos_theta * cos_theta;

			float denom = cos_theta2 * (alpha * alpha - 1.0f) + 1.0f;

			return alpha / (pi * denom * denom);

			//return 1.0f;
		}
		case Beckmann:
		{
			float alpha2 = alpha * alpha;
			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta2 = (1.0f - cos_theta2) / cos_theta2;//maxf(1.0f - cos_theta2, 0.0f) / cos_theta2;
			float cos_theta4 = cos_theta2 * cos_theta2;

			return expf(-tan_theta2 / alpha2) / (pi * alpha2 * cos_theta4);
		}
		case Phong:	
		{
			float specular_component = powf(cos_theta, alpha);
			return (alpha + 2.0f) * specular_component * itau;
		}
		
	}
	return 0.0f;
}

float G1(const Microfacet_Distribution& dist, const vec3& v, const vec3& m, const vec3& n, const float& alpha)
{
	float cos_theta = -v.dot(n);
	float cos_h = -v.dot(m);

	//if (cos_theta <= 0.0f || cos_h <= 0.0f)
	//	return 1.0f;

	if (cos_theta * cos_h <= 0.0f)
		return 0.0f;

	switch (dist)
	{
		case GGX:
		{
			/*float alpha2 = alpha * alpha;
			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta2 = max(1.0f - cos_theta2, 0.0f) / cos_theta2;

			return 2.0f / (1.0f + sqrt14(1.0f + alpha2 * tan_theta2));*/

			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta2 = max(1.0f - cos_theta2, 0.0f) / cos_theta2;

			if (tan_theta2 == 0.0f)
				return 1.0f;

			float alpha2 = alpha * alpha;

			return 2.0f / (1.0f + sqrt14(1.0f + alpha2 * tan_theta2));
		}
		case Beckmann:
		{
			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta = abs(sqrt14(max(1.0f - cos_theta2, 0.0f)) / cos_theta);
			float a = 1.0f / (alpha * tan_theta);

			if (a < 1.6f)
				return (3.535f * a + 2.181f * a * a) / (1.0f + 2.276f * a + 2.577f * a * a);
			else
				return 1.0f;
		}
		case Phong:
		{
			float cos_theta2 = cos_theta * cos_theta;
			float tan_theta = abs(sqrt14(max(1.0f - cos_theta2, 0.0f)) / cos_theta);
			float a = sqrt14(0.5f * alpha + 1.0f) / tan_theta;

			if(a < 1.6f)
				return (3.535f * a + 2.181f * a * a) / (1.0f + 2.276f * a + 2.577f * a * a);
			else
				return 1.0f;
		}
	}
	return 0.0f;
}

float Microfacet_G(const Microfacet_Distribution& dist, const vec3& v_in, const vec3& v_out, const vec3& m, const vec3& n, const float& alpha)
{
	return G1(dist, v_in, m, n, alpha) * G1(dist, v_out, m, n, alpha);
}

float Microfacet_pdf(const Microfacet_Distribution& dist, const vec3& m, const vec3& n, const float& alpha)
{
	float pdf = Microfacet_D(dist, m, n, alpha) * m.dot(n);

	if (pdf <= 0.0f)
		return 1.0f;

	return pdf;
}

vec3 Microfacet_Sample(const Microfacet_Distribution& dist, const vec3& n, const float& alpha)
{
	float u = randf();
	float v = randf();

	float phi = tau * v;
	float cos_theta;

	switch (dist)
	{
	case GGX:
	{
		//float tan_theta2 = alpha * alpha * u / (1.0f - u);
		//cos_theta = 1.0f / sqrt14(1.0f + tan_theta2);

		//float value = sqrt14(u / (1.0f - u));
		//float theta = atan(alpha * value);

		//cos_theta = cosf(theta);

		cos_theta = sqrt14((1.0f - u) / (u * (alpha * alpha - 1.0f) + 1.0f));

		//float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		break;
	}
		case Beckmann:
		{
			float tan_theta2 = -alpha * alpha * log10f(1.0f - u);
			cos_theta = 1.0f / sqrt14(1.0f + tan_theta2);

			break;
		}
		case Phong:
		{
			cos_theta = powf(u, 1.0f / (alpha + 2.0f));
			break;
		}
		
	}

	onb local(n);

	float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

	//https://gist.github.com/telegraphic/841212e8ab3252f5cffe
	//line 93 - 95
	//return vec3(cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta);

	float cos_phi, sin_phi;

	FTA::sincos(phi, &sin_phi, &cos_phi);

	vec3 h = (local.u * cos_phi + local.v * sin_phi) * sin_theta + n * cos_theta;

	return h.norm();

}

#endif // !_MICRO_FACET_H_

