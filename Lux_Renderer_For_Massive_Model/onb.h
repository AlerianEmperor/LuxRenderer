#ifndef _ONB_H_
#define _ONB_H_

#include "Ray.h"


struct onb
{
	vec3 u;
	vec3 v;
	vec3 w;
	onb() {}
	onb(const vec3& n) : w(n)
	{	
		if (n.z < -0.9999999f) // Handle the singularity
		{
			u = vec3(0.0f, -1.0f, 0.0f);
			v = vec3(-1.0f, 0.0f, 0.0f);
			return;
		}
		else
		{
			const float a = 1.0f / (1.0f + n.z);
			const float b = -n.x * n.y * a;
			u = vec3(1.0f - n.x * n.x * a, b, -n.x);
			v = vec3(b, 1.0f - n.y * n.y * a, -n.y);
		}

		/*
		if (n.z >= -0.9999999f) // Handle the singularity
		{
			const float a = 1.0f / (1.0f + n.z);
			const float b = -n.x * n.y * a;
			u = vec3(1.0f - n.x * n.x * a, b, -n.x);
			v = vec3(b, 1.0f - n.y * n.y * a, -n.y);
			return;
		}
		//else
		//{
			u = vec3(0.0f, -1.0f, 0.0f);
			v = vec3(-1.0f, 0.0f, 0.0f);
			return;
		//}
		*/
	}
};

#endif // !_ONB_H_
