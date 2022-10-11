#ifndef _AREA_LIGHT_H_
#define _AREA_LIGHT_H_

//#include "Surface.h"
//#include "Shading.h"
#include "Geometry.h"
#include "Rnd.h"
//#include "Sampling_Struct.h"
//#include "Distribution.h"

using namespace std;

struct AreaLight
{
	vector<vec3> vertex;
	vector<vec3> normal;
	vector<vec3> Ke;
	vector<float> area;
	vector<float> sum_area;
	vector<float> pdf;
	int size;
	float total_area;
	float invA;

	AreaLight() {}

	int sample() const
	{
		float s = randf();
		if (s <= sum_area[0])
		{
			return 0;
		}
		for (int i = 1; i < size; ++i)
		{
			if (sum_area[i - 1] < s && s <= sum_area[i])
			{
				return i;
			}
		}
		return size - 1;

		//return rand() % size;
	}

	vec3 __fastcall sample_light(const int& i)
	{
		float su0 = sqrt14(randf());
		float b0 = 1.0f - su0;
		float b1 = randf() * su0;

		return b0 * vertex[3 * i] + b1 * vertex[3 * i + 1] + (1.0f - b0 - b1) * vertex[3 * i + 2];

		//return b0 * (vertex[3 * i] - vertex[3 * i + 2]) + b1 * (vertex[3 * i + 1] - vertex[3 * i + 2]) + vertex[3 * i + 2];

		//return b0 * vertex[3 * i] + b1 * vertex[3 * i + 1] + vertex[3 * i + 2];
	}

	void norm()
	{
		size = area.size();

		sum_area.resize(size);
		pdf.resize(size);

		float sum = 0.0f;
		for (int i = 0; i < size; ++i)
		{
			sum += area[i];
			sum_area[i] = sum;
		}
		total_area = sum;
		invA = 1.0f / total_area;
		float isum = 1.0f / total_area;

		for (auto& v : sum_area)
			v *= isum;

		for (int i = 0; i < size; ++i)
		{
			pdf[i] = area[i] * isum;

			//vertex[3 * i] = vertex[3 * i] - vertex[3 * i + 2];
			//vertex[3 * i + 1] = vertex[3 * i + 1] - vertex[3 * i + 2];
		}
	}

	void add_light(const vec3& a, const vec3& b, const vec3& c)
	{
		vertex.emplace_back(a);
		vertex.emplace_back(b);
		vertex.emplace_back(c);

		vec3 cr((a - c).cross(b - c));

		//cout << "a: " << a.x << " " << a.y << " " << a.z << "\n";
		//cout << "b: " << b.x << " " << b.y << " " << b.z << "\n";
		//cout << "c: " << c.x << " " << c.y << " " << c.z << "\n";
		float tr = sqrt14(cr.dot(cr)) * 0.5f;
		//cout << "tr: " << tr << "\n";
		if (tr == 0.0f)
			tr = 1.5f;

		//tr = 0.01f;
		//cout << tr << "\n";

		//float tr = 0.1f;
		area.emplace_back(tr);
		normal.emplace_back(cr.norm());

	}

	void add_light_ti(const vec3& a, const vec3& b, const vec3& c, const int& t)
	{
		vertex.emplace_back(a);
		vertex.emplace_back(b);
		vertex.emplace_back(c);
		vec3 cr((b - a).cross(c - a));
		float tr = sqrt14(cr.dot(cr)) * 0.5f;

		area.emplace_back(tr);

		normal.emplace_back(cr.norm());

	}



};
#endif // !_AREA_LIGHT_H_
