#ifndef _FRESNEL_H_
#define _FRESNEL_H_
#include "vec3.h"

//Notice DielectricReflectance can be use for glass, however it is mostly used for other glossy method material that dont flip normal
//Fresnel Dielectric is only used for glass 

//Dielectric reflectance is use for smooth glass
//Rough Glass use the same version but N (macro normal) is replaced by M (micro normal)
float Dielectric_Reflectance(float& eta, float& cos_i, float& cos_t)
{
	//float eta = eta_;
	//float cos_i = cos_i_;

	/*if (cos_i > 0.0f)
	{
		eta = 1.0f / eta;
		cos_i = -cos_i;
	}*/

	if (cos_i < 0.0f)
	{
		eta = 1.0f / eta;
		cos_i = -cos_i;
	}

	float sinTheta2 = eta * eta * (1.0f - cos_i * cos_i);

	if (sinTheta2 > 1.0f)
	{
		cos_t = 0.0f;
		return 1.0f;
	}

	cos_t = sqrt14(max(1.0f - sinTheta2, 0.0f));

	float Rs = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
	float Rp = (eta * cos_t - cos_i) / (eta * cos_t + cos_i);

	return (Rs * Rs + Rp * Rp) * 0.5f;
}

//Dielectric_Reflectance_Original  version
/*
float Dielectric_Reflectance(const float& eta_, const float& cos_i_)
{
	float eta = eta_;
	float cos_i = cos_i_;

	if (cos_i < 0.0f)
	{
	eta = 1.0f / eta;
	cos_i = -cos_i;
	}

	float sinTheta2 = eta * eta * (1.0f - cos_i * cos_i);

	float cos_t;

	if (sinTheta2 > 1.0f)
	{
		cos_t = 0.0f;
		return 1.0f;
	}

	cos_t = sqrt14(max(1.0f - sinTheta2, 0.0f));

	float Rs = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
	float Rp = (eta * cos_t - cos_i) / (eta * cos_t + cos_i);

	return (Rs * Rs + Rp * Rp) * 0.5f;
}
*/

float Dielectric_Reflectance(const float& eta_, const float& cos_i_)
{
	float eta = eta_;
	float cos_i = cos_i_;

	/*if (cos_i < 0.0f)
	{
		eta = 1.0f / eta;
		cos_i = -cos_i;
	}*/

	float sinTheta2 = eta * eta * (1.0f - cos_i * cos_i);

	float cos_t;

	if (sinTheta2 > 1.0f)
	{
		cos_t = 0.0f;
		return 1.0f;
	}

	cos_t = sqrt14(max(1.0f - sinTheta2, 0.0f));

	float Rs = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
	float Rp = (eta * cos_t - cos_i) / (eta * cos_t + cos_i);

	return (Rs * Rs + Rp * Rp) * 0.5f;
}

/*
float Dielectric_Reflectance(const float& eta, const float& cos_i)
{
	float cos_t;
	return Dielectric_Reflectance(eta, cos_i, cos_t);
}
*/
/*
float Fresnel_Dielectric(const vec3& direction_in, const vec3& m, const float& eta)
{
	//float result = 1.0f;

	float cos_i = abs(direction_in.dot(m));

	float sin_o2 = (eta * eta) * (1.0f - cos_i * cos_i);

	if (sin_o2 <= 1.0f)
	{
		float cos_o = sqrt14(1.0f - sin_o2);
		float Rs = (cos_i - eta * cos_o) / (cos_i + eta * cos_o);
		float Rp = (eta * cos_i - cos_o) / (eta * cos_i + cos_o);

		return 0.5f * (Rs * Rs + Rp * Rp);
	}

	return 1.0f;
	//return result;
}
*/
/*
float Conductor_Reflectance(const float& eta, const float& k, const float& cos_i)
{
	float cos_i2 = cos_i * cos_i;
	float sin_i2 = 1.0f - cos_i2;//max(1.0f - cos_i2, 0.0f);
	float sin_i4 = sin_i2 * sin_i2;

	float event_horizon = eta * eta - k * k - sin_i2;

	float a2_plus_b2 = sqrt14(max(event_horizon * event_horizon + 4.0f * eta * eta * k * k, 0.0f));

	float a = sqrt14(max((a2_plus_b2 + event_horizon) * 0.5f, 0.0f));



	float Rs = ((a2_plus_b2 + cos_i2) - 2.0f * a * cos_i) / (a2_plus_b2 + cos_i2 + 2.0f * a * cos_i);

	float Rp = (cos_i2 * a2_plus_b2 + sin_i4) - (2.0f *  a * cos_i * sin_i2) / ((cos_i2 * a2_plus_b2 + sin_i4) + (2.0f * a * cos_i * sin_i2));

	//return 0.5f * (Rs + Rs * Rp);

	return 0.5f * (Rs * Rs + Rp * Rp);
}
*/

static float Conductor_Reflectance(const float& eta, const float& k, const float& cos_i)
{
	float cos_i2 = cos_i * cos_i;

	float sin_i2 = 1.0f - cos_i2;

	float sin_i4 = sin_i2 * sin_i2;

	float under_sqr = eta * eta - k * k - sin_i2;

	float a2_b2 = sqrt14(under_sqr * under_sqr + 4.0f * eta * eta * k * k);

	float a = sqrt14(max((a2_b2 + under_sqr) * 0.5f, 0.0f));

	float Rs = ((a2_b2 + cos_i2) - 2.0f * a * cos_i) / ((a2_b2 + cos_i2) + 2.0f * a * cos_i);

	//float Rp = (a2_b2 * cos_i2 - 2.0f * a * sin_i2 * cos_i + sin_i4) / (a2_b2 * cos_i2 + 2.0f * a * sin_i2 * cos_i + sin_i4);

	float Rp = ((a2_b2 * cos_i2 + sin_i4) - 2.0f * a * sin_i2 * cos_i) / ((a2_b2 * cos_i2 + sin_i4) + 2.0f * a * sin_i2 * cos_i);

	//original formula
	//return (Rs + Rs * Rp) * 0.5f;
	
	//positive formula
	return (Rs * Rs + Rp * Rp) * 0.5f;
}

vec3 Conductor_Reflectance_RGB(const vec3& eta, const vec3& k, const float& cos_i)
{
	//float cos_i_ = max(min(cos_i, 1.0f), -1.0f);

	float x = Conductor_Reflectance(eta.x, k.x, cos_i);
	float y = Conductor_Reflectance(eta.y, k.y, cos_i);
	float z = Conductor_Reflectance(eta.z, k.z, cos_i);

	return vec3(x, y, z);
}

float Compute_Diffuse_Fresnel(const float& ior, const int& sampleCount)
{
	float diffuseFresnel = 0.0f;
	float b = Dielectric_Reflectance(ior, 0.0f);

	for (int i = 1; i <= sampleCount; ++i)
	{
		float costheta2 = float(i) / sampleCount;

		float a = Dielectric_Reflectance(ior, min(sqrt14(costheta2), 1.0f));

		diffuseFresnel += (a + b) * (0.5f / sampleCount);
		b = a;
	}

	return diffuseFresnel;
}


float ThinFilmReflectance(const float& eta, const float& cos_i, float& cos_t)
{
	float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);

	if (sin_t2 > 1.0f)
	{
		cos_t = 0.0f;
		return 1.0f;
	}

	cos_t = sqrt14(max(1.0f - sin_t2, 0.0f));

	float Rs = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
	float Rp = (eta * cos_t - cos_i) / (eta * cos_t + cos_i);

	float Rs2 = Rs * Rs;
	float Rp2 = Rp * Rp;

	return 1.0f - ((1.0f - Rs2) / (1.0f + Rs2) + (1.0f - Rp2) / (1.0f + Rp2)) * 0.5f;
}

vec3 ThinFilmReflectanceInteference(const float& eta, const float& cos_i, const float& thickness, float& cos_t)
{
	float cos_i2 = cos_i * cos_i;

	float sin_i2 = 1.0f - cos_i2;

	float sin_t2 = eta * eta * sin_i2;

	if (sin_t2 > 1.0f)
	{
		cos_t = 0.0f;
		return vec3(1.0f);
	}

	cos_t = sqrt14(1.0f - sin_t2);

	float Ts = 4.0f * eta * cos_i * cos_t / sqr(eta * cos_i + cos_t);
	float Tp = 4.0f * eta * cos_i * cos_t / sqr(eta * cos_t + cos_i);

	float Rs = 1.0f - Ts;
	float Rp = 1.0f - Tp;

	vec3 inv_lambda(1.0f / 650.0f, 1.0f / 510.0f, 475.0f);
	float inv_eta = 1.0f / eta;
	
	vec3 phi(thickness * cos_t * 4.0f * pi * inv_eta * inv_lambda);

	vec3 cos_phi(FTA::cos(phi.x), FTA::cos(phi.y), FTA::cos(phi.z));

	vec3 tS = Ts * Ts / ((Rs * Rs + 1.0f) - 2.0f * Rs * cos_phi);
	vec3 tP = Tp * Tp / ((Rp * Rp + 1.0f) - 2.0f * Rp * cos_phi);

	return vec3(1.0f) - (tS + tP) * 0.5f;
}



#endif // !_FRESNEL_H_

