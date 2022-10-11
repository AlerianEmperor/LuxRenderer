#ifndef _TRIANGLE_16_H_
#define _TRIANGLE_16_H_

#include "Ray.h"

struct HitRecord
{
	vec3 n;
	float t;
	float u;
	float v;
	int ti;
};

//max diff v vn 791565
struct Index_List
{
	int v;
	int vt;
	int vn;
	Index_List() {}
	Index_List(int v_, int vt_, int vn_) : v(v_), vt(vt_), vn(vn_) {}
};

//24 bytes
//23 x 3
//19 

//12 bytes actually
struct Triangle_16
{
	//96 bits
	int ind_v0 : 27;// : 27;
	int ind_v1 : 27;// : 27;
	int ind_v2 : 27;// : 27;
	int mtl    : 15;// : 15;

	Triangle_16() {}
	Triangle_16(int ind_v0_, int ind_v1_, int ind_v2_, int mtl_) : ind_v0(ind_v0_), ind_v1(ind_v1_), ind_v2(ind_v2_), mtl(mtl_) {}
};


bool __fastcall hit_triangle(const Ray& r, vec3& p0, vec3& e1, vec3& e2, unsigned int& ti, float* mint, HitRecord& rec)
{

	vec3 qvec;
	vec3 pvec(r.d.cross(e2));

	float det = e1.dot(pvec);//double

	vec3 tvec(r.o - p0);

	float inv_det = 1.0f / det;//double

	float u, v;//double

	if (det > eps)
	{

		u = tvec.dot(pvec);

		//compare with int slower
		if (u <= 0.0f || u >= det)//if (u <= 0.0 || u >= det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		//compare with int 36.701s
		if (v <= 0.0f || u + v >= det)//if (v <= 0.0f || u + v >= det)
			return false;
	}
	else if (det < -eps)
	{
		u = tvec.dot(pvec);

		if (u >= 0.0f || u <= det)//if (u >= 0.0f || u <= det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		if (v >= 0.0 || u + v <= det)//if (v >= 0.0 || u + v <= det)
			return false;
	}
	else
		return false;

	float t = e2.dot(qvec) * inv_det;
	if (t >= *mint || t <= eps)
		return false;

	*mint = t;
	rec.t = t;
	rec.u = u * inv_det;
	rec.v = v * inv_det;
	//rec.n = n;
	rec.ti = ti;

	return true;
}

bool __fastcall hit_anything(const Ray& r, vec3& p0, vec3& e1, vec3& e2)
{
	//1 base

	vec3 pvec(r.d.cross(e2));

	float det = e1.dot(pvec);

	vec3 qvec;
	float u, v;

	if (det > eps)
	{
		vec3 tvec(r.o - p0);

		u = tvec.dot(pvec);

		if (u < 0.0f || u > det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		if (v < 0.0f || u + v > det)
			return false;
	}
	else if (det < -eps)
	{
		vec3 tvec(r.o - p0);

		u = tvec.dot(pvec);

		if (u > 0.0f || u < det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		if (v > 0.0f || u + v < det)
			return false;
	}
	else
		return false;

	float inv_det = 1.0f / det;

	float t = e2.dot(qvec) * inv_det;

	return t >= eps;
	//return t < d && t >= eps;
}

bool __fastcall hit_triangle_alpha(const Ray& r, vec3& p0, vec3& e1, vec3& e2, HitRecord& rec) //, const float& d) const
{
	//1 base

	vec3 pvec(r.d.cross(e2));

	float det = e1.dot(pvec);

	vec3 qvec;
	float u, v;

	if (det > eps)
	{
		vec3 tvec(r.o - p0);

		u = tvec.dot(pvec);

		if (u < 0.0f || u > det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		if (v < 0.0f || u + v > det)
			return false;
	}
	else if (det < -eps)
	{
		vec3 tvec(r.o - p0);

		u = tvec.dot(pvec);

		if (u > 0.0f || u < det)
			return false;

		qvec = tvec.cross(e1);

		v = r.d.dot(qvec);

		if (v > 0.0f || u + v < det)
			return false;
	}
	else
		return false;

	float inv_det = 1.0f / det;

	float t = e2.dot(qvec) * inv_det;

	//return t >= eps;

	if (t < eps)
		return false;

	rec.u = u;
	rec.v = v;
	rec.t = t;

	return true;

	//return t < d && t >= eps;
}


#endif
