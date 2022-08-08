#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "vec3.h"
#include <vector>

using namespace std;
struct Geometry
{
	Geometry() {}
	vector<vec3> v;
	vector<vec3> vt;
	vector<vec3> vn;
};

struct Face
{
	Face() {}
	Face(int v0, int vt0, int v1, int vt1, int v2, int vt2)
	{
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;

		vt[0] = vt0;
		vt[1] = vt1;
		vt[2] = vt2;

		vn[0] = -1;
		vn[1] = -1;
		vn[2] = -1;
	}

	Face(int v0, int vt0, int vn0, int v1, int vt1, int vn1, int v2, int vt2, int vn2)
	{
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;

		vt[0] = vt0;
		vt[1] = vt1;
		vt[2] = vt2;

		vn[0] = vn0;
		vn[1] = vn1;
		vn[2] = vn2;
	}

	int v[3]  = { -1 };
	int vt[3] = { -1 };
	int vn[3] = { -1 };
};

#endif // !_GEOMETRY_H_
