#ifndef _RAY_H_
#define _RAY_H_
#include "vec3.h"

/*static float minf_for_ray(const float& a, const float& b)
{
	return a < b ? a : b;
}*/

struct Ray
{
	vec3 o;
	vec3 d;
	vec3 invd;
	vec3 roinvd;
	//float min_ray;
	int sign[3];
	//int sign_box[3];
	Ray() {}
	Ray(vec3 o_, vec3 d_) : o(o_), d(d_), invd(1.0f / d.x, 1.0f / d.y, 1.0f / d.z), roinvd(-o.x / d.x, -o.y / d.y, -o.z / d.z)
	{
		sign[0] = d.x > 0.0f, sign[1] = d.y > 0.0f, sign[2] = d.z > 0.0f;

		//sign_box[0] = d.x > 0.0f ? 1 : -1;
		//sign_box[1] = d.y > 0.0f ? 1 : -1;
		//sign_box[2] = d.z > 0.0f ? 1 : -1;

		//min_ray = minf_for_ray(o.x, minf_for_ray(o.y, o.z));

	}
	/*Ray(vec3 o_, vec3 d_) : o(o_), d(d_), invd(1 / d.x, 1 / d.y, 1 / d.z)
	{
		sign[0] = d.x > 0.0f, sign[1] = d.y > 0.0f, sign[2] = d.z > 0.0f;
	}*/
};

#endif // !_RAY_H_

