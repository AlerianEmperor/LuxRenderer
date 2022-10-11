#ifndef _ENVIROMENT_H_
#define _ENVIROMENT_H_
#include "vec3.h"
#include "Rnd.h"
#include "fasttrigo.h"
#include <vector>

using namespace std;

static float Luminance2(const vec3& c)
{
	return 0.3f * c.x + 0.6f * c.y + 0.1f * c.z;
}

static float clamp(const float& v, const float& low, const float& high)
{
	return min(max(low, v), high);
}

static int clamp(const int& v, const int& low, const int& high)
{
	return min(max(low, v), high);
}

struct Enviroment
{
	int width;
	int height;

	vector<vec3> c;

	vector<float> pdfX;
	vector<float> cdfX;

	vector<float> pdfY;
	vector<float> cdfY;



	Enviroment() {}
	Enviroment(int w, int h, vector<vec3> c_) : width(w), height(h), c(c_) { CDF(w, h, c_); }

	void CDF(int w, int h, vector<vec3> c_)
	{
		width = w;
		height = h;
		
		c.resize(width * height);
		c = c_;

		pdfX.resize(width * height);
		cdfX.resize(width * height);

		pdfY.resize(height);
		cdfY.resize(height);

		float total_weight_Y = 0.0f;

		for (int j = 0; j < height; ++j)
		{
			float total_weight_X = 0.0f;
			
			for (int i = 0; i < width; ++i)
			{
				float weight = Luminance2(c[j * width + i]);

				total_weight_X += weight;

				pdfX[j * width + i] = weight;
				cdfX[j * width + i] += weight;
			}

			float inv_total_weight_X = 1.0f / total_weight_X;

			for (int i = 0; i < width; ++i)
			{
				pdfX[j * width + i] *= inv_total_weight_X;
				cdfX[j * width + i] *= inv_total_weight_X;
			}

			total_weight_Y += total_weight_X;

			pdfY[j] = total_weight_X;
			cdfY[j] += total_weight_X;

		}

		float inv_total_weight_Y = 1.0f / total_weight_Y;

		for (int j = 0; j < height; ++j)
		{
			pdfY[j] *= inv_total_weight_Y;
			cdfY[j] *= inv_total_weight_Y;
		}
	}

	//convert a direction to uv coordinate
	//1 to compute probe value when hit by accident or when sample
	void DirectionToUV(const vec3& d, float& u, float& v)
	{
		double theta = acos(clamp(d.y, -1.0f, 1.0f));

		double phi = (d.x == 0.0f && d.z == 0.0) ? 0.0 : atan2(d.z, d.x);

		u = (pi + phi) * ipi * 0.5;

		v = theta * ipi;
	}

	//when obtain the uv use it compute value of the probe or enviroment 
	//here we just sample a spherical texture
	vec3 UVToValue(float& u, float& v)
	{
		int i = clamp(int(u * width), 0, width - 1);
		int j = clamp(int(v * height), 0, height - 1);

		return c[j * width + i];
	}

	float UVToPdf(float& u, float& v)
	{
		int i = clamp(int(u * width), 0, width - 1);
		int j = clamp(int(v * height), 0, height - 1);

		float pdf = pdfX[j * width + i] * pdfY[j];

		return pdf;
	}

	vec3 UVToDirection(float& u, float& v)
	{
		float theta = pi * v;
		float phi = 2.0f * pi * u;

		float sin_theta, cos_theta;
		float sin_phi, cos_phi;

		FTA::sincos(theta, &sin_theta, &cos_phi);
		FTA::sincos(phi, &sin_phi, &cos_phi);

		float x = -sin_theta * cos_phi;
		float y = cos_theta;
		float z = -sin_theta * sin_phi;
	}

	int SampleTexCoord(vector<float>& c, const int& low_, const int& high_, const float& value) 
	{
		int low = low_;
		int high = high_;

		while (low < high)
		{
			int mid = low + (high - low) / 2;

			if (c[mid] == value)
				return mid;
			else if (c[mid] < value)
				low = mid + 1;//mid
			else
				high = mid;//mid - 1
		}
		return low;
	}

	void SampleEnviroment(vec3& color, vec3& direction, float& pdf)
	{
		float r1 = randf();
		float r2 = randf();

		int row = SampleTexCoord(cdfY, 0, height, r1);

		int col = SampleTexCoord(cdfX, row * width, (row + 1) * width, r2) - row * width;

		color = c[row * width + col];

		float u = (float)col / width;
		float v = (float)row / height;

		pdf = pdfX[row * width + col] * pdfY[row];

		float sin_theta = FTA::sin(v * pi);

		if (sin_theta != 0.0f)
			pdf *= width * height / (2.0f * pi * pi * sin_theta);
		else
			pdf = 0.0f;

		direction = UVToDirection(u, v);
	}

};

#endif // !_ENVIROMENT_H_

