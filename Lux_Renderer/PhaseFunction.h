#ifndef _PHASE_FUNCTION_H_
#define _PHASE_FUNCTION_H_

#include "vec3.h"
#include "Rnd.h"
#include "fasttrigo.h"

static const float Inv4Pi = 0.07957747154594766788;

//Phase Function : handle how incoming ray will interactive with the participating media
//Volume : handle the strength of the radiance that the ray carry when escpae the participating media

//In short
//Phase Function compute ray direction
//Medium compute Radiance


class PhaseFunction
{

};

float g = 1.2f;

//taken from pbrt 15.2
//Henyey GreenStein Definition

float HenyeyGreenStein(vec3& direction_in, vec3& direction_out)
{
	//compute cos_theta for Henyey GreenStein Sample
	float u = randf();
	float g2 = g * g;

	float cos_theta;

	if (abs(g) < 1e-3)
		cos_theta = 1.0f - 2.0f * u;
	else
	{
		float t = (1.0f - g2) / (1.0f - g + 2.0f * g * u);
		cos_theta = (1.0f + g2 - t * t) / (2.0f * g);
	}

	float s = 1.0f - cos_theta * cos_theta;

	float sin_theta = sqrt14(max(0.0f, s));

	float phi = 2.0f * pi * randf();

	vec3 v1, v2;

	//Coordinate system
	if (abs(direction_in.x) > abs(direction_in.y))
	{
		float inv_length = 1.0f / sqrt14(direction_in.x * direction_in.x + direction_in.z * direction_in.z);
		v1 = vec3(-direction_in.z * inv_length, 0, direction_in.x * inv_length);
	}
	else
	{
		float inv_length = 1.0f / sqrt14(direction_in.y * direction_in.y + direction_in.z * direction_in.z);
		v1 = vec3(0, direction_in.z * inv_length, -direction_in.y * inv_length);
	}

	v2 = direction_in.dot(v1);

	//Spherical Coordinate Direction
	direction_out = sin_theta * FTA::cos(phi) * v1 + sin_theta * FTA::sin(phi) * v2 + cos_theta * direction_in;

	//Phase HG
	float denom = 1.0f + g * g + 2.0f * g * cos_theta;

	return Inv4Pi * (1.0f - g * g) / (denom * sqrt14(denom));
}

#endif // !_PHASE_FUNCTION_H_

