#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "vec2.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define i255 1 / 255

//static int int_floor(const float& x)
//{
//	return (int)x - (x < (int)x);

	//int i = (int)x; /* truncate */
	//return i - (i > x); /* convert trunc to floor */
//}
int __fastcall clamp_int_texture(const int& i, const int& a, const int& b)
{
	//bad, cause error!!!
	//return max(a, min(i, b));



	return min(max(a, i), b);
	/*if (i <= a)
	return a;
	else if (i >= b)
	return b;
	return i;*/
}


struct Texture
{
	//I am very tempted to use just one function and same dimensions for all 3 
	//However, diffuse, alpha and bump textures some times have different dimensions
	
	int w;
	int h;
	int n;

	//int w2;
	//int h2;
	//int n2;

	//int w3;
	//int h3;
	int n3;

	//int w4;
	//int h4;
	int n4;

	//vector<float> c;//color
	//vector<float> a;//alpha
	//vector<float> b;//bump
	//vector<float> r;//rough

	
	vector<uint8_t> c;
	vector<float> a;
	vector<float> b;//bump
	vector<float> r;//rough

	vec3 __fastcall ev(const vec2& t) const
	{
		
		//int x = (t.x - floorf(t.x)) * (w);
		//int y = (t.y - floorf(t.y)) * (h);

		//int i = n * (x + y * w);

		float u = t.x - floorf(t.x),
			  v = t.y - floorf(t.y);

		int x = clamp_int_texture(int(u * w), 0, w - 1),
			y = clamp_int_texture(int((1.0f - v) * h), 0, h - 1);
		
		int i = n * (x + y * w);

		return vec3(c[i] , c[i + 1] , c[i + 2] );
	}


	float __fastcall ev_alpha(const vec2& t) const
	{
		//int x = (t.x - floorf(t.x)) * (w2);
		//int y = (t.y - floorf(t.y)) * (h2);

		//int i = n2 * (w2 * y + x);

		float u = t.x - floorf(t.x),
			  v = t.y - floorf(t.y);
		int x = clamp_int_texture(int(u * w), 0, w - 1),
			y = clamp_int_texture(int((1.0f - v) * h), 0, h - 1);
		//int i = x + ( h - 1 - y) * w;

		//int i = n2 * (x + y * w2);

		int i = (x + y * w);

		return a[i];
	}

	vec3 __fastcall ev_bump(const vec2& t) const
	{

		//int x = (t.x - floorf(t.x)) * (w3);
		//int y = (t.y - floorf(t.y)) * (h3);

	//	int i = 3 * (x + y * w3);


		float u = t.x - floorf(t.x),
			v = t.y - floorf(t.y);
		int x = clamp_int_texture(int(u * w), 0, w - 1),
			y = clamp_int_texture(int((1.0f - v) * h), 0, h - 1);
		
		int i = n3 * (x + y * w);

		return vec3(b[i], b[i + 1], b[i + 2]);
	}
	
	float __fastcall ev_roughness(const vec2& t) const
	{
		//int x = (t.x - floorf(t.x)) * (w2);
		//int y = (t.y - floorf(t.y)) * (h2);

		//int i = n2 * (w2 * y + x);

		float u = t.x - floorf(t.x),
			v = t.y - floorf(t.y);
		int x = clamp_int_texture(int(u * w), 0, w - 1),
			y = clamp_int_texture(int((1.0f - v) * h), 0, h - 1);
		//int i = x + ( h - 1 - y) * w;

		int i1 = n4 * (x + y * w);
		int i2 = n4 * (x + y * w) + 1;
		int i3 = n4 * (x + y * w) + 2;

		float r1 = r[i1];
		float r2 = r[i2];
		float r3 = r[i3];

		return 0.05;//(r1 + r2 + r3) * 0.333f;

		//return (0.299f * r1 + 0.587f * r2 + 0.114f * r3) * 0.1f;
	}

};

#endif // !_TEXTURE_H_
