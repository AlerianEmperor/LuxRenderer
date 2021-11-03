#ifndef _BBOX_H_
#define _BBOX_H_

#include "Ray.h"


static float maxf(const float& a, const float& b)
{
	return a > b ? a : b;
}

static float minf(const float& a, const float& b)
{
	return a < b ? a : b;
}
/*
static void minf2(float* a, float* b)
{
	a = a < b ? a : b;
}
*/

struct BBox
{
	vec3 bbox[2];
		
	BBox() { bbox[0] = { 1e10f, 1e10f, 1e10f }, bbox[1] = { -1e10f, -1e10f, -1e10f }; }
	BBox(vec3 bmin, vec3 bmax) { bbox[0] = bmin; bbox[1] = bmax; }

	/*bool __fastcall hit_axis_tl_mint(const Ray& r, float& tl, float& mint) const
	{
		
		float txmax[3];
		
		for (int i = 2; i >= 0; --i)
		{
			txmax[i] = bbox[r.sign[i]][i] * r.invd[i] + r.roinvd[i];
			
		}

		float th = minf(txmax[2], minf(txmax[1], txmax[0]));

		if (th < 0.0f)
			return false;		

		float txmin = bbox[1 - r.sign[0]].x * r.invd.x + r.roinvd.x;
		float tymin = bbox[1 - r.sign[1]].y * r.invd.y + r.roinvd.y;
		float tzmin = bbox[1 - r.sign[2]].z * r.invd.z + r.roinvd.z;

		tl = maxf(txmin, maxf(tymin, tzmin));

		//if (tl >= mint)
		//	return false;
		

		return tl <= 0.0f || tl <= th * 1.00000024f && tl <= mint;

		//return tl < mint || tl <= th * 1.00000024f;
	}
	*/
	bool __fastcall hit_axis_tl(const Ray& r, float& tl) const
	{	
		/*float txmax = bbox[r.sign[0]].x * r.invd.x + r.roinvd.x;
		
		float tymax = bbox[r.sign[1]].y * r.invd.y + r.roinvd.y;
	
		float tzmax = bbox[r.sign[2]].z * r.invd.z + r.roinvd.z;
		
		float th = minf(tzmax, minf(tymax, txmax));//30.61s
		*/

		//if (r.o.x * r.invd.x > bbox[r.sign[0]].x * r.invd.x)
		//	return false;

		/*if (!((r.o.x > bbox[r.sign[0]].x) ^ r.sign[0]) ||
			!((r.o.y > bbox[r.sign[1]].y) ^ r.sign[1]) ||
			!((r.o.z > bbox[r.sign[2]].z) ^ r.sign[2]))
				return false;
		*/

		/*if ((r.o.x * r.sign_box[0] > bbox[r.sign[0]].x * r.sign_box[0]) ||
			(r.o.y * r.sign_box[1] > bbox[r.sign[1]].y * r.sign_box[1]) ||
			(r.o.z * r.sign_box[2] > bbox[r.sign[2]].z * r.sign_box[2]))
				return false;
			*/
		float txmax[3];
		//#pragma omp parallel for schedule(guided)
		
		//changed original
		//for (int i = 0; i < 4; ++i)

		//float th = minf(txmax[2], minf(txmax[1], txmax[0]));

		//float th = 0.0f;
		//#pragma omp parallel for// schedule(guided)
		for (int i = 2; i >= 0; --i)
		{
			txmax[i] = bbox[r.sign[i]][i] * r.invd[i] + r.roinvd[i];
			//txmax[i] = bbox[r.sign[i % 3]][i % 3] * r.invd[i % 3] + r.roinvd[i % 3];
			//txmax[i - 1] = bbox[r.sign[(i - 1) % 3]][(i - 1) % 3] * r.invd[(i - 1) % 3] + r.roinvd[(i - 1) % 3];
			//txmax[i - 2] = bbox[r.sign[(i - 2) % 3]][(i - 2) % 3] * r.invd[(i - 2) % 3] + r.roinvd[(i - 2) % 3];
			//txmax[i - 3] = bbox[r.sign[(i - 3) % 3]][(i - 3) % 3] * r.invd[(i - 3) % 3] + r.roinvd[(i - 3) % 3];

			//th = minf(th, txmax[])
			//if (txmax[i] < 0.0f)
			//	return false;
		}
		
		//float th = minf(txmax[1], txmax[0]);
		float th = minf(txmax[2], minf(txmax[1], txmax[0]));

		//minf2(&txmax[2], &minf(txmax[1], txmax[0]));

		//minf2(&th, &txmax[2]);

		//compare with int slower
		if (th < 0.0f)
			return false;
		
		//float txmin[4];

		float txmin = bbox[1 - r.sign[0]].x * r.invd.x + r.roinvd.x;
		float tymin = bbox[1 - r.sign[1]].y * r.invd.y + r.roinvd.y;
		float tzmin = bbox[1 - r.sign[2]].z * r.invd.z + r.roinvd.z;

		tl = maxf(txmin, maxf(tymin, tzmin));

		/*for (int i = 3; i >= 0; --i)
		{
			txmin[i] = bbox[1 - r.sign[i % 3]][i % 3] * r.invd[i % 3] + r.roinvd[i % 3];
		}*/

		//tl = maxf(txmin[0], maxf(txmin[1], txmin[2]));

		/*float txmin[4];
		for (int i = 0; i < 4; ++i)
		{
			txmin[i] = bbox[1 - r.sign[i % 3]][i % 3] * r.invd[i % 3] + r.roinvd[i % 3];
		}

		tl = maxf(txmin[0], maxf(txmin[1], txmin[2]));
		*/

		//compare with int slower
		return tl <= 0.0f || tl <= th * 1.00000024f;
		
		
	}
	
	bool __fastcall hit_shadow_th(const Ray& r, float& th) const
	{
		float txmin = bbox[1 - r.sign[0]].x * r.invd.x + r.roinvd.x;
		float txmax = bbox[r.sign[0]].x * r.invd.x + r.roinvd.x;

		float tymin = bbox[1 - r.sign[1]].y * r.invd.y + r.roinvd.y;
		float tymax = bbox[r.sign[1]].y * r.invd.y + r.roinvd.y;

		float tzmin = bbox[1 - r.sign[2]].z * r.invd.z + r.roinvd.z;
		float tzmax = bbox[r.sign[2]].z * r.invd.z + r.roinvd.z;


		float tl = maxf(txmin, maxf(tymin, tzmin));

		th = minf(txmax, minf(tymax, tzmax));

		return tl <= th * 1.00000024f && th >= 0.0f;
		

		/*
		float txmax[3];
		
		for (int i = 2; i >= 0; --i)
		{
			txmax[i] = bbox[r.sign[i]][i] * r.invd[i] + r.roinvd[i];
			
		}

		th = minf(txmax[2], minf(txmax[1], txmax[0]));

		
		if (th < 0.0f)
			return false;

		
		float txmin = bbox[1 - r.sign[0]].x * r.invd.x + r.roinvd.x;
		float tymin = bbox[1 - r.sign[1]].y * r.invd.y + r.roinvd.y;
		float tzmin = bbox[1 - r.sign[2]].z * r.invd.z + r.roinvd.z;

		float tl = maxf(txmin, maxf(tymin, tzmin));

		//compare with int slower
		return tl <= 0.0f || tl <= th * 1.00000024f;
		*/
	}


	vec3 c() const { return (bbox[0] + bbox[1]) * 0.5f; }


	void expand(const BBox& box)
	{
		bbox[0] = vec3(min(bbox[0].x, box.bbox[0].x), min(bbox[0].y, box.bbox[0].y), min(bbox[0].z, box.bbox[0].z));
		bbox[1] = vec3(max(bbox[1].x, box.bbox[1].x), max(bbox[1].y, box.bbox[1].y), max(bbox[1].z, box.bbox[1].z));
	}

	BBox expand_box(const BBox& box)
	{
		vec3 v1(min(bbox[0].x, box.bbox[0].x), min(bbox[0].y, box.bbox[0].y), min(bbox[0].z, box.bbox[0].z));
		vec3 v2(max(bbox[1].x, box.bbox[1].x), max(bbox[1].y, box.bbox[1].y), max(bbox[1].z, box.bbox[1].z));

		return BBox(v1, v2);
	}

	void expand(const vec3& p)
	{
		//bbox[0] = minvec(bbox[0], p);
		//bbox[1] = maxvec(bbox[1], p);

		bbox[0] = vec3(min(bbox[0].x, p.x), min(bbox[0].y, p.y), min(bbox[0].z, p.z));
		bbox[1] = vec3(max(bbox[1].x, p.x), max(bbox[1].y, p.y), max(bbox[1].z, p.z));
	}

	uint32_t maxDim()
	{
		vec3 extend(bbox[1] - bbox[0]);
		if (extend.x > extend.y && extend.x > extend.z) return 0;
		else if (extend.y > extend.z) return 1;
		return 2;
		
	}
	uint32_t minDim()
	{
		vec3 extend(bbox[1] - bbox[0]);
		if (extend.x < extend.y && extend.x < extend.z) return 0;
		else if (extend.y < extend.z) return 1;
		return 2;
	}

	float area()
	{
		vec3 extend(bbox[1] - bbox[0]);
		return (extend.x * extend.y + extend.y * extend.z + extend.z * extend.x);
	}

	
};

#endif // ! _BBOX_H_

