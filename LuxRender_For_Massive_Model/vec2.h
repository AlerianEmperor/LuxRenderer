#ifndef _VEC2_H_
#define _VEC2_H_

struct vec2
{
	float x;
	float y;

	vec2() {}
	vec2(float x_, float y_) : x(x_), y(y_) {}

	vec2 operator* (const float& v) const { return vec2(x * v, y * v); }
	vec2 operator+ (const vec2& v) const { return vec2(x + v.x, y + v.y); }
	vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
};


#endif // !_VEC2_H_
