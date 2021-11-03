#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

#include "BBox.h"
#define eps 0.000000001f
struct HitRecord
{
	//HitRecord() {}
	vec3 n;
	float t;
	float u;
	float v;
	int ti;
	float r;//roughness, if any :)
};

struct Triangle
{
	
	Triangle() {}

	//Triangle(vec3 p0, vec3 p1, vec3 p2, int vt0_, int vt1_, int vt2_, int m) : p0(p0), e1(p1 - p0), e2(p2 - p0), vt0(vt0_), vt1(vt1_), vt2(vt2_), n(e1.cross(e2).norm()), mtl(m) {}

	
	Triangle(vec3 p0, vec3 p1, vec3 p2, int vt0_, int vt1_, int vt2_, int vn0_, int vn1_, int vn2_, int m) : p0(p0), e1(p1 - p0), e2(p2 - p0), vt0(vt0_), vt1(vt1_), vt2(vt2_) //{} n(((p1 - p0).cross(p2 - p0)).norm()), mtl(m)
	{
		//(vn0_), vn1(vn1_), vn2(vn2_),

		mtl = m;

		if (vn0_ >= 0)
		{
			vn[0] = vn0_;
			vn[1] = vn1_;
			vn[2] = vn2_;
			compute_normal = true;
		}
		else
		{
			vn[0] = -1;
			vn[1] = -1;
			vn[2] = -1;
			n = (((p1 - p0).cross(p2 - p0)).norm());
			compute_normal = false;
		}
	}


	bool __fastcall optimize_2(const Ray& r, HitRecord& rec, float* mint, unsigned int& ti)
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

	//bool __fastcall hit_anything(const Ray& r, const float& d) const
	bool __fastcall hit_anything(const Ray& r) const//, const float& d) const
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

	bool __fastcall hit_anything_alpha(const Ray& r, HitRecord& rec) const//, const float& d) const
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

	/*
	bool __fastcall hit_anything_range(const Ray& r, const float& d) const
	{
		vec3 pvec(r.d.cross(e2));
		float det = pvec.dot(e1);
		if (det > -0.000002f && det < 0.000002f)
			return false;
		float inv_det = 1.0f / det;

		vec3 tvec(r.o - p0);
		vec3 qvec(tvec.cross(e1));



		float u = tvec.dot(pvec) * inv_det;
		//if (u < 0.0f || u > 1.0f)
		if (u <= 0.000001f || u >= 1.0f)
			return false;


		float v = qvec.dot(r.d) * inv_det;
		if (v <= 0.000001f || u + v >= 1.0f)
			return false;

		//float t = e2.dot(qvec) * inv_det;
		//return t <= d && t >= 0.0001f;

		float t = e2.dot(qvec) * inv_det;
		//if (t > d || t < 0.00001f)
		//	return false;
		//return true;

		return t <= d && t >= 0.00002f;

	}
	*/
	vec3 e1;
	vec3 e2;
	vec3 p0;
	//int vt[3];
	
	uint16_t mtl;
	bool compute_normal = false;
	//bool see_light = false;

	int vt0;
	int vt1;
	int vt2;
	
	union
	{
		/*struct 
		{
			int vn0;
			int vn1;
			int vn2;
		};*/

		int vn[3] = { -1, -1, -1 };
		vec3 n;
	};
};

#endif // !_TRIANGLE_H_

