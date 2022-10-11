#ifndef _VEC3_H_
#define _VEC3_H_
#include <iostream>

#define min(x, y) x < y ? x : y
#define max(x, y) x > y ? x : y


#define abs(x) ((x)<0 ? -(x) : (x))

const float eps = 0.0000001f;

#define inf 1e20
//const float pi = 3.1415926f;
//const float ipi = 0.3183098f;

const float pi = 3.1415926f;
const float ipi = 1.0f / pi;
const float i2pi = 0.5f / pi;
const float i4pi = 0.25f / pi;

const float tau = 2.0f * pi;
const float itau = 1.0f / tau;//same i2pi, i2pi added if you dont remeber tau

const float i180 = 0.0055f;

#define sqr(x) x * x

double inline __declspec (naked) __fastcall sqrt14(double n)
{
	_asm fld qword ptr[esp + 4]
		_asm fsqrt
	_asm ret 8
}




struct vec3
{
	vec3() : x(0), y(0), z(0) {}
	vec3(float v_) : x(v_), y(v_), z(v_) {}
	vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}


	float x, y, z;
		

	vec3 __fastcall norm() const
	{
		float l = 1 / sqrt14(x*x + y*y + z*z); return *this * l;
	}

	void __fastcall normalize() const
	{
		float l = 1 / sqrt14(x*x + y*y + z*z); *this *= l;
	}

	float _fastcall dot(const vec3& v) const
	{
		return x * v.x + y * v.y + z * v.z; 
	}


	vec3 __fastcall cross(const vec3& b) const
	{

		return{ y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x };

	}
	float _fastcall length() const { return sqrt14(x * x + y * y + z * z); }
	float _fastcall length2() const { return x * x + y * y + z * z; }

	float operator[](const int& i) const { return (&x)[i]; }


	friend vec3 __fastcall operator+(const vec3& a, const vec3& b)
	{
		return{ a.x + b.x, a.y + b.y, a.z + b.z };
	}
	friend vec3 __fastcall operator-(const vec3& a, const vec3& b)
	{

		return{ a.x - b.x, a.y - b.y, a.z - b.z };
	}
	friend vec3 __fastcall operator*(const vec3& a, const vec3& b)
	{

		return{ a.x * b.x, a.y * b.y, a.z * b.z };
	}

	friend vec3 operator*=(const vec3& a, const float& v)
	{
		//float ax = a.x, ay = a.y, az = a.z;
		return{ a.x * v, a.y * v, a.z * v };
	}

	friend vec3 __fastcall operator*(const vec3& a, const float& v) { return vec3(a.x * v, a.y * v, a.z * v); }
	friend vec3 __fastcall operator*(const float& v, const vec3& a) { return vec3(a.x * v, a.y * v, a.z * v); }
	vec3 operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z;  return *this; }
	vec3 operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z;  return *this; }
	void operator*=(const float& value) { x *= value; y *= value; z *= value; }
	void operator*=(const vec3& value) { x *= value.x; y *= value.y; z *= value.z; }

	void operator/=(const vec3& value) { x /= value.x; y /= value.y; z /= value.z; }

	float maxc() const { float d = max(x, y); return max(d, z); }//{ return max(max(x, y), z); }//
	float minc() const { float d = min(x, y); return min(d, z); }//{ return min(min(x, y), z); }//

	friend bool operator==(const vec3& a, const vec3& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	float average() const
	{
		return (x + y + z) / 3.0f;
	}

	friend vec3 operator/(const vec3& a, const vec3& b) { return{ a.x / b.x, a.y / b.y, a.z / b.z }; }
	friend vec3 operator/=(const vec3& a, const float& v) { return{ a.x / v, a.y / v, a.z / v }; }
	friend vec3 operator-(const vec3& a) { return{ -a.x, -a.y, -a.z }; }

	friend vec3 operator/(const vec3& a, const float& v) { return{ a.x / v, a.y / v, a.z / v }; }
	friend vec3 operator/(const vec3& a, const int& v) { return{ a.x / v, a.y / v, a.z / v }; }

	
	bool all_zero()
	{
		return x == y == z == 0.0f;
	}
};

#endif // !_MATH_H_

