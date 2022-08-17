#ifndef _BSDF_ROUGHNESS_FINAL_H_
#define _BSDF_ROUGHNESS_FINAL_H_
//#include "Ray.h"
//#include "Rnd.h"
//#include "fasttrigo.h"

#include "material.h"
#include "Microfacet.h"
#include "Fresnel.h"
//#include "GGX.h"

#define sign_num(x) x > 0 ? 1 : -1
//1e-6f -> eps
struct Sampling_Struct
{
	Sampling_Struct() {}
	Sampling_Struct(vec3 direction_, vec3 weight_, float pdf_, bool isSpecular_) : direction(direction_), weight(weight_), pdf(pdf_), isSpecular(isSpecular_) {}
	vec3 direction;
	vec3 weight = vec3(0.0f);
	float pdf = 0.0f;
	bool isSpecular = false;

};

//TO DO 1:
//https://github.com/LuxCoreRender/LuxCore/blob/master/src/slg/materials/cloth.cpp
//add cloth from luxcore render
//it appear that paper about cloth rendering scarce
//dream work 2017 paper is wrong

//TO DO 2:
//become IGM on codeforces
//start date
//9:35 pm 17-8-2022

class BSDF_Sampling
{
public:
	virtual bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const = 0;
	virtual vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const = 0;
	virtual float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const = 0;
	virtual bool isSpecular() const = 0;
};

//-----------DIFFUSE----------

static vec3 cosine_hemisphere_Sampling_with_n(const vec3& n)
{
	float u = randf(), v = randf();
	float r = sqrt14(u);
	float theta = 2.0f * pi * v;

	float c, s;
	FTA::sincos(theta, &c, &s);

	vec3 coord(r * c, r * s, sqrt14(1.0f - u));

	onb local(n);

	vec3 direction(local.u * coord.x + local.v * coord.y + local.w * coord.z);

	return direction.norm();
}

class Diffuse : public BSDF_Sampling
{
public:
	Diffuse() {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		vec3 d = cosine_hemisphere_Sampling_with_n(n);

		float cos_o = (d.dot(n));

		//if (cos_o <= 1e-6)//0.0f
		if (cos_o <= eps)//0.0f	
		{
			//cout << "negative cos\n";
			return false;
		}
		float pdf = (cos_o) * ipi;

		//vec3 weight = color;

		sampling = Sampling_Struct(d, color, pdf, false);

		isReflect = true;
		return true;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos = direction_out.dot(n);

		return cos > eps ? cos * ipi : 0.0f;//original
		
		//return abs(cos);
		
		//return cos > 1e-6 ? cos * ipi : 1.0f;//0.0f
		
		//return (direction_out.dot(n)) * ipi;
	}

	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos = direction_out.dot(n);


		return cos > eps ? cos * color * ipi : vec3(0.0f);//0.0f
		
		//return abs(cos) * color * ipi;
		
		//return cos > 1e-6 ? cos * color * ipi : vec3(0.0f);
	}

	bool isSpecular() const
	{
		return false;
	}
};

//----------MIRROR----------

static vec3 Reflect(const vec3& d, const vec3& n)
{
	return (d - 2.0f * d.dot(n) * n).norm();
}

class Mirror : public BSDF_Sampling
{
public:
	Mirror() {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), 1.0f, true);
		isReflect = true;
		return true;

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}

	bool isSpecular() const
	{
		return true;
	}

};

//----------GLASS----------

static vec3 Refract(const vec3& direction_in, const vec3& n, const float& eta, const float& cos_i, const float& cos_t)
{
	return (eta * direction_in + (eta * cos_i - cos_t) * n).norm();
}

class Glass_Color : public BSDF_Sampling
{
public:
	float ior = 1.6f;
	vec3 real_color = vec3(0.816842f, 0.897079f, 0.861554f);//default green color

	Glass_Color() {}
	Glass_Color(float ior_) : ior(ior_) {}
	Glass_Color(vec3 real_color_, float ior_) : real_color(real_color_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//cout << "glass\n";

		//if(n.x != original_n.x || n.y != original_n.y || n.z != original_n.z)


		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float reflectance, cos_t;

		cos_i = abs(cos_i);

		reflectance = Dielectric_Reflectance(eta, cos_i, cos_t);

		if (randf() < reflectance)
		{
			vec3 direction(Reflect(direction_in, n));
			isReflect = true;

			//sampling = Sampling_Struct(direction, vec3(1.0f), reflectance, true);

			sampling = Sampling_Struct(direction, real_color, reflectance, true);

			return true;
		}
		else
		{
			vec3 direction(Refract(direction_in, n, eta, cos_i, cos_t));
			isReflect = false;

			//sampling = Sampling_Struct(direction, vec3(eta * eta), 1.0f - reflectance, true);

			//sampling = Sampling_Struct(direction, vec3(1.0f), 1.0f - reflectance, true);

			sampling = Sampling_Struct(direction, real_color, 1.0f - reflectance, true);

			return true;
		}

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}

	bool isSpecular() const
	{
		return true;
	}
};

class Glass_Green : public BSDF_Sampling
{
public:
	float ior = 1.6f;
	vec3 real_color = vec3(0.816842f, 0.897079f, 0.861554f);

	Glass_Green() {}
	Glass_Green(float ior_) : ior(ior_) {}
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//cout << "glass\n";

		//if(n.x != original_n.x || n.y != original_n.y || n.z != original_n.z)


		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0 ? ior : 1.0f / ior;

		float reflectance, cos_t;

		float abs_cos_i = abs(cos_i);

		reflectance = Dielectric_Reflectance(eta, cos_i, cos_t);



		if (randf() < reflectance)
		{
			vec3 direction(Reflect(direction_in, n));
			isReflect = true;

			//sampling = Sampling_Struct(direction, vec3(1.0f), reflectance, true);

			sampling = Sampling_Struct(direction, real_color, reflectance, true);

			return true;
		}
		else
		{
			//vec3 direction(Refract(direction_in, n, eta, cos_t));
			vec3 direction(Refract(direction_in, n, eta, abs_cos_i, cos_t));
			isReflect = false;

			//sampling = Sampling_Struct(direction, vec3(eta * eta), 1.0f - reflectance, true);

			//sampling = Sampling_Struct(direction, vec3(1.0f), 1.0f - reflectance, true);

			sampling = Sampling_Struct(direction, real_color, 1.0f - reflectance, true);

			return true;
		}

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}

	bool isSpecular() const
	{
		return true;
	}
};

class Glass_Schlick : public BSDF_Sampling
{
public:
	float ior;

	Glass_Schlick() {}
	Glass_Schlick(float ior_) : ior(ior_) {}
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

		float R;

		vec3 Refractive_direction;

		if (c2 < 0.0f)
			R = 1.0f;
		else
		{
			//Version of computing cos_t or cosine of refractive direction
			//original version

			Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
			float cos_t = cos_i < 0.0f ? abs_cos_i : -Refractive_direction.dot(n);

			//use dot product

			//float cos_t = abs(Refractive_direction.dot(n));
			//Refractive_direction = Refract(direction_in, n, eta, cos_t);
			//Refractive_direction = Refract(direction_in, n, eta, abs_cos_i, cos_t);


			//new cos_t version use fomular

			//in this version cos_t was compute before Refractive direction
			//unlike the original version where refractive_direction is compute first, then come cos_t

			//float cos_t = sqrt14(c2);
			//Refractive_direction = Refract(direction_in, n, eta, abs_cos_i, cos_t);




			float R0 = (ior - 1.0f) / (ior + 1.0f);

			R0 = R0 * R0;

			float p = 1.0f - cos_t;

			float p2 = p * p;

			R = R0 + (1.0f - R0) * p2 * p2 * p;
		}

		if (randf() < R)
		{
			vec3 direction_out = direction_in - 2.0f * cos_i * n;
			sampling = Sampling_Struct(direction_out, vec3(1.0f), R, true);
			isReflect = true;
			return true;
		}
		else
		{
			vec3 direction_out = Refractive_direction;
			sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - R, true);
			isReflect = false;
			return true;
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}

	bool isSpecular() const
	{
		return true;
	}
};


class Glass_Fresnel : public BSDF_Sampling
{
public:
	float ior;

	Glass_Fresnel() {}
	Glass_Fresnel(float ior_) : ior(ior_) {}
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);

		if (randf() < F)
		{

			vec3 direction_out = direction_in - 2.0f * cos_i * n;
			sampling = Sampling_Struct(direction_out, vec3(1.0f), F, true);
			isReflect = true;
			return true;
		}
		else
		{

			//Refract  va Refract 2 deu dung
			//Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
			//float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

			vec3 direction_out = (eta * direction_in + (eta * abs_cos_i - cos_t) * n).norm();

			//vec3 direction_out = Refract(direction_in, n, eta, cos_t);

			//vec3 direction_out = Refract(direction_in, n, eta, abs_cos_i, cos_t);

			sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - F, true);
			isReflect = false;
			return true;
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	bool isSpecular() const
	{
		return true;
	}
};

class Glass_No_Refract : public BSDF_Sampling
{
public:
	float ior;
	vec3 real_color = vec3(1.0f);

	Glass_No_Refract() {}
	Glass_No_Refract(float ior_) : ior(ior_) {}
	Glass_No_Refract(vec3 real_color_, float ior_) : real_color(real_color_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);

		if (randf() < F)
		{
			vec3 direction_out = direction_in - 2.0f * cos_i * n;
			sampling = Sampling_Struct(direction_out, real_color, F, true);
			isReflect = true;
			return true;
		}
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	bool isSpecular() const
	{
		return true;
	}
};

class Glass_Green_Tint : public BSDF_Sampling
{
public:
	float ior = 1.6f;
	float density = 0.6f;
	float infinite_bounce_threshold = 0.4f;

	//Invented by Phan Sang

	//SUPER IMPORTANCE!!!!!!
	//CHEAT look So Good!!!

	//real_color define how the side color of glass look
	//The higher the number the greener it look
	//when light reach the side of glass at a low angle
	//it bouncse many time inside the glass
	//and the green color was amplify every bounce

	//However use Beer law or fresnel will not achive the goal the create this green tint
	//because the light usually only bounce 1 or two times inside glass

	//so I create these color to mimic the infinite bounce of color inside glass
	//and set the bounce condition when the dot product is positive
	//it mean when it only happened when light refract
	//and smaller than a certain threshold
	//0.4 in this case

	//real_color bounce was compute corresponding to the power of 2 for easy combination
	//real_color_7 = real_color_4 * real_color_2 * color instead of multiply color 7 times

	//Normally, this type of phenomenal only achive by spectrum render or adding a medium
	//However, I create some thing that look good without the spectrum or Medium

	//YAY!!!!!


	//This part for Beer Law
	//Not very useful
	//Based on real data 
	//https://chempics.files.wordpress.com/2020/06/combinedribenainstructions.pdf
	//page 19

	//float ext_r = 0.00783f;
	//float ext_g = 0.06537f;
	//float ext_b = 0.00596f;

	//vec3 ext = vec3(0.00783f, 0.06537f, 0.00596f);

	//vec3 ext = vec3(20.0f, 10.0f, 40.0f);

	//vec3 ext = vec3(8.97079, 8.16842, 8.61554);

	vec3 real_color = vec3(0.816842f, 0.897079f, 0.861554f);

	vec3 real_color2 = real_color * real_color;

	vec3 real_color4 = real_color2 * real_color2;

	vec3 real_color6 = real_color4 * real_color2;

	//vec3 real_color7 = real_color * real_color * real_color * real_color * real_color * real_color * real_color;

	vec3 real_color7 = real_color4 * real_color2 * real_color;

	vec3 real_color8 = real_color4 * real_color4;

	vec3 real_color10 = real_color8 * real_color2;

	vec3 real_color12 = real_color7 * real_color4 * real_color;

	vec3 real_color16 = real_color8 * real_color8;

	vec3 ext = vec3(8.97079, 7.46842, 8.61554);

	Glass_Green_Tint() {}
	Glass_Green_Tint(float ior_) : ior(ior_) {}
	Glass_Green_Tint(float ior_, float density_) : ior(ior_), density(density_) {}
	Glass_Green_Tint(float ior_, float density_, float infinite_bounce_threshold_) : ior(ior_), density(density_), infinite_bounce_threshold(infinite_bounce_threshold_) {}

	//the Real Sampling happend in sample volume
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		vec3 weight(1.0f);

		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		if (cos_i > 0.0f && cos_i < infinite_bounce_threshold)
		{
			weight *= real_color16;
		}


		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);

		if (randf() < F)
		{
			vec3 direction_out = direction_in - 2.0f * cos_i * n;

			sampling = Sampling_Struct(direction_out, weight, F, true);

			isReflect = true;
			return true;
		}
		else
		{
			vec3 direction_out = (eta * direction_in + (eta * abs_cos_i - cos_t) * n).norm();

			sampling = Sampling_Struct(direction_out, weight, 1.0f - F, true);

			isReflect = false;
			return true;
		}
		return true;
	}
	bool sample_volume(const vec3& direction_in, vec3& n, vec3& original_n, float& t, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		vec3 weight(1.0f);

		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		if (cos_i > 0.0f && cos_i < infinite_bounce_threshold)
		{
			//Beer law
			/*vec3 extinction_coeficient = t * real_color;//t * ext;//t * density * ext;// *color;

			vec3 Tint = vec3(expf(-extinction_coeficient.x), expf(-extinction_coeficient.y), expf(-extinction_coeficient.z));

			weight *= Tint;*/

			weight *= real_color16;
		}

		//weight *= vec3(0.816842, 0.897079, 0.861554);

		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);

		if (randf() < F)
		{
			vec3 direction_out = direction_in - 2.0f * cos_i * n;
			//sampling = Sampling_Struct(direction_out, real_color, F, true);
			//sampling = Sampling_Struct(direction_out, real_color5, F, true);

			sampling = Sampling_Struct(direction_out, weight, F, true);

			isReflect = true;
			return true;
		}
		else
		{
			//Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
			//float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

			vec3 direction_out = (eta * direction_in + (eta * abs_cos_i - cos_t) * n).norm();
			//sampling = Sampling_Struct(direction_out, real_color, 1.0f - F, true);
			//sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - F, true);

			sampling = Sampling_Struct(direction_out, weight, 1.0f - F, true);

			isReflect = false;
			return true;
		}
		return true;
	}
	//vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const = 0;
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const = 0;
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	bool isSpecular() const
	{
		return true;
	}
};

class Glass_Beer_Law : public BSDF_Sampling
{
public:
	float ior = 1.6f;
	float density = 0.6f;

	//This part for Beer Law
	//Not very useful
	//Based on real data 
	//https://chempics.files.wordpress.com/2020/06/combinedribenainstructions.pdf
	//page 19

	//float ext_r = 0.00783f;
	//float ext_g = 0.06537f;
	//float ext_b = 0.00596f;

	//vec3 ext = vec3(0.00783f, 0.06537f, 0.00596f);

	//vec3 ext = vec3(20.0f, 10.0f, 40.0f);


	vec3 ext = vec3(8.97079, 8.16842, 8.61554);

	Glass_Beer_Law() {}
	Glass_Beer_Law(float ior_) : ior(ior_) {}
	Glass_Beer_Law(float ior_, float density_) : ior(ior_), density(density_) {}


	//the Real Sampling happend in sample volume
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		return false;
	}
	bool sample_volume(const vec3& direction_in, vec3& n, vec3& original_n, float& t, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		vec3 weight(1.0f);

		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		if (cos_i > 0.0f)
		{
			//Beer law
			vec3 extinction_coeficient = t * ext;// *real_color;//t * ext;//t * density * ext;// *color;

			vec3 Tint = vec3(expf(-extinction_coeficient.x), expf(-extinction_coeficient.y), expf(-extinction_coeficient.z));

			weight *= Tint;

		}

		//weight *= vec3(0.816842, 0.897079, 0.861554);

		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);

		if (randf() < F)
		{
			vec3 direction_out = direction_in - 2.0f * cos_i * n;
			//sampling = Sampling_Struct(direction_out, real_color, F, true);
			//sampling = Sampling_Struct(direction_out, real_color5, F, true);

			sampling = Sampling_Struct(direction_out, weight, F, true);

			isReflect = true;
			return true;
		}
		else
		{
			//Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
			//float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

			vec3 direction_out = (eta * direction_in + (eta * abs_cos_i - cos_t) * n).norm();
			//sampling = Sampling_Struct(direction_out, real_color, 1.0f - F, true);
			//sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - F, true);

			sampling = Sampling_Struct(direction_out, weight, 1.0f - F, true);

			isReflect = false;
			return true;
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}

	bool isSpecular() const
	{
		return true;
	}
};

static float remap_roughness(const float& roughness, const float& cos_i)
{
	return (1.2f - 0.2f * sqrt14(abs(cos_i))) * roughness;
}

vec3 ggx_sample_H(const vec3& n, const float& alpha)
{
	float u1 = randf();
	float u2 = randf();

	//float alpha2 = alpha;// *alpha;

	float cos_theta = sqrt14((1.0f - u1) / (u1 * (alpha * alpha - 1.0f) + 1.0f));

	float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

	float phi = 2.0f * pi * u2;

	onb local(n);

	vec3 h = (local.u * cosf(phi) + local.v * sinf(phi)) * sin_theta + n * cos_theta;

	//h.normalize();

	return h.norm();
}


float ggx_D(const vec3& m, const vec3& n, const float& alpha)
{
	float cos_theta = m.dot(n);

	//float alpha2 = alpha;// *alpha;

	float cos_theta2 = cos_theta * cos_theta;

	float denom = cos_theta2 * (alpha * alpha - 1.0f) + 1.0f;


	//Here we use 1.0f in case of roughness is 0 or really close to 0
	//if this value is 0, it will cause condition
	//pdf_m < 1e-10 to false in cause the whole sampling process to false
	//and the rough glass appear black

	//fix to 1.0f for rough dieletric case
	//and this will effectively turn rough glass to smooth glass in case of roughness = 0.0f
	return cos_theta > 0.0f ? alpha / (pi * denom * denom) : 1.0f;

	//original good
	//return cos_theta > 0.0f ? alpha / (pi * denom * denom) : 0.0f;
}

float ggx_G1(const vec3& w, const vec3& m, const vec3& n, const float& alpha)
{
	float cos_i_n = w.dot(n);
	float cos_i_m = w.dot(m);

	if (cos_i_m <= 0.0f || cos_i_n <= 0.0f)
		return 0.0f;

	float cos_theta2 = cos_i_n * cos_i_n;

	float tan_theta2 = (1.0f - cos_theta2) / cos_theta2;

	if (tan_theta2 == 0.0f)
		return 1.0f;

	//float alpha2 = alpha;// *alpha;

	return 2.0f / (1.0f + sqrt14(1.0f + alpha * alpha * tan_theta2));
}

float ggx_G(const vec3& wi, const vec3& wo, const vec3& m, const vec3& n, const float& alpha)
{
	return ggx_G1(-wi, m, n, alpha) * ggx_G1(wo, m, n, alpha);
}

float ggx_pdf(const vec3& m, const vec3& n, const float& alpha)
{
	return ggx_D(m, n, alpha) * m.dot(n);
}

class Rough_Glass_Fresnel : public BSDF_Sampling
{
public:
	float ior;
	float roughness;
	Microfacet_Distribution dist = GGX;


	Rough_Glass_Fresnel() {}
	Rough_Glass_Fresnel(float ior_, float roughness_) : ior(ior_), roughness(roughness_) {}
	Rough_Glass_Fresnel(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		float alpha = remap_roughness(roughness, abs_cos_i);

		//vec3 m = ggx_sample_H(n, alpha);

		vec3 m = Microfacet_Sample(dist, n, alpha);

		float cos_i_m = -direction_in.dot(m);

		float abs_cos_i_m = abs(cos_i_m);

		float F, cos_t;

		//F = Fresnel_Dielectric(eta, abs_cos_i_m, cos_t);

		F = Dielectric_Reflectance(eta, abs_cos_i_m, cos_t);

		vec3 direction_out;

		if (randf() < F)
		{
			//vec3 direction_out = direction_in - 2.0f * cos_i * n;

			direction_out = Reflect(direction_in, m);
			//sampling = Sampling_Struct(direction_out, vec3(1.0f), F, true);
			isReflect = true;
			//return true;
		}
		else
		{
			//Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
			//float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

			direction_out = (eta * direction_in + (eta * abs_cos_i_m - cos_t) * m).norm();
			//sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - F, true);
			isReflect = false;
			//return true;
		}

		bool reflected = cos_i * direction_out.dot(original_n) <= 0.0f;

		if (reflected != isReflect)
			return false;

		//compute weight and pdf
		float cos_i_h = -direction_in.dot(m);
		float cos_o_h = direction_out.dot(m);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha);

		//float D = ggx_D(m, n, alpha);

		//float pdf_m = ggx_pdf(m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float D = Microfacet_D(dist, m, n, alpha);

		float pdf_m = Microfacet_pdf(dist, m, n, alpha);

		if (pdf_m < 1e-10)
			return false;

		vec3 weight = vec3(abs(cos_i_h) * D * G / (pdf_m * abs_cos_i));

		float sample_pdf;
		if (isReflect)
		{
			sample_pdf = pdf_m / (4.0f * abs(cos_i_h)) * F;
		}
		else
		{
			float x = eta * cos_i_h + cos_o_h;
			sample_pdf = pdf_m * cos_o_h / (x * x) * (1.0f - F);
		}


		sampling = Sampling_Struct(direction_out, vec3(1.0f), sample_pdf, false);
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float original_cos_i = direction_in.dot(original_n);

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		bool is_reflection = cos_i * cos_o >= 0.0f;

		float eta = original_cos_i > 0.0f ? ior : 1.0f / ior;

		vec3 m;

		if (is_reflection)
			m = (-direction_in + direction_out).norm();
		else
			m = (direction_in * eta - direction_out).norm();

		float cos_i_m = -direction_in.dot(m);
		float cos_o_m = direction_out.dot(m);

		float eta_h = cos_i_m > 0.0f ? 1.0f / ior : ior;

		float F = Dielectric_Reflectance(eta, abs(cos_i_m));

		//float G = ggx_G(direction_in, direction_out, m, n, roughness);

		//float D = ggx_D(m, n, roughness);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, roughness);

		float D = Microfacet_D(dist, m, n, roughness);

		float r;

		if (is_reflection)
		{
			r = (F * G * D) / (4.0f * abs(cos_i));
		}
		else
		{
			float x = eta * cos_i_m + cos_o_m;
			r = abs(cos_i_m * cos_o_m) * (1.0f - F) * G * D * (x * x * abs(cos_i));
		}
		return r * color;
	}


	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		bool is_reflection = cos_i * cos_o >= 0.0f;

		float alpha = (1.2f - 0.2f * sqrt14(abs(cos_i))) * roughness;

		float eta = cos_i > 0.0f ? 1.0f / ior : ior;

		vec3 m;

		if (is_reflection)
			m = (-direction_in + direction_out).norm();
		else
			m = (direction_in * eta - direction_out).norm();

		float cos_i_m = -direction_in.dot(m);
		float cos_o_m = direction_out.dot(m);

		float eta_h = cos_i_m > 0.0f ? 1.0f / ior : ior;

		float F = Dielectric_Reflectance(eta_h, abs(cos_i_m));

		//float pdf_m = ggx_pdf(m, n, alpha);

		float pdf_m = Microfacet_pdf(dist, m, n, roughness);

		float sample_pdf;

		if (is_reflection)
			sample_pdf = pdf_m / (4.0f * abs(cos_i_m)) * F;
		else
		{
			float x = eta * cos_i_m + cos_o_m;

			sample_pdf = pdf_m * abs(cos_o_m) / (x * x) * (1.0f - F);
		}

		return sample_pdf;
	}
	bool isSpecular() const
	{
		return false;
	}
};

//Very Good Version !!!
/*
class Glass_Fresnel_Rough : public BSDF_Sampling
{
public:
float ior;
float roughness;

Glass_Fresnel_Rough() {}
Glass_Fresnel_Rough(float ior_,  float roughness_) : ior(ior_), roughness(roughness_) {}
bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
{
float cos_i = direction_in.dot(original_n);

float eta = cos_i >= 0.0f ? ior : 1.0f / ior;

float abs_cos_i = abs(cos_i);

float alpha = remap_roughness(roughness, abs_cos_i);

vec3 m = ggx_sample_H(n, alpha);

float cos_i_m = -direction_in.dot(m);

float abs_cos_i_m = abs(cos_i_m);

float F, cos_t;

F = Fresnel_Dielectric(eta, abs_cos_i_m, cos_t);


if (randf() < F)
{
//vec3 direction_out = direction_in - 2.0f * cos_i * n;

vec3 direction_out = Reflect(direction_in, m);
sampling = Sampling_Struct(direction_out, vec3(1.0f), F, true);
isReflect = true;
return true;
}
else
{
//Refractive_direction = (eta * direction_in + (eta * abs_cos_i - sqrt14(c2)) * n).norm();
//float c2 = 1.0f - eta * eta * (1.0f - cos_i * cos_i);

vec3 direction_out = (eta * direction_in + (eta * abs_cos_i_m - cos_t) * m).norm();
sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f - F, true);
isReflect = false;
return true;
}
return true;
}
vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
{
return vec3(1.0f);
}
float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
{
return 1.0f;
}
vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
{
return vec3(0.0f);
}
bool isSpecular() const
{
return true;
}
bool isSampleVolume() const
{
return true;
}
};
*/


//----------Conductor----------
//http://www.cs.virginia.edu/~jdl/bib/globillum/shirley_thesis.pdf
//formular 2.4 2.5 page 15

static float conductor_Reflectance(const float& eta, const float& k, const float& cos_i)
{
	float cos_i2 = cos_i * cos_i;

	float sin_i2 = 1.0f - cos_i2;

	float sin_i4 = sin_i2 * sin_i2;

	float under_sqr = eta * eta - k * k - sin_i2;

	float a2_b2 = sqrt14(under_sqr * under_sqr + 4.0f * eta * eta * k * k);

	float a = sqrt14(max((a2_b2 + under_sqr) * 0.5f, 0.0f));

	float Rs = ((a2_b2 + cos_i2) - 2.0f * a * cos_i) / ((a2_b2 + cos_i2) + 2.0f * a * cos_i);

	//float Rp = (a2_b2 * cos_i2 - 2.0f * a * sin_i2 * cos_i + sin_i4) / (a2_b2 * cos_i2 + 2.0f * a * sin_i2 * cos_i + sin_i4);

	float Rp = ((a2_b2 * cos_i2 + sin_i4) - 2.0f * a * sin_i2 * cos_i) / ((a2_b2 * cos_i2 + sin_i4) + 2.0f * a * sin_i2 * cos_i);

	return (Rs * Rs + Rp * Rp) * 0.5f;
}

class Conductor : public BSDF_Sampling
{
public:
	ComplexIor complex_ior;

	Conductor() {}
	Conductor(ComplexIor complex_ior_) : complex_ior(complex_ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= eps)
			return false;

		vec3 direction = Reflect(direction_in, n);

		isReflect = true;

		float x = conductor_Reflectance(complex_ior.eta.x, complex_ior.k.x, cos_i);
		float y = conductor_Reflectance(complex_ior.eta.y, complex_ior.k.y, cos_i);
		float z = conductor_Reflectance(complex_ior.eta.z, complex_ior.k.z, cos_i);

		vec3 bsdf_eval(x, y, z);

		sampling = Sampling_Struct(direction, bsdf_eval * color, 1.0f, true);
		return true;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		vec3 weight = Conductor_Reflectance_RGB(complex_ior.eta, complex_ior.k, cos_i);
		return color * weight;
		//return vec3(1.0f);//vec3(1.0f) indicate not use
	}

	bool isSpecular() const
	{
		return true;
	}
};


class Plastic_Diffuse : public BSDF_Sampling
{
public:
	float ior = 1.7f;

	Plastic_Diffuse() {}
	Plastic_Diffuse(float ior_) : ior(ior_) {}
	//bool sample(const vec3& direction_in, vec3& n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//cout << "plastic\n";
		float cos_i = -direction_in.dot(n);

		if (cos_i <= eps)//<=0
			return false;

		float eta = 1.0f / ior;

		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;
		
		//cout << "plastic" << "\n";
		isReflect = true;
		//float a = 0.8f;

		if (randf() < spec_prob)//a)//spec_prob
		{
			if (spec_prob <= eps)
				return false;
			sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), spec_prob, true);
			return true;
		}
		else
		{
			//vec3 direction = cosine_hemisphere_Sampling();

			vec3 direction = cosine_hemisphere_Sampling_with_n(n);

			//sampling = Sampling_Struct(cosine_hemisphere_Sampling(), color, 1.0f - spec_prob, false);

			float sample_pdf = direction.dot(n) * ipi * (1.0f - spec_prob);

			if (sample_pdf <= eps)
				return false;

			//sampling = Sampling_Struct(direction, color, direction.dot(n) * ipi * (1.0f - spec_prob), false);

			sampling = Sampling_Struct(direction, color, sample_pdf, false);

			return true;
		}
		return true;
	}
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n) const
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= eps || cos_o <= eps)//<=0.0f
			return 0.0f;

		float eta = 1.0f / ior;

		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;

		float sample_pdf = cos_o * ipi * (1.0f - spec_prob);

		if (sample_pdf <= eps)
			return 0.0f;
		return sample_pdf;
	}
	//vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color) const
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= eps || cos_o <= eps)//<=0.0f
			return vec3(0.0f);

		float eta = 1.0f / ior;

		//float reflectance, cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		return ipi * cos_o * (1.0f - reflectance) * color;
	}

	bool isSpecular() const
	{
		return false;
	}
};

class Plastic_Specular : public BSDF_Sampling
{
public:
	float ior = 1.7f;

	Plastic_Specular() {}
	Plastic_Specular(float ior_) : ior(ior_) {}
	//bool sample(const vec3& direction_in, vec3& n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//cout << "plastic\n";
		float cos_i = -direction_in.dot(n);
		float eta = 1.0f / ior;

		if (cos_i <= eps)
			return false;
		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;
		//cout << "plastic" << "\n";
		isReflect = true;
		//float a = 0.8f;

		if (randf() < spec_prob)//a)//spec_prob
		{
			sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), spec_prob, true);
			return true;
		}
		else
		{
			//vec3 direction = cosine_hemisphere_Sampling();

			vec3 direction = cosine_hemisphere_Sampling_with_n(n);

			//sampling = Sampling_Struct(cosine_hemisphere_Sampling(), color, 1.0f - spec_prob, false);

			sampling = Sampling_Struct(direction, color, direction.dot(n) * ipi * (1.0f - spec_prob), true);

			return true;
		}
		return true;
	}
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n) const
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);
		float eta = 1.0f / ior;

		if (cos_i <= eps || cos_o <= eps)
			return 0.0f;

		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;

		return cos_o * ipi * (1.0f - spec_prob);
	}
	//vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color) const
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);
		if (cos_i <= eps || cos_o <= eps)
			return vec3(0.0f);
		float eta = 1.0f / ior;

		//float reflectance, cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		return ipi * cos_o * (1.0f - reflectance) * color;
	}

	bool isSpecular() const
	{
		return true;
	}
};

class Plastic_Diffuse_Thickness : public BSDF_Sampling
{
public:
	float ior = 1.7f;
	float thickness = 2.0f;
	vec3 sigmaA = vec3(0.1f, 0.2f, 0.3f);//vec3(0.1f, 0.2f, 0.3f);

										 //preset
	float average_transmitance;
	vec3 scaleSigma;

	Plastic_Diffuse_Thickness() { prepare_before_render(); }
	Plastic_Diffuse_Thickness(float ior_) : ior(ior_) { prepare_before_render(); }


	void prepare_before_render()
	{
		scaleSigma = thickness * sigmaA;
		average_transmitance = expf(-2.0f * scaleSigma.average());
	}

	//bool sample(const vec3& direction_in, vec3& n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//cout << "plastic\n";
		float cos_i = -direction_in.dot(n);
		float eta = 1.0f / ior;

		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;
		//cout << "plastic" << "\n";
		isReflect = true;
		//float a = 0.8f;

		if (randf() < spec_prob)//a)//spec_prob
		{
			sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), spec_prob, true);
			return true;
		}
		else
		{
			//vec3 direction = cosine_hemisphere_Sampling();

			vec3 direction = cosine_hemisphere_Sampling_with_n(n);

			//sampling = Sampling_Struct(cosine_hemisphere_Sampling(), color, 1.0f - spec_prob, false);

			float cos_o = direction.dot(n);

			vec3 weight = color;

			if (scaleSigma.maxc() > 0.0f)
			{

				weight.x *= expf(scaleSigma.x * (-1.0f / cos_o - 1.0f / cos_i));
				weight.y *= expf(scaleSigma.y * (-1.0f / cos_o - 1.0f / cos_i));
				weight.z *= expf(scaleSigma.z * (-1.0f / cos_o - 1.0f / cos_i));
			}

			sampling = Sampling_Struct(direction, weight, direction.dot(n) * ipi * (1.0f - spec_prob), false);

			return true;
		}
		return true;
	}
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n) const
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);
		float eta = 1.0f / ior;

		//float reflectance;
		//float cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		float spec_prob = reflectance;

		return cos_o * ipi * (1.0f - spec_prob);
	}
	//vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color) const
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		//version 1
		//good
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		//float reflectance, cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		vec3 weight = ipi * cos_o * (1.0f - reflectance) * color;

		if (scaleSigma.maxc() > 0.0f)
		{

			weight.x *= expf(scaleSigma.x * (-1.0f / cos_o - 1.0f / cos_i));
			weight.y *= expf(scaleSigma.y * (-1.0f / cos_o - 1.0f / cos_i));
			weight.z *= expf(scaleSigma.z * (-1.0f / cos_o - 1.0f / cos_i));
		}

		return weight;
		//return ipi * cos_o * (1.0f - reflectance) * color;*/

		//version 2
		/*float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float Fi = Dielectric_Reflectance(eta, cos_i);

		if (cos_i * cos_o >= eps)
		return vec3(Fi);
		else
		{
		vec3 weight = ipi * cos_o * (1.0f - Fi) * color;

		if (scaleSigma.maxc() > 0.0f)
		{

		weight.x *= expf(scaleSigma.x * (-1.0f / cos_o - 1.0f / cos_i));
		weight.y *= expf(scaleSigma.y * (-1.0f / cos_o - 1.0f / cos_i));
		weight.z *= expf(scaleSigma.z * (-1.0f / cos_o - 1.0f / cos_i));
		}
		return weight;
		}*/

	}

	bool isSpecular() const
	{
		return false;
	}
};

bool checkReflection(const vec3& wi, const vec3& wo)
{
	//float value = (wi.dot(wo) - 1.0f);
	//return abs(value) < 0.001f;

	float value = wi.z * wo.z - wi.x * wo.x - wi.y * wi.y - 1.0f;

	return abs(value) < 0.001f;
}

//Very Good!!!
class Plastic_Complex : public BSDF_Sampling
{
public:
	float ior = 1.7f;
	float thickness = 1.0f;
	vec3 sigmaA = vec3(0.0f, 0.0f, 0.0f);

	float diffuseFresnel;
	float averageTransmittance = 1.0f;
	vec3 scaleSigmaA = vec3(0.0f);

	Plastic_Complex() { Prepare_For_Render(); }
	Plastic_Complex(float ior_, float thickness_) : ior(ior_), thickness(thickness_) { Prepare_For_Render(); }

	//setup for rendering
	void Prepare_For_Render()
	{
		scaleSigmaA = thickness * sigmaA;
		averageTransmittance *= expf(-2.0f * scaleSigmaA.average());

		diffuseFresnel = Compute_Diffuse_Fresnel(ior, 1000000);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float eta = 1.0f / ior;

		float abs_cos_i = abs(cos_i);

		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrateWeight = averageTransmittance * (1.0f - Fi);

		float specularWeight = Fi;

		float specular_prob = specularWeight / (specularWeight + substrateWeight);

		if (randf() < specular_prob)
		{
			vec3 direction_out = Reflect(direction_in, n);
			float weight = Fi / specular_prob;

			sampling = Sampling_Struct(direction_out, vec3(weight), specular_prob, false);
		}
		else
		{
			vec3 direction_out = cosine_hemisphere_Sampling_with_n(n);

			float cos_o = direction_out.dot(n);

			float abs_cos_o = abs(cos_o);

			float Fo = Dielectric_Reflectance(eta, cos_o);

			vec3 weight = (1.0f - Fi) * (1.0f - Fo) * eta * eta * (color / (vec3(1.0f) - color * diffuseFresnel));



			if (scaleSigmaA.maxc() > 0.0f)
			{
				weight.x *= expf(scaleSigmaA.x * (-1.0f / cos_o - 1.0f / cos_i));
				weight.y *= expf(scaleSigmaA.y * (-1.0f / cos_o - 1.0f / cos_i));
				weight.z *= expf(scaleSigmaA.z * (-1.0f / cos_o - 1.0f / cos_i));
			}

			float sample_pdf = abs_cos_o * ipi * (1.0f - specular_prob);

			float inv_diff_prob = 1.0f / (1.0f - specular_prob);

			weight *= inv_diff_prob;

			sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		float abs_cos_i = abs(cos_i);
		float abs_cos_o = abs(cos_o);


		float eta = 1.0f / ior;

		float Fi = Dielectric_Reflectance(eta, cos_i);
		float Fo = Dielectric_Reflectance(eta, cos_o);

		bool reflection = checkReflection(direction_in, direction_out);

		//if (cos_i * cos_o > 0.0f)
		if (reflection)
		{
			return vec3(Fi);
		}
		else
		{
			vec3 brdf = (1.0f - Fi) * (1.0f - Fo) * eta * eta * abs_cos_o * ipi * (color / (1.0f - color * diffuseFresnel));

			if (scaleSigmaA.maxc() > 0.0f)
			{
				brdf.x *= expf(scaleSigmaA.x * (-1.0f / cos_o - 1.0f / cos_i));
				brdf.y *= expf(scaleSigmaA.y * (-1.0f / cos_o - 1.0f / cos_i));
				brdf.z *= expf(scaleSigmaA.z * (-1.0f / cos_o - 1.0f / cos_i));
			}
			return brdf;
		}
		return vec3(0.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return 1.0f;

		float eta = 1.0f / ior;

		//float cos_i = -direction_in.dot(n);
		//float cos_o = direction_out.dot(n);

		float abs_cos_i = abs(cos_i);
		float abs_cos_o = abs(cos_o);

		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrateWeight = averageTransmittance * (1.0f - Fi);

		float specularWeight = Fi;

		float specularprob = (specularWeight) / (specularWeight + substrateWeight);

		bool reflection = cos_i * cos_o >= 0.0f; //checkReflection(direction_in, direction_out);

												 //if (cos_i * cos_o > 0.0f)
		if (reflection)
			return specularprob;
		else
			abs_cos_o * (1.0f - specularprob);
	}
	bool isSpecular() const
	{
		return false;
	}
};

//Very Good Too!!!
class Plastic_Complex2 : public BSDF_Sampling
{
public:
	float ior = 1.7f;
	float thickness = 0.0f;
	vec3 sigmaA = vec3(0.0f, 0.0f, 0.0f);

	float diffuseFresnel;
	float averageTransmittance;
	vec3 scaleSigmaA;

	Plastic_Complex2() {}
	Plastic_Complex2(float ior_, float thickness_) : ior(ior_), thickness(thickness_) { RenderVender(); }

	//setup for rendering
	void RenderVender()
	{
		scaleSigmaA = thickness * sigmaA;
		averageTransmittance *= expf(-2.0f * scaleSigmaA.average());

		diffuseFresnel = Compute_Diffuse_Fresnel(ior, 1000000);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float eta = 1.0f / ior;

		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrateWeight = averageTransmittance * (1.0f - Fi);

		float specularWeight = Fi;

		float specular_prob = specularWeight / (specularWeight + substrateWeight);

		if (randf() < specular_prob)
		{
			vec3 direction_out = Reflect(direction_in, n);
			float weight = Fi / specular_prob;

			sampling = Sampling_Struct(direction_out, vec3(weight), specular_prob, false);
		}
		else
		{
			vec3 direction_out = cosine_hemisphere_Sampling_with_n(n);

			float cos_o = direction_out.dot(n);
			float Fo = Dielectric_Reflectance(eta, cos_o);

			vec3 weight = (1.0f - Fi) * (1.0f - Fo) * eta * eta * (color / (vec3(1.0f) - color * diffuseFresnel));



			if (scaleSigmaA.maxc() > 0.0f)
			{
				weight.x *= expf(scaleSigmaA.x * (-1.0f / cos_o - 1.0f / cos_i));
				weight.y *= expf(scaleSigmaA.y * (-1.0f / cos_o - 1.0f / cos_i));
				weight.z *= expf(scaleSigmaA.z * (-1.0f / cos_o - 1.0f / cos_i));
			}

			float sample_pdf = cos_o * ipi * (1.0f - specular_prob);

			float inv_diff_prob = 1.0f / (1.0f - specular_prob);

			weight *= inv_diff_prob;

			sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		float eta = 1.0f / ior;

		float Fi = Dielectric_Reflectance(eta, cos_i);
		float Fo = Dielectric_Reflectance(eta, cos_o);

		bool reflection = checkReflection(direction_in, direction_out);

		//if (cos_i * cos_o > 0.0f)
		if (reflection)
		{
			return vec3(Fi);
		}
		else
		{
			vec3 brdf = (1.0f - Fi) * (1.0f - Fo) * eta * eta * cos_o * ipi * (color / (1.0f - color * diffuseFresnel));

			if (scaleSigmaA.maxc() > 0.0f)
			{
				brdf.x *= expf(scaleSigmaA.x * (-1.0f / cos_o - 1.0f / cos_i));
				brdf.y *= expf(scaleSigmaA.y * (-1.0f / cos_o - 1.0f / cos_i));
				brdf.z *= expf(scaleSigmaA.z * (-1.0f / cos_o - 1.0f / cos_i));
			}
			return brdf;
		}
		return vec3(0.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return 1.0f;

		float eta = 1.0f / ior;

		//float cos_i = -direction_in.dot(n);
		//float cos_o = direction_out.dot(n);

		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrateWeight = averageTransmittance * (1.0f - Fi);

		float specularWeight = Fi;

		float specularprob = (specularWeight) / (specularWeight + substrateWeight);

		bool reflection = cos_i * cos_o >= 0.0f; //checkReflection(direction_in, direction_out);

												 //if (cos_i * cos_o > 0.0f)
		if (reflection)
			return specularprob;
		else
			cos_o * (1.0f - specularprob);
	}
	bool isSpecular() const
	{
		return false;
	}
};

//Rough Plastic

class Rough_Plastic_Diffuse : public BSDF_Sampling
{
public:
	float ior;
	//float inv_ior;
	float roughness;
	Microfacet_Distribution dist = GGX;
	//0.2f
	vec3 spec_color = vec3(0.4f, 0.4f, 0.4f);//lamp
	
	Rough_Plastic_Diffuse() {}
	Rough_Plastic_Diffuse(float ior_, float roughness_) : ior(ior_), roughness(roughness_) {}
	Rough_Plastic_Diffuse(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) {}
	
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		//add this part to remove negative cos which cause blue stripe
		if (cos_i <= eps)
		{
			//cos_i = abs(cos_i);
			//sampling = Sampling_Struct();
			return false;
		}

		float eta = 1.0f / ior;

		//float eta = inv_ior;

		//vec3 m = ggx_sample_H(n, roughness);

		vec3 m = Microfacet_Sample(dist, n, roughness);

		//vec3 coord = ggx_sample_H(n, roughness);

		//onb local(n);

		//vec3 m(local.u * coord.x + local.v * coord.y + n * coord.z);

		//vec3 m = GGX_Sample_H(n, roughness);
		//m.normalize();

		//vec3 m = ggx_sample_H1(n, roughness);

		//m.normalize();

		//vec3 m = GGX_Sample_H_with_N(roughness, n);

		//float u1 = randf();
		//float u2 = randf();

		//vec3 m = visual_GGX_Sample_H(direction_in, u1, u2);

		float cos_t;
		//float F = Fresnel_Dielectric(eta, -direction_in.dot(m), cos_t);// Dielectric_Reflectance(eta, -direction_in.dot(m));

		float cos_i_m = -direction_in.dot(m);

		//newly add
		//if (cos_i_m <= eps)
		//	return false;

		//old
		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float F = Dielectric_Reflectance(eta, cos_i);

		//float F = Dielectric_Reflectance(eta, -direction_in.dot(m));

		vec3 direction_out;
		//float sample_pdf;// = pdf(direction_in, direction_out, n);

		bool is_specular;
		if (randf() < F)
			//if(randf() < 0.5f)
		{
			//original
			direction_out = Reflect(direction_in, m);
			
			//optimize but slower
			//direction_out = (direction_in + 2.0f * cos_i_m * m).norm();
			
			//sample_pdf = pdf(direction_in, direction_out, n);// *F;// / F;

			/*if (direction_out.dot(n) <= 0.0f)
			{
			sampling = Sampling_Struct();
			return false;
			}*/
			//sample_pdf * F;

			is_specular = true;//original false
			//is_specular = false;
		}
		else
		{
			//onb local(m);//onb local(n) cause false reflection on lower window bar in bathroom scene
			//vec3 coord = cosine_hemisphere_Sampling();

			//direction_out = (coord.x * local.u + coord.y * local.v + coord.z * local.w).norm();

			//sample_pdf = pdf(direction_in, direction_out, n);// *(1.0f - F);// / (1.0f - F);

			direction_out = cosine_hemisphere_Sampling_with_n(n);

			//original
			//is_specular = true;

			is_specular = false;//orignal true
		}

		isReflect = true;


		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		if (sample_pdf <= eps)
		{
			//sampling = Sampling_Struct();
			return false;
		}

		vec3 weight = eval(direction_in, direction_out, n, original_n, color);

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, is_specular);
		//sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, false);
		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//add this one to remove blue stripe
		if (cos_i <= eps || cos_o <= eps)// <= 0.0f
			return vec3(0.0f);

		/*if (cos_i <= 0.0f || cos_o <= 0.0f)
		{
		cos_i = abs(cos_i);
		cos_o = abs(cos_o);
		}*/
		vec3 m = (-direction_in + direction_out).norm();

		//float eta = 1.0f / ior;
		//float F = Dielectric_Reflectance(eta, direction_in.dot(m));

		//float cos_i_m = -direction_in.dot(m);
		//float eta = cos_i_m > 0.0f ? ior : 1.0f / ior;
		//cos_i_m = abs(cos_i_m);

		float cos_i_m = -direction_in.dot(m);

		//newly add
		//if (cos_i_m <= eps)
		//	return vec3(0.0f);

		float eta = 1.0f / ior;
		float cos_t;

		//old
		float F = Dielectric_Reflectance(eta, cos_i_m);//Fresnel_Dielectric(eta, cos_i_m, cos_t);// Dielectric_Reflectance(eta, cos_i_m);

		//float F = Dielectric_Reflectance(eta, cos_i);
													   //float D = ggx_D(m, n, roughness);//, m, n);

													   //float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = spec_color * (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		return spec_brdf + diff_brdf;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= eps || cos_o <= eps)//<=0.0f
			return 0.0f;

		vec3 m = (-direction_in + direction_out).norm();

		float eta = 1.0f / ior;

		//newly add
		float cos_i_m = -direction_in.dot(m);

		//if (cos_i_m <= eps)
		//	return 0.0f;
		
		//old
		float F = Dielectric_Reflectance(eta, abs(cos_i_m));
		//float F = Dielectric_Reflectance(eta, cos_i);

		//float F = Dielectric_Reflectance(eta, -direction_in.dot(m));

		//original
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * direction_out.dot(m));
		//float diff_pdf = cos_o * ipi;

		//multiply cos_o so the spec_pdf will be larger and reduce firefly

		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * direction_out.dot(m) * cos_o);


		//original good
		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m) * cos_o);

		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m));

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness) / (4.0f * direction_out.dot(m));

		//if (spec_pdf <= eps)
		//	return 0.0f;

		float diff_pdf = cos_o * ipi;

		//failed
		//float cos_o_m = direction_out.dot(m);
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * cos_o_m);
		//float diff_pdf = cos_o_m * ipi;

		//return spec_pdf * F + diff_pdf * (1.0f - F);

		float pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		if (pdf <= eps)
			return 0.0f;

		return pdf;
	}
	bool isSpecular() const
	{
		return false;
	}
};



class Rough_Plastic_Specular : public BSDF_Sampling
{
public:
	float ior;
	float roughness;

	Microfacet_Distribution dist = GGX;

	Rough_Plastic_Specular() {}
	Rough_Plastic_Specular(float ior_, float roughness_) : ior(ior_), roughness(roughness_) {}
	Rough_Plastic_Specular(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= eps)
		{
			sampling = Sampling_Struct();
			return false;
		}

		float eta = 1.0f / ior;

		//vec3 m = ggx_sample_H(n, roughness);

		vec3 m = Microfacet_Sample(dist, n, roughness);

		//vec3 coord = ggx_sample_H(n, roughness);

		//onb local(n);

		//vec3 m(local.u * coord.x + local.v * coord.y + n * coord.z);

		//vec3 m = GGX_Sample_H(n, roughness);
		//m.normalize();

		//vec3 m = ggx_sample_H1(n, roughness);

		//m.normalize();

		//vec3 m = GGX_Sample_H_with_N(roughness, n);

		//float u1 = randf();
		//float u2 = randf();

		//vec3 m = visual_GGX_Sample_H(direction_in, u1, u2);

		float cos_t;
		//float F = Fresnel_Dielectric(eta, -direction_in.dot(m), cos_t);// Dielectric_Reflectance(eta, -direction_in.dot(m));

		float F = Dielectric_Reflectance(eta, -direction_in.dot(m));

		vec3 direction_out;
		//float sample_pdf;// = pdf(direction_in, direction_out, n);

		bool is_specular;
		if (randf() < F)
			//if(randf() < 0.5f)
		{
			direction_out = Reflect(direction_in, m);
			//sample_pdf = pdf(direction_in, direction_out, n);// *F;// / F;

			/*if (direction_out.dot(n) <= 0.0f)
			{
			sampling = Sampling_Struct();
			return false;
			}*/
			//sample_pdf * F;

			is_specular = true;
		}
		else
		{
			//onb local(m);//onb local(n) cause false reflection on lower window bar in bathroom scene
			//vec3 coord = cosine_hemisphere_Sampling();

			//direction_out = (coord.x * local.u + coord.y * local.v + coord.z * local.w).norm();

			//sample_pdf = pdf(direction_in, direction_out, n);// *(1.0f - F);// / (1.0f - F);

			direction_out = cosine_hemisphere_Sampling_with_n(n);

			is_specular = true;//true
		}

		isReflect = true;


		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		if (sample_pdf < eps)
		{
			sampling = Sampling_Struct();
			return false;
		}

		vec3 weight = eval(direction_in, direction_out, n, original_n, color);

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, is_specular);
		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= eps || cos_o <= eps)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		//float eta = 1.0f / ior;
		//float F = Dielectric_Reflectance(eta, direction_in.dot(m));

		//float cos_i_m = -direction_in.dot(m);
		//float eta = cos_i_m > 0.0f ? ior : 1.0f / ior;
		//cos_i_m = abs(cos_i_m);

		float cos_i_m = -direction_in.dot(m);
		float eta = 1.0f / ior;
		float cos_t;

		float F = Dielectric_Reflectance(eta, cos_i_m);//Fresnel_Dielectric(eta, cos_i_m, cos_t);// Dielectric_Reflectance(eta, cos_i_m);

													   //float D = ggx_D(m, n, roughness);//, m, n);

													   //float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		return spec_brdf + diff_brdf;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= eps || cos_o <= eps)
			return 1.0f;

		vec3 m = (-direction_in + direction_out).norm();

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, -direction_in.dot(m));

		//original
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * direction_out.dot(m));
		//float diff_pdf = cos_o * ipi;

		//multiply cos_o so the spec_pdf will be larger and reduce firefly

		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * direction_out.dot(m) * cos_o);


		//original good
		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m) * cos_o);

		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m));

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness);

		float diff_pdf = cos_o * ipi;

		//failed
		//float cos_o_m = direction_out.dot(m);
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * cos_o_m);
		//float diff_pdf = cos_o_m * ipi;

		//return spec_pdf * F + diff_pdf * (1.0f - F);

		float pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		if (pdf <= eps)//1e-6
			return 1.0f;
		return pdf;
	}

	bool isSpecular() const
	{
		return true;
	}
};




//Bài học xương máu rút ra từ Rough Conductor

//1 khi nó là specular reflection thì phải để là specular = true
//nếu để false thì sẽ ko bao giờ ra

//2 không tuyệt đối tuân theo hướng dẫn vì hướng dẫn có thể sai
//ví dụ ở đây specular = true mà cho ra false

//3 nếu reflect mặt micro facet thì nó luôn là sepcular

//4 mặt h sample từ microfacet phải tính bằng onb
//và Phải LUÔN LUÔN tính bằng ONB
//nếu dùng trực tiếp hoặc dùng trực tiếp + n
//thì KHÔNG THỂ CỨU VÃN

vec3 positive_vector(const vec3& v)
{
	return vec3(abs(v.x), abs(v.y), abs(v.z));
}



class Rough_Conductor : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior = Ior_List[2];

	Microfacet_Distribution dist = GGX;

	Rough_Conductor() {}
	Rough_Conductor(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Rough_Conductor(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);

		//if (cos_i <= 0.0f)
		//	return false;

		if (cos_i <= eps)
		//if(cos_i <= 0.0f)
			return false;

		//vec3 h = ggx_sample_H(n, roughness);

		//float h_pdf = ggx_pdf(h, n, roughness);

		vec3 h = Microfacet_Sample(dist, n, roughness);

		float h_pdf = Microfacet_pdf(dist, h, n, roughness);

		float cos_i_h = -direction_in.dot(h);

		//newly add
		//if (cos_i_h <= eps)
		//	return false;

		vec3 direction_out = Reflect(direction_in, h);//(direction_in + 2.0f * cos_i_h * h).norm();


													  //float G = ggx_G(direction_in, direction_out, h, n, roughness);

													  //float D = ggx_D(h, n, roughness);

		//old
		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);
		
		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);

		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		//float x = conductor_Reflectance(ior.eta.x, ior.k.x, cos_i);
		//float y = conductor_Reflectance(ior.eta.y, ior.k.y, cos_i);
		//float z = conductor_Reflectance(ior.eta.z, ior.k.z, cos_i);

		//vec3 F = vec3(x, y, z);



		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);

		//cos_i_h = cos_i_h >= 0.0f ? cos_i_h : 0.0f;

		//if (cos_i_h <= 0.0f)
		//	return false;

		//cos_i_h = abs(cos_i_h);


		float sample_pdf = h_pdf / (4.0f * cos_i_h);

		if (sample_pdf <= eps)
			return false;

		//original
		vec3 weight = cos_i_h * D * G / (h_pdf * cos_i);
		//vec3 weight = color * (F * D * G) * cos_i_h / (h_pdf * cos_i);

		//modify
		//vec3 weight = color * (D * G) * cos_i_h / (h_pdf * cos_i);

		sampling = Sampling_Struct(direction_out, color * F * weight, sample_pdf, true);

		//sampling = Sampling_Struct(direction_out, color * F * weight, sample_pdf, false);

		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return vec3(0.0f);

		if (cos_i <= eps || cos_o <= eps)
		//if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		//newly add
		//if (cos_i_h <= eps)
		//	return false;

		//if (cos_i_h <= 0.0f)
		//	return vec3(0.0f);

		//float x = conductor_Reflectance(ior.eta.x, ior.k.x, cos_i);
		//float y = conductor_Reflectance(ior.eta.y, ior.k.y, cos_i);
		//float z = conductor_Reflectance(ior.eta.z, ior.k.z, cos_i);

		//vec3 F = vec3(x, y, z);

		
		//old
		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);


		//float G = ggx_G(direction_in, direction_out, h, n, roughness);

		//float D = ggx_D(h, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		return color * F * D * G / (4.0f * cos_i);

		//float cos_i = -direction_in.dot(n);

		/*
		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);

		return color * F;
		*/
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		vec3 h = (-direction_in + direction_out).norm();

		//float sample_pdf = ggx_pdf(h, n, roughness);

		float alpha = roughness;

		//float sample_pdf = Microfacet_pdf(dist, h, n, alpha);

		//return abs(sample_pdf / (4.0f * -direction_in.dot(h)));

		float pdf_h = Microfacet_pdf(dist, h, n, alpha);

		//if (pdf_h <= eps)
		//	return 0.0f;

		float cos_i_h = -direction_in.dot(h);

		//if (cos_i_h <= eps)
		//	return 0.0f;

		//float sample_pdf = (pdf_h / (4.0f * -direction_in.dot(h)));

		float sample_pdf = (pdf_h / (4.0f * -direction_in.dot(h)));

		//if (sample_pdf < 1e-3)
		//	return 1.0f;

		if (sample_pdf <= eps)
			return 0.0f;
		return sample_pdf;
	}

	bool isSpecular() const
	{
		return false;
	}
};


class Final_Rough_Conductor : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor() {}
	Final_Rough_Conductor(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Final_Rough_Conductor(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float alpha2 = roughness;// *roughness;


		vec3 m = Microfacet_Sample(dist, n, alpha2);

		//vec3 m = ggx_sample_H(n, alpha2);

		//vec3 m = GGX_Sample_H(n, alpha2);

		//vec3 m = sampleGGXVNDF(direction_in, n, alpha2, alpha2);

		vec3 direction_out = Reflect(direction_in, m);

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return false;

		float cos_o_m = direction_out.dot(m);


		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		float cos_m_n = m.dot(n);

		//Good
		vec3 weight = color * G * cos_o_m / (cos_m_n * cos_i);

		//vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		//weight = positive_vector(weight);

		//weight = bsdf failed
		//vec3 weight = color * D * G / (4.0f * cos_i);

		float pdf = D * cos_m_n * J;

		//add
		//if (pdf < 0.0f)
		//	pdf = -pdf;
		sampling = Sampling_Struct(direction_out, weight, pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness;// *roughness;



								 //float D = ggx_D(m, n, alpha2);

								 //float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//good
		return color * (D * G) / (4.0f * cos_i);

		//vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

		//return eval;

		//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		vec3 m = (-direction_in + direction_out).norm();

		float alpha2 = roughness;// *roughness;

								 //float D = ggx_D(m, n, alpha2);

		float D = Microfacet_pdf(dist, m, n, alpha2);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		float sample_pdf = D * cos_m_n * J;;
		//return D * cos_m_n * J;

		//if (sample_pdf < 0.0f)
		//	sample_pdf = -sample_pdf;

		return sample_pdf;
	}

	bool isSpecular() const
	{
		return false;
	}
};


//equation in page 8 under figure 9
//this equation is used to reduce the weight at grazing angle
//we have high specular with low sampling probability (or pdf)
//and cause firefly

//https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
//alpha_b = (1.2f - 0.2f * sqrt(i.dot(n))) * alpha
float change_roughness(const float& r, const float& cos_i)
{
	//float alpha = (1.2f - 0.2f * sqrt14(abs(cos_i))) * r;

	//return alpha * alpha;

	//khu dung sqrt voi so am hinh se ra mau tim
	//when use sqrt with negative number, it will result in purple color

	//wrong, since cos_i can be negative
	//return (1.2f - 0.2f * sqrt14((cos_i))) * r;

	return 1.2f - 0.2f * sqrt14(abs(cos_i)) * r;
}

//clamp value to 0, 1 range 
//should use the name clamp instead of saturate to avoid confusion with hsl saturate value
//https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/saturate
float saturate(const float& value)
{
	return min(1.0f, max(0.0f, value));
}


float D(const vec3& m, const vec3& n, const float& alpha)
{
	float cos_m = m.dot(n);

	if (cos_m <= 0.0f)
		return 0.0f;

	float alpha2 = alpha * alpha;

	float cos_m2 = cos_m * cos_m;

	float tan_m2 = (1.0f - cos_m2) / cos_m2;

	float cos_m4 = cos_m2 * cos_m2;

	float x = alpha2 + tan_m2;

	return alpha2 / (pi * cos_m4 * x * x);
}

float G1(const vec3& v, const vec3& m, const vec3& n, const float& alpha)
{
	float cos_vm = v.dot(m);
	float cos_vn = v.dot(n);

	if (cos_vm * cos_vn <= 0.0f)
		return 0.0f;

	float cos_vn2 = cos_vn * cos_vn;

	float tan_theta2 = (1.0f - cos_vn2) / cos_vn2;

	float alpha2 = alpha * alpha;

	2.0f / (1.0f + sqrt14(1.0f + alpha2 * tan_theta2));
}

float G(const vec3& vi, const vec3& vo, const vec3& m, const vec3& n, const float& alpha)
{
	return G1(vi, m, n, alpha) * G1(vo, m, n, alpha);
}

vec3 sample_D(const vec3& n, const float& alpha)
{
	float u1 = randf();
	float u2 = randf();

	float tan_theta2 = alpha * alpha * u1 / (1.0f - u1);
	float phi = 2.0f * pi * u2;

	float cos_theta = sqrt14(1.0f / (1.0f + tan_theta2));

	float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

	onb local(n);

	vec3 h = (local.u * cosf(phi) + local.v * sinf(phi)) * sin_theta + n * cos_theta;

	return h.norm();
}

float sample_D_pdf(const vec3& m, const vec3& n, const float& alpha)
{
	return D(m, n, alpha) * m.dot(n);
}

class ThinDielectric : public BSDF_Sampling
{
public:
	float ior = 1.0f;

	ThinDielectric() {}
	ThinDielectric(float ior_) : ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float eta = 1.0f / ior;
		float R = Dielectric_Reflectance(eta, abs(direction_in.dot(n)));

		float T = 1.0f - R;

		//R' = R + TRT + TR^3T +...
		//R' = R + T * T * (R + R^3 + R^5 + R^7...)

		//Taylor Series 1 + x + x^2 +... = 1 / (1 - x)

		//R * (1 + R^2 + R^4 + R^6) = R / (1 - R^2) here x = R^2

		//R' = R + T * T * R / (1 - R * R)

		if (R < 1)
			R += T * T * R / (1.0f - R * R);

		if (randf() < R)
		{
			isReflect = true;
			sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), R, true);
			return true;
		}
		else
		{
			isReflect = false;
			sampling = Sampling_Struct(direction_in, vec3(1.0f), 1.0f - R, true);
			return true;
		}
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(1.0f);
		/*
		float cos_i = direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = cos_i >= 0 ? ior : 1.0f / ior;

		cos_i = abs(cos_i);

		float reflectance, cos_t;

		float R = Dielectric_Reflectance(eta, cos_i, cos_t);

		float T = 1.0f - R;

		if (R < 1)
		R += T * T * R / (1.0f - R * R);

		if (cos_i * cos_o >= 0.0f)
		return vec3(R);
		else
		return vec3(1.0f - R);
		*/
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		//return 1.0f;

		float cos_i = direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = cos_i >= 0 ? ior : 1.0f / ior;

		cos_i = abs(cos_i);

		float cos_t;

		float R = Dielectric_Reflectance(eta, cos_i, cos_t);

		float T = 1.0f - R;

		if (R < 1)
			R += T * T * R / (1.0f - R * R);

		if (cos_i * cos_o >= 0.0f)
			return R;
		else
			return 1.0f - R;

	}
	bool isSpecular() const
	{
		return true;
	}
};

class ThinSheet : public BSDF_Sampling
{
public:
	float ior = 1.7f;
	float thickness = 0.5f;
	float sigmaA = 0.0f;
	bool use_Interference = false;

	ThinSheet() {}
	ThinSheet(float ior_, float thickness_, float sigmaA_, bool use_Interference_) : ior(ior_), thickness(thickness_), sigmaA(sigmaA_), use_Interference(use_Interference_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		//vec3 direction_out = vec3(direction_in.x * n.x, direction_in.y * n.y, direction_in.z * n.z);

		//direction_out.normalize();
		//failed
		//vec3 direction_out = direction_in;

		//onb local(n);

		//vec3 direction_out(-direction_in.x * local.u - direction_in.y *local.v + direction_in.z *local.w);

		//direction_out.normalize();

		//vec3 direction_out = Reflect(direction_in, n);

		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0 ? ior : 1.0f / ior;

		cos_i = abs(cos_i);

		float cos_t;

		float F = Dielectric_Reflectance(eta, cos_i, cos_t);

		//float F = Fresnel_Dielectric(eta, cos_i, cos_t);

		vec3 direction_out;// = Reflect(direction_in, n);


		if (randf() < F)
		{
			isReflect = true;
			direction_out = Reflect(direction_in, n);
		}
		else
		{
			isReflect = false;
			//direction_out = Refract(direction_in, n, eta, cos_t);

			direction_out = Refract(direction_in, n, eta, cos_i, cos_t);

			//direction_out = (eta * direction_in + (eta * cos_i - cos_t) * n).norm();
		}
		float sample_pdf = 1.0f;

		if (sigmaA == 0.0f && !use_Interference)
		{
			sampling = Sampling_Struct(direction_out, vec3(1.0f), 1.0f, true);
			return true;
		}

		vec3 weight;
		//float cos_t;


		if (use_Interference)
			weight = ThinFilmReflectanceInteference(1.0f / ior, abs(direction_in.dot(n)), 500.0f * thickness, cos_t);
		else
			weight = vec3(ThinFilmReflectance(1.0f / ior, abs(direction_in.dot(n)), cos_t));

		vec3 transmitance = vec3(1.0f) - weight;

		if (sigmaA != 0.0f && cos_t > 0.0f)
			transmitance *= expf(-sigmaA * (thickness * 2.0f / cos_t));

		float value = 1.0f - transmitance.average();

		float i_value = 1.0f / value;

		weight *= i_value;

		sampling = Sampling_Struct(direction_out, weight, 1.0f, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		vec3 transmittance;

		float cos_t;

		if (use_Interference)
			transmittance = vec3(1.0f) - ThinFilmReflectanceInteference(1.0f / ior, abs(direction_in.dot(n)), thickness * 500.0f, cos_t);
		else
			transmittance = vec3(1.0f) - ThinFilmReflectance(1.0f / ior, abs(direction_in.dot(n)), cos_t);

		if (sigmaA != 0.0f && cos_t > 0.0f)
			transmittance *= expf(-sigmaA * (thickness * 2.0f / cos_t));

		return transmittance;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;//check Reflection Condition, or dot product of direction_in and direction_out if too small, pdf = 0.0f
					//however return pdf 0.0f cause error and thin sheet is pure specular so this function is
					//not needed any way
	}
	bool isSpecular() const
	{
		return true;
	}
};


//ior qua lon se dan den total internal refelection
//lam cho ham sample return false
class SmoothCoat : public BSDF_Sampling
{
public:
	float ior = 1.1f;
	//float thickness = 1.0f;
	//vec3 sigmaA = vec3(0.0f);

	float thickness = 5.0f;
	vec3 sigmaA = vec3(0.1f, 0.1f, 0.1f);

	BSDF_Sampling* submaterial = new Plastic_Diffuse(1.7f);//new Rough_Conductor_Extended(0.1, Ior_List[2]);

	vec3 sub_color = vec3(0.2, 0.6, 0.9);

	//Microfacet_Distribution sub_material_dist = GGX;

	//vec3 sub_color = vec3(0.811, 0.933, 0.98);

	//preset
	float average_transmitance;
	vec3 scaleSigma;

	SmoothCoat() { prepare_before_render(); }
	SmoothCoat(float ior_, float thickness_, vec3 sigmaA_) : ior(ior_), thickness(thickness_), sigmaA(sigmaA_) { prepare_before_render(); };

	void prepare_before_render()
	{
		scaleSigma = thickness * sigmaA;
		average_transmitance = expf(-2.0f * scaleSigma.average());
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		if (-direction_in.dot(n) <= 0.0f)
			return false;

		float cos_i_original = direction_in.dot(original_n);

		float cos_i = abs(cos_i_original);

		float eta = 1.0f / ior;

		float cos_ti;

		float Fi = Dielectric_Reflectance(eta, cos_i, cos_ti);

		float substrateWeight = average_transmitance * (1.0f - Fi);

		float specularWeight = Fi;

		float specular_prob = specularWeight / (specularWeight + substrateWeight);

		vec3 direction_out;
		vec3 sample_weight;
		float sample_pdf;
		bool specular;

		if (randf() < specular_prob)
		{
			direction_out = Reflect(direction_in, n);
			sample_pdf = specular_prob;
			sample_weight = vec3(Fi / specular_prob);
			specular = true;
		}
		else
		{
			vec3 substrate_direction_in = (eta * direction_in + (eta * cos_i - cos_ti) * n).norm();

			Sampling_Struct sub_sampling;
			bool isReflect;

			vec3 sub_mat_color = sub_color;

			bool sample_sub = submaterial->sample(substrate_direction_in, n, original_n, sub_sampling, isReflect, sub_mat_color);

			if (!sample_sub)
				return false;

			float cos_theta_substrate;// reflection of sub material 
			float cos_theta_to;//refraction of submaterial in to dieletric material above it

			cos_theta_substrate = abs(sub_sampling.direction.dot(n));

			//float Fo = Dielectric_Reflectance(ior, cos_sub_o);


			float sub_eta = ior;

			float Fo = Dielectric_Reflectance(sub_eta, cos_theta_substrate, cos_theta_to);


			if (Fo == 1.0f)
				return false;

			direction_out = Refract(sub_sampling.direction, n, ior, cos_theta_substrate, cos_theta_to);

			sample_weight = sub_sampling.weight * (1.0f - Fi) * (1.0f - Fo);

			if (scaleSigma.maxc() > 0.0f)
			{
				sample_weight.x *= expf(scaleSigma.x * (-1.0f / cos_theta_substrate - 1.0f / cos_ti));
				sample_weight.y *= expf(scaleSigma.y * (-1.0f / cos_theta_substrate - 1.0f / cos_ti));
				sample_weight.z *= expf(scaleSigma.z * (-1.0f / cos_theta_substrate - 1.0f / cos_ti));
			}

			float transmitance_prob = (1.0f - specular_prob);
			float inv_transmitance_prob = 1.0f / transmitance_prob;

			sample_weight *= inv_transmitance_prob;

			sample_pdf = sub_sampling.pdf;
			sample_pdf *= transmitance_prob;
			sample_pdf *= eta * eta * cos_theta_to / cos_theta_substrate;

			specular = sub_sampling.isSpecular;
		}

		sampling = Sampling_Struct(direction_out, sample_weight, sample_pdf, specular);

		//sampling = Sampling_Struct(direction_out, sample_weight, sample_pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float eta = 1.0f / ior;

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i = 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		bool reflect = cos_i * cos_o >= eps;

		float abs_cos_i = abs(cos_i);
		float abs_cos_o = abs(cos_o);

		float cos_theta_ti;
		float cos_theta_to;

		float Fi = Dielectric_Reflectance(eta, abs_cos_i, cos_theta_ti);
		float Fo = Dielectric_Reflectance(eta, abs_cos_o, cos_theta_to);

		if (reflect)
			return vec3(Fi);
		else
		{
			vec3 substrate_direction_in = Refract(direction_in, n, eta, abs_cos_i, cos_theta_ti);
			vec3 substrate_direction_out = Refract(direction_out, n, eta, abs_cos_o, cos_theta_to);

			float lapalacian = eta * eta * abs_cos_o / cos_theta_to;

			vec3 sub_material_color = sub_color;
			vec3 substrate_weight = submaterial->eval(substrate_direction_in, substrate_direction_out, n, original_n, sub_material_color);

			if (scaleSigma.maxc() > 0.0f)
			{
				substrate_weight.x *= expf(scaleSigma.x * (-1.0f / cos_theta_to - 1.0f / cos_theta_ti));
				substrate_weight.y *= expf(scaleSigma.y * (-1.0f / cos_theta_to - 1.0f / cos_theta_ti));
				substrate_weight.z *= expf(scaleSigma.z * (-1.0f / cos_theta_to - 1.0f / cos_theta_ti));
			}

			return lapalacian * (1.0f - Fi) * (1.0f - Fo) * substrate_weight;

			//else
			//	return vec3(0.0f);
		}
		return vec3(0.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float eta = 1.0f / ior;

		float cos_t;

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return 1.0f;

		float abs_cos_i = abs(cos_i);
		float abs_cos_o = abs(cos_o);

		float cos_theta_ti;
		float cos_theta_to;

		float Fi = Dielectric_Reflectance(eta, abs_cos_i, cos_theta_ti);
		float Fo = Dielectric_Reflectance(eta, abs_cos_o, cos_theta_to);

		vec3 substrate_direction_in = Refract(direction_in, n, eta, abs_cos_i, cos_theta_ti);
		vec3 substrate_direction_out = Refract(direction_out, n, eta, abs_cos_o, cos_theta_to);

		float substrate_weight = average_transmitance * (1.0f - Fi);
		float specular_weight = Fi;

		float specular_prob = specular_weight / (specular_weight + substrate_weight);

		if (randf() < specular_prob)
		{
			return specular_prob;
		}
		else
		{
			return submaterial->pdf(substrate_direction_in, substrate_direction_out, n, original_n) * (1.0f - specular_prob) * eta * eta * substrate_direction_out.dot(n) / cos_theta_to;
		}

	}
	bool isSpecular() const
	{
		return false;
	}
};
/*

//---------------------------A COMMUNIST DENOUNCEMENT----------------------------------------------

//----------------MUST READ IF YOU COME FROM VIET NAM OR ANY DICTATORSHIP------------------------

//-------ALSO MUST READ IF YOU COME FROM A FREE WORLD TO APPRECIATE HOW LUCKY YOU ARE---------


There are two ways to implement Transparent Material

1. Every Ray pass throught transparent material are stop at random,
which mean, the ray that allowed to pass carry full energy
the one not allowed to pass just stop there and contribute zero energy.

2. Every Ray are allowed to pass, but some of it energy is steal

//------------------------------------------

In "VIET NAM" 1 and 2 are common ways that the communist party use to steal from Viet Namese people

1. first is land confsicated, some one who is unlucky, mostly the farmer and poor have all the
land and property snatch away by the communist pigs.

2. second is "BOT", every traffic vehicles pass throught BOT have to pay fee
BOT is not the only tax that impose on Vietnamese, there are thousand of them
which make life so poor and miserable.

the one who speak out the true was imprison and torture to dead, one of the famous incident is

"DONG TAM" Land confiscated. I will tell you a summary just in case you dont know:

-There is a poor but peacefully small village name Dong Tam, 40km from Ha Noi, The Communist
want to rob the rice field from the villagers, the villagers was enraged, but they do not protest
or take arm against the goverment, instead they write letters to the head of the state

NGUYEN PHU TRONG and ask that old "Dick" to respect the villagers 's right to own their land,

Trong saw this as an act of Rebellion, so on 2am 9-1-2020, they bring 8000 policements
to storm mr LE DINH KINH house, who were 84 at that times, have 58 years communist party 
membership, and still faithfull, whole hearted with the party to his dead,

the police shot him to "DEAD" on his bed,

they "TORE HIS CHEST APART" to take out the "BULLET", took away all his sons and grandsons

as well as many family members.


RESULT: Two sons were sentence to death, the rest got life sentences or 5-15 years in prison.


"SO IF YOU READ THESE LINES AND LIVE IN A FREE WORLD, GO TO YOUR MOM AND DAD AND THANK THEM FOR
GIVING YOU SUCH PREVILIGE THAT ONLY A FEW PEOPLE THAT LIVE IN HISTORY CAN HAVE"

"VIETNAM, CHINA TODAY IS JUST THE RESEMBLE OF NAZI GERMANY AND SOVIET UNION, TWO OF WORST OF ALL TIME
BUT YET THE FREE WORLD STILL TOLERATE VIET NAM AND CHINA HUMAN RIGHT ABUSE, WHY? 

BECAUSE THE FREE WORLD ARE HAVING GOOD DEALS WITH DICTATORSHIP, THEY ARE MAKING TONS OF MONEY FROM
CHEAP LABOUR PRICES IN VIET NAM, CHINA... ". THEY HAVE THE POWER TO END THE WORLD SUFFERING, 

BUT THEY JUST DONT CARE.

BUT IF THE FREE WORLD KEEP IGNORING, THE OPRESSING REGIME WILL ONE DAY RISE AGAIN AND

WHEN THEY ARE STRONG ENOUGH,

THE FREE WORLD WILL BE NO MORE BUT A DISTANT MEMORY!

//Thank you for reading!

//God bless your soul

From Lord Of The Alerian

*/
/*
class Transparent : public BSDF_Sampling
{
public:
	float transparent = 0.95f;//0 = light block completely, 1 = light travel freely
	Transparent() {}
	Transparent(float transparent_) : transparent(transparent_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		if (randf() < transparent)
		{
			sampling = Sampling_Struct(direction_in, color, transparent, true);
			return true;
		}
		else
			return false;
	}
	bool sample_volume(const vec3& direction_in, vec3& n, vec3& original_n, float& t, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return color;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return transparent;
	}
	bool isSpecular() const
	{
		return true;
	}
};
*/
//object treated as light source
//however there are two many triangles in this object and we dont need to sample this light source
//so Light_Diffuse come to light
class Light_Diffuse : public BSDF_Sampling
{
public:
	vec3 Kd;
	Light_Diffuse() {}
	Light_Diffuse(vec3 Kd_) : Kd(Kd_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		return vec3(Kd);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		return 1.0f;
	}
	bool isSpecular() const
	{
		return true;
	}
};


//failed
float Lerp(const float& a, const float& b, const float& t)
{
	//return a * (1.0f - t) + b * t;

	return a + t * (b - a);
}

vec3 Lerp(const vec3& a, const vec3& b, const float& t)
{
	//return a * (1.0f - t) + b * t;

	return a + t * (b - a);
}

static float Saturate(const vec3& color)
{
	return 0.3f * color.x + 0.6f * color.y + 1.0f * color.z;
}

static vec3 Tint(const vec3& color)
{
	float Luminance = 0.3f * color.x + 0.6f * color.y + 1.0f * color.z;
	float inv_Luminance = 1.0f / Luminance;
	return Luminance > 0.0f ? color * inv_Luminance : vec3(1.0f);
}

static vec3 Sheen(const vec3& wi, const vec3& wo, const vec3& m, const vec3& sheenColor, const float& sheenTint, const vec3& color)
{
	float cos_i_m = -wi.dot(m);

	vec3 Tint_Color = Tint(color);

	return sheenColor * Lerp(vec3(1.0f), Tint_Color, sheenTint);

}
float GTR1(const float& HdotL, const float& a)
{
	if (a >= 1.0f)
		return ipi;
	float a2 = a * a;
	float t = 1.0f + (a2 - 1.0f) * HdotL * HdotL;
	return (a2 - 1) / (pi * logf(a2) * t);
}

float SmithGGX_G1(const vec3& w, const vec3& n, const float& a)
{
	float a2 = a * a;
	//float abs_cos_i = abs(w.dot(n));

	float cos_i = w.dot(n);
	float cos_i2 = cos_i * cos_i;

	return 2.0f / (1.0f + sqrt14(a2 + (1.0f - a2) * cos_i2));
}

float Schlick(const float& F0, const float& cos_theta)
{
	float R0 = 1.0f - cos_theta;

	float R0_2 = R0 * R0;

	return F0 + (1.0f - F0) * R0 * R0_2 * R0_2;
}

static float maxf2(float a, float b)
{
	return a > b ? a : b;
}

class Disney_Clear_Coat : public BSDF_Sampling
{
public:
	float alpha = 1.0f;

	float clearCoat_Weight = 0.2f;
	float clearCoat_Gloss = 0.22f;

	float metallic = 0.5f;
	float specTrans = 0.16f;

	vec3 clearCoatColor = vec3(0.8f, 0.6f, 0.2f);

	Disney_Clear_Coat() {}
	Disney_Clear_Coat(float alpha_, float clearCoat_Weight_, float clearCoat_Gloss_, float metallic_, float specTrans_, vec3 clearCoatColor_) :
		alpha(alpha_), clearCoat_Weight(clearCoat_Weight_), clearCoat_Gloss(clearCoat_Gloss_), metallic(metallic_), specTrans(specTrans_), clearCoatColor(clearCoatColor_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		float a = 0.25f;
		float a2 = a * a;

		float r0 = randf();
		float r1 = randf();

		float cos_theta = sqrt14(maxf2(0.0f, (1.0f - powf(a2, 1.0f - r0)) / (1.0f - a2)));
		float sin_theta = sqrt14(maxf2(0.0f, 1.0f - cos_theta * cos_theta));

		float phi = tau * r1;

		onb local(n);

		vec3 coord(sin_theta * cosf(phi), cos_theta, sin_theta * sinf(phi));

		vec3 m = (coord.x * local.u + coord.y * local.v + coord.z * n).norm();

		if (-direction_in.dot(m) < 0.0f)
			m = -m;

		vec3 direction_out = Reflect(direction_in, m);

		if (-direction_in.dot(direction_out) < 0.0f)
			return false;

		float clearCoatWeight = clearCoat_Weight;
		float clearCoatGloss = clearCoat_Gloss;

		float cos_m_n = m.dot(n);

		float cos_o_m = direction_out.dot(m);

		float abs_cos_i = abs(direction_in.dot(n));

		float abs_cos_o = abs(direction_out.dot(n));

		float d = GTR1(abs(cos_m_n), Lerp(0.1f, 0.001f, clearCoat_Gloss));
		float f = Schlick(0.04f, cos_o_m);
		float g = SmithGGX_G1(direction_in, n, 0.25f) * SmithGGX_G1(direction_out, n, 0.25f);

		float sample_pdf = d / (4.0f * abs(-direction_in.dot(m)));

		sampling.direction = direction_out;
		sampling.weight = 0.25f * clearCoatColor * clearCoatWeight * g * f * d;
		sampling.pdf = sample_pdf;
		sampling.isSpecular = false;

		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) const
	{
		float abs_cos_i = abs(direction_in.dot(n));
		float abs_cos_o = abs(direction_out.dot(n));

		vec3 m = (-direction_in + direction_out).norm();

		float abs_cos_m = abs(m.dot(n));

		float d = GTR1(abs_cos_m, Lerp(0.1f, 0.001f, alpha));

		float f = Schlick(0.04, -direction_in.dot(m));

		float gl = SmithGGX_G1(direction_in, n, 0.25f);
		float gv = SmithGGX_G1(direction_out, n, 0.25f);

		float forward_Pdf = d / (4.0f * abs_cos_i);
		float reverse_pdf = d / (4.0f * abs_cos_o);

		return 0.25f * clearCoatColor * d * f * gl * gv;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) const
	{
		float metallicBRDF = metallic;
		float specularBSDF = (1.0f - metallic) * specTrans;
		float dielectricBRDF = (1.0f - specTrans) * (1.0f - metallic);

		float specularWeight = metallicBRDF + dielectricBRDF;
		float transmissionWeight = specularBSDF;
		float diffuseWeight = dielectricBRDF;
		float clearcoatWeight = 1.0f * Saturate(clearCoatColor);

		float norm = 1.0f / (specularWeight + transmissionWeight + diffuseWeight + clearcoatWeight);

		//pSpecular = specularWeight     * norm;
		//pSpecTrans = transmissionWeight * norm;
		//pDiffuse = diffuseWeight      * norm;
		//pClearcoat = clearcoatWeight    * norm;

		return clearcoatWeight * norm;
	}
	bool isSpecular() const
	{
		return false;
	}
};
/*
vec3 Spherical_Coordinate_To_Direction(const float& cos_theta, const float& sin_theta, const float& phi)
{
return vec3(sin_theta * FTA::cos(phi), sin_theta * FTA::sin(phi), cos_theta);
}

vec3 Schlick_Color(const vec3& c, const float& cos_t)
{
float p = 1.0f - cos_t;
float p2 = p * p;
float p4 = p2 * p2;

return c + (vec3(1.0f) - c) * p4 * p;
}

//dont use macro min since it can cause "unsual" error
//fast but sometimes unreliable
static float min_f(const float& x, const float& y)
{
return x < y ? x : y;
}

float clampf(const float& v, const float& low, const float& high)
{
return min(max(low, v), high);
}

float Schlick_Fresnel(const float& u)
{
float m = clampf(1.0f - u, 0.0f, 1.0f);

float m2 = m * m;

return m2 * m2 * m;
}



float GTR2(const float& NdotH, const float& a)
{
float a2 = a * a;

float t = (a2 - 1.0f) * NdotH * NdotH;

return a2 / (pi * t * t);
}

float GTR2_aniso(const float& NdotH, const float& HdotX, const float& HdotY, const float& ax, const float& ay)
{
return 1.0f / (pi * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

float SmithG_GGX(const float& NdotV, const float& alphaG)
{
float a = alphaG * alphaG;

float b = NdotV * NdotV;

return 1.0f / (NdotV + sqrt14(a + b - a * b));
}

float SmithG_GGX_aniso(const float& NdotV, const float& VdotX, const float& VdotY, const float& ax, const float& ay)
{
return 1.0f / (NdotV + sqrt14(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}

vec3 mon2lin(const vec3& v)
{
return vec3(powf(v.x, 2.2f), powf(v.y, 2.2f), powf(v.z, 2.2f));
}

float Fresnel(const float& VdotN, const float& etaI, const float& etaT)
{
float sin_theta2 = sqr(etaI / etaT) * (1.0f - VdotN * VdotN);

if (sin_theta2 > 1.0f)
return 1.0f;

float LdotN = sqrt14(1.0f - sin_theta2);

float eta = etaI / etaT;

float r1 = (VdotN - eta * LdotN) / (VdotN + eta * LdotN);
float r2 = (LdotN - eta * VdotN) / (LdotN + eta * VdotN);

return (r1 * r1 + r2 * r2) * 0.5f;
}
*/



#endif
