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

struct Sampling_Struct
{
	Sampling_Struct() {}
	Sampling_Struct(vec3 direction_, vec3 weight_, float pdf_, bool isSpecular_) : direction(direction_), weight(weight_), pdf(pdf_), isSpecular(isSpecular_) {}
	vec3 direction;
	vec3 weight = vec3(0.0f);
	float pdf = 0.0f;
	bool isSpecular = false;

};


class BSDF_Sampling
{
public:
	virtual bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color) = 0;
	virtual vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) = 0;
	virtual float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) = 0;
	
	virtual vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return vec3(1.0f);
	}
	
	virtual bool isSpecular() = 0;

	virtual bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
		return false;
	}
	virtual vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
		return vec3(0.0f);
	}
	virtual float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
		return 0.0f;
	}

	
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

static vec3 uniform_hemisphere_Sampling(const vec3& n)
{
	float phi = 2.0f * pi * randf();

	float cos_theta = 1.0f - randf();
	float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

	onb local(n);

	float cos_phi, sin_phi;

	FTA::sincos(phi, &sin_phi, &cos_phi);

	return (sin_theta * (local.u * cos_phi + local.v * sin_phi) + n * cos_theta).norm();
}

class Diffuse : public BSDF_Sampling
{
public:
	Diffuse() {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		vec3 d = cosine_hemisphere_Sampling_with_n(n);

		float cos_o = d.dot(n);

		if (cos_o <= 0.0f)
			return false;

		float pdf = (cos_o)* ipi;

		//vec3 weight = color;

		sampling = Sampling_Struct(d, color, pdf, false);

		isReflect = true;
		return true;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return (direction_out.dot(n)) * ipi;
	}

	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos = direction_out.dot(n);

		return cos > 0.0f ? cos * color * ipi : vec3(0.0f);
	}

	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos = direction_out.dot(n);
		
		sample_pdf = cos * ipi;

		return cos * color * ipi;
	}

	bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		sampling = Sampling_Struct(Reflect(direction_in, n), vec3(1.0f), 1.0f, true);
		isReflect = true;
		return true;

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return vec3(1.0f);
	}
	bool isSpecular() 
	{
		return true;
	}

};

//----------GLASS----------

vec3 __fastcall Refract(const vec3& direction_in, const vec3& n, const float& eta, const float& cos_i, const float& cos_t)
{
	return (eta * direction_in + (eta * cos_i - cos_t) * n).norm();
}

class Glass : public BSDF_Sampling
{
public:
	float ior = 1.6f;
	vec3 real_color = vec3(0.816842f, 0.897079f, 0.861554f);

	Glass() {}
	Glass(float ior_) : ior(ior_) {}
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		//cout << "glass\n";

		//if(n.x != original_n.x || n.y != original_n.y || n.z != original_n.z)


		float cos_i = direction_in.dot(original_n);

		float eta = cos_i >= 0 ? ior : 1.0f / ior;

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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return vec3(1.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
};

class Glass_Color : public BSDF_Sampling
{
public:
	float ior = 2.6f;
	vec3 real_color = vec3(0.816842f, 0.897079f, 0.861554f);//default green color

	Glass_Color() {}
	Glass_Color(float ior_) : ior(ior_) {}
	Glass_Color(vec3 real_color_, float ior_) : real_color(real_color_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return color;
	}
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(1.0f);
	}
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return vec3(1.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
	bool isSampleVolume() const
	{
		return true;
	}
};

class Glass_No_Refract : public BSDF_Sampling
{
public:
	float ior;

	Glass_No_Refract() {}
	Glass_No_Refract(float ior_) : ior(ior_) {}
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
	bool isSampleVolume() const
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	//vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color) = 0;
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n) = 0;
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
	bool isSampleVolume() const
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(1.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
	bool isSampleVolume() const
	{
		return true;
	}
};

float remap_roughness(const float& roughness, const float& cos_i)
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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


	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	bool isSpecular() 
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
bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
{
return vec3(1.0f);
}
float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
{
return 1.0f;
}
vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
{
return vec3(0.0f);
}
bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		vec3 direction = Reflect(direction_in, n);

		isReflect = true;

		float x = conductor_Reflectance(complex_ior.eta.x, complex_ior.k.x, cos_i);
		float y = conductor_Reflectance(complex_ior.eta.y, complex_ior.k.y, cos_i);
		float z = conductor_Reflectance(complex_ior.eta.z, complex_ior.k.z, cos_i);

		vec3 bsdf_eval(x, y, z);

		sampling = Sampling_Struct(direction, bsdf_eval * color, 1.0f, true);
		return true;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		vec3 weight = Conductor_Reflectance_RGB(complex_ior.eta, complex_ior.k, cos_i);
		return color * weight;
		//return vec3(1.0f);//vec3(1.0f) indicate not use
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;

		float cos_i = -direction_in.dot(n);
		vec3 weight = Conductor_Reflectance_RGB(complex_ior.eta, complex_ior.k, cos_i);
		return color * weight;
	}
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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

			sampling = Sampling_Struct(direction, color, direction.dot(n) * ipi * (1.0f - spec_prob), false);

			return true;
		}
		return true;
	}
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n) const
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		//float reflectance, cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		return ipi * cos_o * (1.0f - reflectance) * color;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		sample_pdf = ipi * cos_o * (1.0f - reflectance);

		return color * sample_pdf;
	}
	//bool isSpecular() 
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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

			sampling = Sampling_Struct(direction, color, direction.dot(n) * ipi * (1.0f - spec_prob), true);

			return true;
		}
		return true;
	}
	//float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n) const
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		//float reflectance, cos_t;

		//Dielectric_Reflectance(eta, cos_i, reflectance, cos_t);

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		return ipi * cos_o * (1.0f - reflectance) * color;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		sample_pdf = ipi * cos_o * (1.0f - reflectance);

		return color * sample_pdf;
	}
	//bool isSpecular() 
	bool isSpecular() 
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
	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float reflectance = Dielectric_Reflectance(eta, cos_i);

		sample_pdf = ipi * cos_o * (1.0f - reflectance);

		return color * sample_pdf;
	}
	//bool isSpecular() 
	bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	bool isSpecular() 
	{
		return false;
	}
};

class RoughPlastic_Original_Diffuse_Version : public BSDF_Sampling
{
public:
	float ior;
	float roughness;
	vec3 specular = vec3(0.1f);

	Microfacet_Distribution dist = GGX;

	RoughPlastic_Original_Diffuse_Version() {}
	RoughPlastic_Original_Diffuse_Version(float ior_, float roughness_) : ior(ior_), roughness(roughness_) {}
	RoughPlastic_Original_Diffuse_Version(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		//float cos_i_n = -direction_in.dot(n);
		//float cos_o_n = 

		float original_cos_i = direction_in.dot(original_n);

		float cos_i = abs(original_cos_i);

		float alpha2 = roughness * roughness;// *0.1f;

											 //vec3 m = ggx_sample_H(n, alpha2);

		vec3 m = Microfacet_Sample(dist, n, alpha2);

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, cos_i);

		vec3 direction_out;

		if (randf() < F)
			direction_out = Reflect(direction_in, m);
		else
			direction_out = cosine_hemisphere_Sampling_with_n(n);


		vec3 weight = eval(direction_in, direction_out, n, original_n, color);
		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		//if (sample_pdf < 1e-6)
		//	return false;

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, false);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i_n = -direction_in.dot(n);
		float cos_o_n = direction_out.dot(n);

		//if (cos_i_n < 0.0f || cos_o_n <= 0.0f)
		//	return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_n_m = n.dot(m);

		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness * roughness;// *0.1f;

		float eta = 1.0f / ior;

		float cos_i_m = -direction_in.dot(m);

		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float D = Microfacet_D(dist, m, n, alpha2);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha2);

		float J = 0.25f / cos_o_m;

		vec3 bsdf = color * ipi * cos_o_n * (1.0f - F) + specular * (F * D * G / (4.0f * cos_i_n));

		return bsdf;

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_o_n = direction_out.dot(n);

		vec3 m = (-direction_in + direction_out).norm();

		float eta = 1.0f / ior;

		float cos_i_m = -direction_in.dot(m);

		float cos_m_n = m.dot(n);

		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness * roughness * 0.1f;

		//float D = ggx_D(m, n, alpha2);

		float D = Microfacet_D(dist, m, n, alpha2);

		float J = 0.25f / cos_o_m;

		float F = Dielectric_Reflectance(eta, cos_i_m);

		float sample_pdf = cos_o_n * ipi * (1.0f - F) + D * cos_m_n * J * F;

		return sample_pdf;
	}
	bool isSpecular() 
	{
		return false;
	}
};

//Difference float F = Dielectric_Reflectance(eta, original_cos_i);
//instead of float F = Dielectric_Reflectance(eta, cos_i);
//more glossy
class RoughPlastic_Original_Glossy_Version : public BSDF_Sampling
{
public:
	float ior;
	float roughness;
	vec3 specular = vec3(0.2f);

	Microfacet_Distribution dist = GGX;

	RoughPlastic_Original_Glossy_Version() {}
	RoughPlastic_Original_Glossy_Version(float ior_, float roughness_) : ior(ior_), roughness(roughness_) {}
	RoughPlastic_Original_Glossy_Version(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		//float cos_i_n = -direction_in.dot(n);
		//float cos_o_n = 

		float original_cos_i = direction_in.dot(original_n);

		float cos_i = abs(original_cos_i);

		float alpha2 = roughness * roughness;// *0.1f;

											 //vec3 m = ggx_sample_H(n, alpha2);

		vec3 m = Microfacet_Sample(dist, n, alpha2);

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, original_cos_i);

		vec3 direction_out;

		if (randf() < F)
			direction_out = Reflect(direction_in, m);
		else
			direction_out = cosine_hemisphere_Sampling_with_n(n);


		vec3 weight = eval(direction_in, direction_out, n, original_n, color);
		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		//if (sample_pdf < 1e-6)
		//	return false;

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i_n = -direction_in.dot(n);
		float cos_o_n = direction_out.dot(n);

		//if (cos_i_n < 0.0f || cos_o_n <= 0.0f)
		//	return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_n_m = n.dot(m);

		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness * roughness;// *0.1f;

		float eta = 1.0f / ior;

		float cos_i_m = -direction_in.dot(m);

		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float D = Microfacet_D(dist, m, n, alpha2);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha2);

		float J = 0.25f / cos_o_m;

		vec3 bsdf = color * ipi * cos_o_n * (1.0f - F) + specular * (F * D * G / (4.0f * cos_i_n));

		return bsdf;

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_o_n = direction_out.dot(n);

		vec3 m = (-direction_in + direction_out).norm();

		float eta = 1.0f / ior;

		float cos_i_m = -direction_in.dot(m);

		float cos_m_n = m.dot(n);

		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness * roughness;

		//float D = ggx_D(m, n, alpha2);

		float D = Microfacet_D(dist, m, n, alpha2);

		float J = 0.25f / cos_o_m;

		float F = Dielectric_Reflectance(eta, cos_i_m);

		float sample_pdf = cos_o_n * ipi * (1.0f - F) + D * cos_m_n * J * F;

		return sample_pdf;
	}
	bool isSpecular() 
	{
		return false;
	}
};


//Rough Plastic

class Rough_Plastic_Diffuse : public BSDF_Sampling
{
public:
	float ior = 1.5f;
	float eta = 1.0f / ior;
	float roughness;
	Microfacet_Distribution dist = Beckmann;
	//0.2f
	vec3 spec_color = vec3(0.4f, 0.4f, 0.4f);//lamp

	Rough_Plastic_Diffuse() { eta = 1.0f / ior; }
	Rough_Plastic_Diffuse(float ior_, float roughness_) : ior(ior_), roughness(roughness_) { eta = 1.0f / ior; }
	Rough_Plastic_Diffuse(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) { eta = 1.0f / ior; }

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		//add this part to remove negative cos
		//cause blue stripe
		if (cos_i <= 0.0f)
		{
			//cos_i = abs(cos_i);
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

		float cos_i_m = -direction_in.dot(m);

		float F = Dielectric_Reflectance(eta, cos_i_m);

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

			is_specular = true;//false;
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

			is_specular = false;//true;
		}

		isReflect = true;


		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		if (sample_pdf < 1e-6f)
		{
			sampling = Sampling_Struct();
			return false;
		}

		vec3 weight = eval(direction_in, direction_out, n, original_n, color);

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, false);
		return true;

		/*float cos_o = direction_out.dot(n);

		if (cos_o <= 0.0f)
			return false;


		vec3 h = (-direction_in + direction_out).norm();

		
		float cos_i_h = -direction_in.dot(h);
		

		float F2 = Dielectric_Reflectance(eta, cos_i_h);

		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		vec3 spec_brdf = spec_color * (D * F2 * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		vec3 weight = spec_brdf + diff_brdf;


		float spec_pdf = Microfacet_pdf(dist, h, n, roughness) / (4.0f * direction_out.dot(h));

		float diff_pdf = cos_o * ipi;

		//failed
		//float cos_o_m = direction_out.dot(m);
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * cos_o_m);
		//float diff_pdf = cos_o_m * ipi;

		float sample_pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);

		return true;*/
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//add this one to remove blue stripe
		if (cos_i <= 0.0f || cos_o <= 0.0f)
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
		float eta = 1.0f / ior;
		float cos_t;

		float F = Dielectric_Reflectance(eta, cos_i_m);//Fresnel_Dielectric(eta, cos_i_m, cos_t);// Dielectric_Reflectance(eta, cos_i_m);

													   //float D = ggx_D(m, n, roughness);//, m, n);

													   //float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = spec_color * (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		return spec_brdf + diff_brdf;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return 1.0f;

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

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness) / (4.0f * direction_out.dot(m));

		float diff_pdf = cos_o * ipi;

		//failed
		//float cos_o_m = direction_out.dot(m);
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * cos_o_m);
		//float diff_pdf = cos_o_m * ipi;

		return spec_pdf * F + diff_pdf * (1.0f - F);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();


		float cos_i_m = -direction_in.dot(m);
		float eta = 1.0f / ior;


		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float D = ggx_D(m, n, roughness);//, m, n);

		//float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		//original good
		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m) * cos_o);

		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m));

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness) / (4.0f * direction_out.dot(m));

		float diff_pdf = cos_o * ipi;

		sample_pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		return spec_brdf + diff_brdf;
	}
	bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		/*if (cos_i <= 0.0f)
		{
		sampling = Sampling_Struct();
		return false;
		}*/

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

			is_specular = true;
		}

		isReflect = true;


		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		if (sample_pdf < 1e-9f)
		{
			sampling = Sampling_Struct();
			return false;
		}

		vec3 weight = eval(direction_in, direction_out, n, original_n, color);

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, is_specular);
		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return vec3(0.0f);

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

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return 1.0f;

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

		return spec_pdf * F + diff_pdf * (1.0f - F);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();


		float cos_i_m = -direction_in.dot(m);
		float eta = 1.0f / ior;


		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float D = ggx_D(m, n, roughness);//, m, n);

		//float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		//original good
		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m) * cos_o);

		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m));

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness) / (4.0f * direction_out.dot(m));

		float diff_pdf = cos_o * ipi;

		sample_pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		return spec_brdf + diff_brdf;
	}
	bool isSpecular() 
	{
		return true;
	}
};


class Rough_Plastic_Diffuse_Thickness : public BSDF_Sampling
{
public:
	float ior = 1.7;
	float roughness = 0.1;
	float thickness = 1.0f;
	vec3 sigmaA = vec3(0.2f, 0.2f, 0.2f);

	Microfacet_Distribution dist = GGX;

	//preset 
	vec3 scaleSigma;
	float average_transmitance;

	//0.2f
	vec3 spec_color = vec3(0.4f, 0.4f, 0.4f);//lamp

	Rough_Plastic_Diffuse_Thickness() { prepare_before_render(); }
	Rough_Plastic_Diffuse_Thickness(float ior_, float roughness_) : ior(ior_), roughness(roughness_) { prepare_before_render(); }
	Rough_Plastic_Diffuse_Thickness(Microfacet_Distribution dist_, float ior_, float roughness_) : dist(dist_), ior(ior_), roughness(roughness_) { prepare_before_render(); }

	void prepare_before_render()
	{
		scaleSigma = thickness * sigmaA;
		average_transmitance = expf(-2.0f * scaleSigma.average());
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		//add this part to remove negative cos
		//cause blue stripe
		if (cos_i <= 0.0f)
		{
			//cos_i = abs(cos_i);
			sampling = Sampling_Struct();
			return false;
		}

		float eta = 1.0f / ior;

		vec3 m = ggx_sample_H(n, roughness);

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

			is_specular = false;
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

			is_specular = true;
		}

		isReflect = true;


		float sample_pdf = pdf(direction_in, direction_out, n, original_n);

		if (sample_pdf < 1e-6f)
		{
			sampling = Sampling_Struct();
			return false;
		}

		vec3 weight = eval(direction_in, direction_out, n, original_n, color);

		sampling = Sampling_Struct(direction_out, weight / sample_pdf, sample_pdf, false);
		return true;

	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//add this one to remove blue stripe
		if (cos_i <= 0.0f || cos_o <= 0.0f)
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
		float eta = 1.0f / ior;
		float cos_t;

		float F = Dielectric_Reflectance(eta, cos_i_m);//Fresnel_Dielectric(eta, cos_i_m, cos_t);// Dielectric_Reflectance(eta, cos_i_m);

													   //float D = ggx_D(m, n, roughness);//, m, n);

													   //float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = spec_color * (D * F * G) / (4.0f * cos_i);

		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		if (scaleSigma.maxc() > 0.0f)
		{
			diff_brdf.x *= expf(scaleSigma.x * (-1.0f / cos_o - 1.0f / cos_i));
			diff_brdf.y *= expf(scaleSigma.y * (-1.0f / cos_o - 1.0f / cos_i));
			diff_brdf.z *= expf(scaleSigma.z * (-1.0f / cos_o - 1.0f / cos_i));
		}

		return spec_brdf + diff_brdf;
	}

	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return 1.0f;

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

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness) / (4.0f * direction_out.dot(m));

		float diff_pdf = cos_o * ipi;

		//failed
		//float cos_o_m = direction_out.dot(m);
		//float spec_pdf = GGX_pdf(roughness, m.dot(n), m, n) / (4.0f * cos_o_m);
		//float diff_pdf = cos_o_m * ipi;

		return spec_pdf * F + diff_pdf * (1.0f - F);
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();


		float cos_i_m = -direction_in.dot(m);
		float eta = 1.0f / ior;


		float F = Dielectric_Reflectance(eta, cos_i_m);

		//float D = ggx_D(m, n, roughness);//, m, n);

		//float G = ggx_G(direction_in, direction_out, m, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		vec3 spec_brdf = (D * F * G) / (4.0f * cos_i);
		vec3 diff_brdf = color * ipi * cos_o * (1.0f - F);

		//original good
		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m) * cos_o);

		//float spec_pdf = ggx_pdf(m, n, roughness) / (4.0f * direction_out.dot(m));

		float spec_pdf = Microfacet_pdf(dist, m, n, roughness);

		float diff_pdf = cos_o * ipi;

		sample_pdf = spec_pdf * F + diff_pdf * (1.0f - F);

		return spec_brdf + diff_brdf;
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Rough_Plastic_Tungsten : public BSDF_Sampling
{
public:
	float ior = 1.7f;
	float thickness = 1.0f;
	float roughness = 0.1f;
	//float substrateWeight;
	vec3 sigmaA = vec3(0.0f, 0.0f, 0.0f);

	float diffuseFresnel;
	float averageTransmittance = 1.0f;
	vec3 scaleSigmaA = vec3(0.0f);

	Microfacet_Distribution dist = GGX;

	BSDF_Sampling* sub_material;// = new Rough_Glass(dist, ior, roughness);

	Rough_Plastic_Tungsten() 
	{
		Prepare_For_Render(); 
		sub_material = new Rough_Glass_Fresnel(ior, roughness);
	}
	Rough_Plastic_Tungsten(float ior_, float roughness_, float thickness_) : ior(ior_), roughness(roughness_), thickness(thickness_)
	{
		Prepare_For_Render(); 
		sub_material = new Rough_Glass_Fresnel(ior, roughness);
	}
	Rough_Plastic_Tungsten(Microfacet_Distribution dist_, float ior_, float roughness_, float thickness_) : dist(dist_), ior(ior_), roughness(roughness_), thickness(thickness_)
	{
		Prepare_For_Render(); 
		sub_material = new Rough_Glass_Fresnel(dist, ior, roughness);
	}

	//setup for rendering
	void Prepare_For_Render()
	{
		scaleSigmaA = thickness * sigmaA;

		averageTransmittance *= expf(-2.0f * scaleSigmaA.average());

		diffuseFresnel = Compute_Diffuse_Fresnel(ior, 1000000);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float eta = 1.0f / ior;

		//float abs_cos_i = abs(cos_i);

		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrate_weight = color.average() * averageTransmittance * (1.0f - Fi);

		float specular_weight = Fi;

		float specular_prob =  specular_weight / (specular_weight + substrate_weight);

		if (randf() < specular_prob)
		{
			//sub sample is Rough_Glass_Fresnel or Rough_Dielectric
			//changing subs_sample to other materials may create interesting results;
			Sampling_Struct sub_sample;
			
			bool is_sub_Reflect;

			bool rough_dielectric = sub_material->sample(direction_in, n, original_n, sub_sample, is_sub_Reflect, color);

			if (!rough_dielectric)
				return false;


			float alpha = roughness;
			vec3 m = Microfacet_Sample(dist, n, alpha);
			vec3 direction_out = Reflect(direction_in, m);
			//float weight = Fi / specular_prob;

			//sampling = Sampling_Struct(direction_out, vec3(weight), specular_prob, false);

			float cos_o = direction_out.dot(n);

			float abs_cos_o = abs(cos_o);

			float Fo = Dielectric_Reflectance(eta, abs_cos_o);

			vec3 brdf_substrate = (1.0f - Fi) * (1.0f - Fo) * eta * eta * color / (vec3(1.0f) - color * diffuseFresnel) * ipi * abs_cos_o;
			vec3 brdf_specular = sub_sample.weight * sub_sample.pdf;

			float pdf_substrate = abs_cos_o * ipi * (1.0f - specular_prob);
			float pdf_specular = sub_sample.pdf * specular_prob;

			vec3 sample_weight = (brdf_substrate + brdf_specular) / (pdf_substrate + pdf_specular);

			float sample_pdf = pdf_substrate + pdf_specular;

			sampling = Sampling_Struct(sub_sample.direction, sample_weight, sample_pdf, true);

			return true;
			//vec3 brdf_specular = 
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

			vec3 brdf_substrate = weight * abs_cos_o * ipi;
			float pdf_substrate = abs_cos_o * ipi * (1.0f - specular_prob);

			vec3 brdf_specular = sub_material->eval(direction_in, direction_out, n, original_n, color);
			float pdf_specular = sub_material->pdf(direction_in, direction_out, n, original_n);

			pdf_specular *= specular_prob;
			
			vec3 sample_weight = (brdf_specular + brdf_substrate) / (pdf_specular + pdf_substrate);
			float sample_pdf = pdf_specular + pdf_substrate;

			//float sample_pdf = abs_cos_o * ipi * (1.0f - specular_prob);

			//float inv_diff_prob = 1.0f / (1.0f - specular_prob);

			//weight *= inv_diff_prob;



			sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);
		}
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		//float abs_cos_i = abs(cos_i);
		//float abs_cos_o = abs(cos_o);


		float eta = 1.0f / ior;

		float Fi = Dielectric_Reflectance(eta, cos_i);
		float Fo = Dielectric_Reflectance(eta, cos_o);

		bool reflection = checkReflection(direction_in, direction_out);

		//if (cos_i * cos_o > 0.0f)

		vec3 glossy_R(0.0f);
		vec3 diffuse_R(0.0f);

		//if (reflection)
		//{
			glossy_R = sub_material->eval(direction_in, direction_out, n, original_n, color);

			//return vec3(Fi);
		//}
		//else
		//{
			//float eta = 1.0f / ior;
			//float Fi = Dielectric_Reflectance(eta, cos_i);//abs_cos_i
			//float Fo = Dielectric_Reflectance(eta, cos_o);//abs_cos_o

			diffuse_R = (1.0f - Fi) * (1.0f - Fo) * eta * eta * cos_o * ipi * (color / (1.0f - color * diffuseFresnel));//abs_cos_o

			if (scaleSigmaA.maxc() > 0.0f)
			{
				diffuse_R.x *= expf(scaleSigmaA.x * (-1.0f / cos_o - 1.0f / cos_i));
				diffuse_R.y *= expf(scaleSigmaA.y * (-1.0f / cos_o - 1.0f / cos_i));
				diffuse_R.z *= expf(scaleSigmaA.z * (-1.0f / cos_o - 1.0f / cos_i));
			}
			//return brdf;
		//}
		//return vec3(0.0f);

		return glossy_R + diffuse_R;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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


		float glossy_pdf = sub_material->pdf(direction_in, direction_out, n, original_n);
		float diffuse_pdf = abs_cos_o * ipi;


		float Fi = Dielectric_Reflectance(eta, cos_i);

		float substrate_weight =  averageTransmittance * (1.0f - Fi);//color.average() *

		float specular_weight = Fi;

		float specular_prob = (specular_weight) / (specular_weight + substrate_weight);

		diffuse_pdf *= (1.0f - specular_prob);
		glossy_pdf *= specular_prob;

		return diffuse_pdf + glossy_pdf;

		//bool reflection = cos_i * cos_o >= 0.0f; //checkReflection(direction_in, direction_out);

												 //if (cos_i * cos_o > 0.0f)
		//if (reflection)
		//	return specularprob;
		//else
		//	abs_cos_o * (1.0f - specularprob);
	}
	bool isSpecular() 
	{
		return false;
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

class Final_Rough_Conductor : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor() {}
	Final_Rough_Conductor(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Final_Rough_Conductor(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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


	bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
		float alpha2 = roughness_value;// *roughness;


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

		float alpha = roughness_value;

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
	vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		//float alpha2 = roughness_value;// *roughness;



		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness_value;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//good
		return color * (D * G) / (4.0f * cos_i);

		//vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

		//return eval;

		//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
		vec3 m = (-direction_in + direction_out).norm();

		float alpha2 = roughness_value;// *roughness;

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


	/*
	bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
	float alpha = roughness_value;// *roughness;


	vec3 m = ggx_sample_H(n, alpha);

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

	//float alpha = roughness_value;

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
	vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
	float cos_i = -direction_in.dot(n);
	float cos_o = direction_out.dot(n);

	if (cos_i <= 0.0f || cos_o <= 0.0f)
	return vec3(0.0f);

	vec3 m = (-direction_in + direction_out).norm();

	float cos_m_n = m.dot(n);
	float cos_o_m = direction_out.dot(m);

	//float alpha2 = roughness;// *roughness;



	//float D = ggx_D(m, n, alpha2);

	//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

	float alpha = roughness_value;

	float D = Microfacet_D(dist, m, n, alpha);

	float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

	float J = 1.0f / (4.0f * cos_o_m);

	//good
	return color * (D * G) / (4.0f * cos_i);

	//vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

	//return eval;

	//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
	vec3 m = (-direction_in + direction_out).norm();

	float alpha = roughness_value;// *roughness;

	float D = ggx_D(m, n, alpha);

	float cos_o_m = direction_out.dot(m);

	float cos_m_n = m.dot(n);

	float J = 1.0f / (4.0f * cos_o_m);

	float sample_pdf = D * cos_m_n * J;;
	//return D * cos_m_n * J;

	if (sample_pdf <= 0.0f)
	sample_pdf = 1.0f;//-sample_pdf;

	return sample_pdf;
	}
	*/

	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
		{
			sample_pdf = 1.0f;
			return vec3(0.0f);
		}
		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		//float alpha2 = roughness;// *roughness;

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//pdf


		sample_pdf = D * cos_m_n * J;


		//bsdf
		//good
		//return color * (D * G) / (4.0f * cos_i);

		return color * (F * D * G) / (4.0f * cos_i);
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Final_Rough_Conductor2 : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;
	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor2() {}
	Final_Rough_Conductor2(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float alpha2 = roughness;// *roughness;

		vec3 m = Microfacet_Sample(dist, n, alpha2);

		//vec3 m = sampleGGXVNDF(direction_in, n, alpha2, alpha2);

		vec3 direction_out = Reflect(direction_in, m);

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return false;

		float cos_o_m = direction_out.dot(m);


		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		float D = ggx_D(m, n, alpha2);

		float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float J = 1.0f / (4.0f * cos_o_m);

		float cos_m_n = m.dot(n);

		vec3 weight = color * G * cos_o_m / (cos_m_n * cos_i);

		//vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		//weight = bsdf failed
		//vec3 weight = color * D * G / (4.0f * cos_i);

		float pdf = D * cos_m_n * J;

		sampling = Sampling_Struct(direction_out, weight, pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness;// *roughness;


								 //bỏ F khỏi phương trình là ý tưởng tuyệt vời
								 //vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		float D = ggx_D(m, n, alpha2);

		float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float J = 1.0f / (4.0f * cos_o_m);

		return color * (D * G) / (4.0f * cos_i);

		//return color * G * cos_o_m / (cos_m_n * cos_i);

		//return color * F * ( D * G) / (4.0f * cos_i);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 m = (-direction_in + direction_out).norm();

		float alpha2 = roughness;// *roughness;

		float D = ggx_D(m, n, alpha2);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		return D * cos_m_n * J;
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Final_Rough_Conductor_Roughness_Mapping : public BSDF_Sampling
{
public:
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor_Roughness_Mapping() {}
	Final_Rough_Conductor_Roughness_Mapping(ComplexIor ior_) : ior(ior_) {}
	Final_Rough_Conductor_Roughness_Mapping(Microfacet_Distribution dist_, ComplexIor ior_) : dist(dist_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(0.0f);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 0.0f;
	}

	bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
		//float alpha = roughness_value;// *roughness;
		float cos_i = -direction_in.dot(n);


		if (cos_i <= 0.0f)
			return false;



		float alpha = remap_roughness(roughness_value, cos_i);

		vec3 m = ggx_sample_H(n, alpha);

		//vec3 m = GGX_Sample_H(n, alpha2);

		//vec3 m = sampleGGXVNDF(direction_in, n, alpha2, alpha2);

		vec3 direction_out = Reflect(direction_in, m);


		float cos_o = direction_out.dot(n);

		if (cos_o <= 0.0f)
			return false;

		float cos_o_m = direction_out.dot(m);


		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		//float alpha = roughness_value;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		float cos_m_n = m.dot(n);

		//Good
		vec3 weight = color * abs(G * cos_o_m / (cos_m_n * cos_i));

		//vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		//weight = positive_vector(weight);

		//weight = bsdf failed
		//vec3 weight = color * D * G / (4.0f * cos_i);

		float pdf = (D * cos_m_n * J);

		//add
		if (pdf < 0.0f)
			pdf = -pdf;
		sampling = Sampling_Struct(direction_out, weight, pdf, false);

		return true;
	}
	vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		//float alpha2 = roughness;// *roughness;



		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		//float alpha = roughness_value;

		float alpha = remap_roughness(roughness_value, cos_i);

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//good
		return color * abs((D * G) / (4.0f * cos_i));

		//vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

		//return eval;

		//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
		vec3 m = (-direction_in + direction_out).norm();

		//float alpha2 = roughness_value;// *roughness;

		float cos_i_n = abs(-direction_in.dot(n));

		float alpha = remap_roughness(roughness_value, cos_i_n);

		float D = ggx_D(m, n, alpha);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		float sample_pdf = D * cos_m_n * J;;
		//return D * cos_m_n * J;

		//if (sample_pdf < 0.0f)
		//	sample_pdf = -sample_pdf;

		return abs(sample_pdf);
	}

	bool isSpecular() 
	{
		return false;
	}
};

class Final_Rough_Conductor_Absolute : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor_Absolute() {}
	Final_Rough_Conductor_Absolute(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Final_Rough_Conductor_Absolute(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float alpha2 = roughness;// *roughness;


		vec3 m = ggx_sample_H(n, alpha2);

		//vec3 m = GGX_Sample_H(n, alpha2);

		//vec3 m = sampleGGXVNDF(direction_in, n, alpha2, alpha2);

		vec3 direction_out = Reflect(direction_in, m);

		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return false;

		float cos_o_m = direction_out.dot(m);


		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		float cos_m_n = m.dot(n);

		//Good
		//vec3 weight = color * G * cos_o_m / (cos_m_n * cos_i);

		vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		weight = positive_vector(weight);

		//weight = bsdf failed
		//vec3 weight = color * D * G / (4.0f * cos_i);

		float pdf = D * cos_m_n * J;

		//add
		if (pdf < 0.0f)
			pdf = -pdf;
		sampling = Sampling_Struct(direction_out, weight, pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness;// *roughness;


								 //bỏ F khỏi phương trình là ý tưởng tuyệt vời
		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//good
		//return color * (D * G) / (4.0f * cos_i);

		vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

		return eval;

		//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 m = (-direction_in + direction_out).norm();

		//float alpha2 = roughness;// *roughness;

		//float D = ggx_D(m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		float sample_pdf = D * cos_m_n * J;;
		//return D * cos_m_n * J;

		if (sample_pdf < 0.0f)
			sample_pdf = -sample_pdf;

		return sample_pdf;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
		{
			sample_pdf = 1.0f;
			return vec3(0.0f);
		}
		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness;// *roughness;

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//pdf


		sample_pdf = D * cos_m_n * J;


		//bsdf
		//good
		//return color * (D * G) / (4.0f * cos_i);

		return color * (F * D * G) / (4.0f * cos_i);
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Final_Rough_Conductor_modify : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Final_Rough_Conductor_modify() {}
	Final_Rough_Conductor_modify(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Final_Rough_Conductor_modify(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float alpha2 = roughness;// *roughness;


								 //vec3 m = ggx_sample_H(n, alpha2);

								 //vec3 m = GGX_Sample_H(n, alpha2);

								 //vec3 m = sampleGGXVNDF(direction_in, n, alpha2, alpha2);

		vec3 m = Microfacet_Sample(dist, n, alpha2);

		vec3 direction_out = Reflect(direction_in, m);


		float cos_o = direction_out.dot(n);

		//if (cos_i <= 0.0f || cos_o <= 0.0f)
		//	return false;

		float cos_o_m = direction_out.dot(m);


		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		float cos_m_n = m.dot(n);

		//vec3 weight = color * (F * D * G) / (4.0f * cos_i);

		vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		//vec3 weight = color * F * G * cos_o_m / (cos_m_n * cos_i);

		//weight = bsdf failed
		//vec3 weight = color * D * G / (4.0f * cos_i);

		float pdf = D * cos_m_n * J;

		sampling = Sampling_Struct(direction_out, weight, pdf, true);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		float alpha2 = roughness;// *roughness;


								 //bỏ F khỏi phương trình là ý tưởng tuyệt vời
		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, -direction_in.dot(m));

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		return color * (F * G) / (4.0f * cos_i);

		//return color * G * cos_o_m / (cos_m_n * cos_i);

		//return color * F * ( D * G) / (4.0f * cos_i);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 m = (-direction_in + direction_out).norm();

		//float alpha2 = roughness;// *roughness;

		//float D = ggx_D(m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		return D * cos_m_n * J;
	}

	bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
		float alpha = roughness_value;// *roughness;


		vec3 m = ggx_sample_H(n, alpha);

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

		//float alpha = roughness_value;

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
	vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		//float alpha2 = roughness;// *roughness;



		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness_value;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//good
		return color * (D * G) / (4.0f * cos_i);

		//vec3 eval = positive_vector(color * G * cos_o_m / (cos_m_n * cos_i));

		//return eval;

		//return color * positive_vector(F) * ( D * G) / (4.0f * cos_i);
	}
	float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
		vec3 m = (-direction_in + direction_out).norm();

		float alpha = roughness_value;// *roughness;

		float D = ggx_D(m, n, alpha);

		float cos_o_m = direction_out.dot(m);

		float cos_m_n = m.dot(n);

		float J = 1.0f / (4.0f * cos_o_m);

		float sample_pdf = D * cos_m_n * J;;
		//return D * cos_m_n * J;

		//if (sample_pdf < 0.0f)
		//	sample_pdf = -sample_pdf;

		return sample_pdf;
	}


	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
		{
			sample_pdf = 1.0f;
			return vec3(0.0f);
		}
		vec3 m = (-direction_in + direction_out).norm();

		float cos_m_n = m.dot(n);
		float cos_o_m = direction_out.dot(m);

		//float alpha2 = roughness;// *roughness;

		//float D = ggx_D(m, n, alpha2);

		//float G = ggx_G(direction_in, direction_out, m, n, alpha2);

		float alpha = roughness;

		float D = Microfacet_D(dist, m, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, m, n, alpha);

		float J = 1.0f / (4.0f * cos_o_m);

		//pdf


		sample_pdf = D * cos_m_n * J;


		//bsdf
		return color * (D * G) / (4.0f * cos_i);
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Rough_Conductor_TungSten : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Rough_Conductor_TungSten() {}
	Rough_Conductor_TungSten(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Rough_Conductor_TungSten(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		//vec3 h = ggx_sample_H(n, roughness);

		//float h_pdf = ggx_pdf(h, n, roughness);

		vec3 h = Microfacet_Sample(dist, n, roughness);

		float h_pdf = Microfacet_pdf(dist, h, n, roughness);

		float cos_i_h = -direction_in.dot(h);

		vec3 direction_out = Reflect(direction_in, h);//(direction_in + 2.0f * cos_i_h * h).norm();


													  //float G = ggx_G(direction_in, direction_out, h, n, roughness);

													  //float D = ggx_D(h, n, roughness);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);


		float sample_pdf = h_pdf / (4.0f * cos_i_h);

		//original
		vec3 weight = cos_i_h * D * G / (h_pdf * cos_i);
		
		sampling = Sampling_Struct(direction_out, color * F * weight, sample_pdf, true);

		return true;

	}
	bool sample_volume(const vec3& direction_in, vec3& n, vec3& original_n, float& t, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		
		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		
		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		return color * F * D * G / (4.0f * cos_i);

		
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 h = (-direction_in + direction_out).norm();

		float alpha = roughness;

		float sample_pdf = Microfacet_pdf(dist, h, n, alpha);

		return abs(sample_pdf / (4.0f * -direction_in.dot(h)));
	}


	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);


		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);


		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		sample_pdf = Microfacet_pdf(dist, h, n, alpha) / (4 * cos_i_h);

		sample_pdf = abs(sample_pdf);

		return color * F * D * G / (4.0f * cos_i);
	}
	bool isSpecular() 
	{
		return false;
	}
	bool isSampleVolume() const
	{
		return false;
	}
};

class Rough_Conductor_TungSten_Roughness_Mapping : public BSDF_Sampling
{
public:
	//float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Rough_Conductor_TungSten_Roughness_Mapping() {}
	Rough_Conductor_TungSten_Roughness_Mapping(ComplexIor ior_) : ior(ior_) {}
	Rough_Conductor_TungSten_Roughness_Mapping(Microfacet_Distribution dist_, ComplexIor ior_) : dist(dist_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return false;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 0.0f;
	}

	bool sample_roughness(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color, float& roughness_value) const
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		//vec3 h = ggx_sample_H(n, roughness);

		//float h_pdf = ggx_pdf(h, n, roughness);

		float alpha = roughness_value;

		vec3 h = Microfacet_Sample(dist, n, alpha);

		float h_pdf = Microfacet_pdf(dist, h, n, alpha);

		float cos_i_h = -direction_in.dot(h);

		vec3 direction_out = Reflect(direction_in, h);//(direction_in + 2.0f * cos_i_h * h).norm();


													  //float G = ggx_G(direction_in, direction_out, h, n, roughness);

													  //float D = ggx_D(h, n, roughness);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);



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

		//original
		vec3 weight = cos_i_h * D * G / (h_pdf * cos_i);
		//vec3 weight = color * (F * D * G) * cos_i_h / (h_pdf * cos_i);

		//modify
		//vec3 weight = color * (D * G) * cos_i_h / (h_pdf * cos_i);

		sampling = Sampling_Struct(direction_out, color * F * weight, sample_pdf, true);

		return true;

	}

	vec3 eval_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& roughness_value) const
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		//if (cos_i_h <= 0.0f)
		//	return vec3(0.0f);

		//float x = conductor_Reflectance(ior.eta.x, ior.k.x, cos_i);
		//float y = conductor_Reflectance(ior.eta.y, ior.k.y, cos_i);
		//float z = conductor_Reflectance(ior.eta.z, ior.k.z, cos_i);

		//vec3 F = vec3(x, y, z);

		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);


		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		//float G = ggx_G(direction_in, direction_out, h, n, roughness);

		//float D = ggx_D(h, n, roughness);

		float alpha = roughness_value;

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
	float pdf_roughness(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, float& roughness_value) const
	{
		vec3 h = (-direction_in + direction_out).norm();

		//float sample_pdf = ggx_pdf(h, n, roughness);

		float alpha = roughness_value;

		float sample_pdf = Microfacet_pdf(dist, h, n, alpha);

		return sample_pdf / (4.0f * -direction_in.dot(h));
	}


	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return false;
	}
	bool isSampleVolume() const
	{
		return false;
	}
};

class Rough_Conductor_TungSten_Absolute : public BSDF_Sampling
{
public:
	float roughness;
	ComplexIor ior;

	Microfacet_Distribution dist = GGX;

	Rough_Conductor_TungSten_Absolute() {}
	Rough_Conductor_TungSten_Absolute(float roughness_, ComplexIor ior_) : roughness(roughness_), ior(ior_) {}
	Rough_Conductor_TungSten_Absolute(Microfacet_Distribution dist_, float roughness_, ComplexIor ior_) : dist(dist_), roughness(roughness_), ior(ior_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		//vec3 h = ggx_sample_H(n, roughness);

		//float h_pdf = ggx_pdf(h, n, roughness);

		float alpha = roughness;

		vec3 h = Microfacet_Sample(dist, n, alpha);

		vec3 direction_out = Reflect(direction_in, h);//(direction_in + 2.0f * cos_i_h * h).norm();

													  //float G = ggx_G(direction_in, direction_out, h, n, roughness);

													  //float D = ggx_D(h, n, roughness);

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		//float x = conductor_Reflectance(ior.eta.x, ior.k.x, cos_i);
		//float y = conductor_Reflectance(ior.eta.y, ior.k.y, cos_i);
		//float z = conductor_Reflectance(ior.eta.z, ior.k.z, cos_i);

		//vec3 F = vec3(x, y, z);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);

		//cos_i_h = cos_i_h >= 0.0f ? cos_i_h : 0.0f;

		//if (cos_i_h <= 0.0f)
		//	return false;

		//cos_i_h = abs(cos_i_h);

		float h_pdf = Microfacet_pdf(dist, h, n, alpha);

		float cos_i_h = -direction_in.dot(h);



		float sample_pdf = h_pdf / (4.0f * cos_i_h);

		if (sample_pdf < 0.0f)
			sample_pdf = -sample_pdf;

		//original
		vec3 weight = positive_vector(cos_i_h * D * G / (h_pdf * cos_i));
		//vec3 weight = color * (F * D * G) * cos_i_h / (h_pdf * cos_i);

		//modify
		//vec3 weight = color * (D * G) * cos_i_h / (h_pdf * cos_i);

		sampling = Sampling_Struct(direction_out, color * F * weight, sample_pdf, true);

		return true;

	}
	bool sample_volume(const vec3& direction_in, vec3& n, vec3& original_n, float& t, Sampling_Struct& sampling, bool& isReflect, vec3& color) const
	{
		return false;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		float cos_o = direction_out.dot(n);

		if (cos_i <= 0.0f || cos_o <= 0.0f)
			return vec3(0.0f);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		//if (cos_i_h <= 0.0f)
		//	return vec3(0.0f);

		//float x = conductor_Reflectance(ior.eta.x, ior.k.x, cos_i);
		//float y = conductor_Reflectance(ior.eta.y, ior.k.y, cos_i);
		//float z = conductor_Reflectance(ior.eta.z, ior.k.z, cos_i);

		//vec3 F = vec3(x, y, z);

		//vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);


		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i);

		//float G = ggx_G(direction_in, direction_out, h, n, roughness);

		//float D = ggx_D(h, n, roughness);

		float alpha = roughness;

		float D = Microfacet_D(dist, h, n, alpha);

		float G = Microfacet_G(dist, direction_in, direction_out, h, n, alpha);

		return positive_vector(color * F * D * G / (4.0f * cos_i));

		//float cos_i = -direction_in.dot(n);

		/*
		vec3 h = (-direction_in + direction_out).norm();

		float cos_i_h = -direction_in.dot(h);

		vec3 F = Conductor_Reflectance_RGB(ior.eta, ior.k, cos_i_h);

		return color * F;
		*/
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 h = (-direction_in + direction_out).norm();

		//float sample_pdf = ggx_pdf(h, n, roughness);

		float sample_pdf = Microfacet_pdf(dist, h, n, roughness);

		if (sample_pdf < 0.0f)
			sample_pdf = -sample_pdf;

		return sample_pdf / (4.0f * abs(-direction_in.dot(h)));
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return false;
	}
	bool isSampleVolume() const
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
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

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;//check Reflection Condition, or dot product of direction_in and direction_out if too small, pdf = 0.0f
					//however return pdf 0.0f cause error and thin sheet is pure specular so this function is
					//not needed any way
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
	bool need_original_normal() const
	{
		return true;
	}
};


//ior qua lon se dan den total internal refelection
//lam cho ham sample return false
class SmoothCoatTungsten : public BSDF_Sampling
{
public:
	float ior = 1.1f;
	//float thickness = 1.0f;
	//vec3 sigmaA = vec3(0.0f);

	float thickness = 5.0f;
	vec3 sigmaA = vec3(0.1f, 0.1f, 0.1f);

	BSDF_Sampling* submaterial = new Plastic_Diffuse(1.7f);//new Rough_Conductor_TungSten(0.1, Ior_List[2]);

	vec3 sub_color = vec3(0.2, 0.6, 0.9);

	//Microfacet_Distribution sub_material_dist = GGX;

	//vec3 sub_color = vec3(0.811, 0.933, 0.98);

	//preset
	float average_transmitance;
	vec3 scaleSigma;

	SmoothCoatTungsten() { prepare_before_render(); }
	SmoothCoatTungsten(float ior_, float thickness_, vec3 sigmaA_) : ior(ior_), thickness(thickness_), sigmaA(sigmaA_) { prepare_before_render(); };

	void prepare_before_render()
	{
		scaleSigma = thickness * sigmaA;
		average_transmitance = expf(-2.0f * scaleSigma.average());
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
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
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
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
	bool isSpecular() 
	{
		return false;
	}
};

class Transparent : public BSDF_Sampling
{
public:
	float transparent = 0.95f;//0 = light block completely, 1 = light travel freely
	Transparent() {}
	Transparent(float transparent_) : transparent(transparent_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
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
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return color;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return transparent;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		return vec3(0.0f);
	}
	bool isSpecular() 
	{
		return true;
	}
};

//object treated as light source
//however there are two many triangles in this object and we dont need to sample this light source
//so Light_Diffuse come to light
class Light_Diffuse : public BSDF_Sampling
{
public:
	vec3 Kd;
	Light_Diffuse() {}
	Light_Diffuse(vec3 Kd_) : Kd(Kd_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		return vec3(Kd);
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return 1.0f;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		sample_pdf = 1.0f;
		return color;
	}
	bool isSpecular() 
	{
		return false;
	}
};

class Velvet : public BSDF_Sampling
{
public:
	vec3 R = vec3(0.6, 0.6, 0.6);//Reflectance
	float f = 0.5;//fall off
	Velvet () {}
	Velvet(vec3 R_, float f_) : R(R_), f(f_) {}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		vec3 d = cosine_hemisphere_Sampling_with_n(n);

		float cos_o = d.dot(n);

		if (cos_o <= 0.0f)
			return false;

		float pdf = cos_o * ipi;

		vec3 weight = color;

		sampling = Sampling_Struct(d, weight, pdf, false);

		isReflect = true;
		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_o = direction_out.dot(n);
		float cos_i = -direction_in.dot(n);

		float sin_o = sqrt14(1.0f - cos_o * cos_o);
		float horizon_scatter = powf(sin_o, f);

		return color * cos_o + horizon_scatter * R * cos_i * ipi;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return direction_out.dot(n) * ipi;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& color, float& sample_pdf) const
	{
		float cos_o = direction_out.dot(n);
		float cos_i = -direction_in.dot(n);

		float sin_o = sqrt14(1.0f - cos_o * cos_o);
		float horizon_scatter = powf(sin_o, f);

		sample_pdf = cos_o * ipi;

		return color * cos_o + horizon_scatter * R * cos_i * ipi;
	}
	bool isSpecular() 
	{
		return false;
	}
};

//Cloth Utility
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

class Cloth : public BSDF_Sampling
{
public:

	float ior = 1.5f;
	float roughness = 0.06f;
	float inv_roughness;// = 1.0f / roughness;
	float a, b, c, d, e;

	Cloth()
	{
		inv_roughness = 1.0f / roughness;
		float r = 1.0f - (1.0f - roughness) * (1.0f - roughness);//1.0f - roughness * roughness;
		a = Lerp(25.3245f, 21.5473f, r);
		b = Lerp(3.32435f, 3.82987f, r);
		c = Lerp(0.16801f, 0.19823f, r);
		d = Lerp(-1.27393f, -1.97760f, r);
		e = Lerp(-4.85967f, -4.32054f, r);
	}
	Cloth(float roughness_) : roughness(roughness_)
	{
		inv_roughness = 1.0f / roughness;
		float r = 1.0f - (1.0f - roughness) * (1.0f - roughness);
		a = Lerp(25.3245f, 21.5473f, r);
		b = Lerp(3.32435f, 3.82987f, r);
		c = Lerp(0.16801f, 0.19823f, r);
		d = Lerp(-1.27393f, -1.97760f, r);
		e = Lerp(-4.85967f, -4.32054f, r);
	}


	float Charlie_D(float& cos_h)
	{
		float cos_h2 = cos_h * cos_h;

		float sin_h2 = maxf(1.0 - cos_h2, 0.0078125);//1.0f - cos_h2;

		//float sin_h = sqrt14(sin_h2);

		return (2.0f + inv_roughness) * powf(sin_h2, inv_roughness * 0.5f) / (2.0f * pi);
	}

	float Charlie_L(float x)
	{
		return a / (1.0f + b * powf(abs(x), c)) + d * x + e;
	}

	float Charlie_G(float& cos_i, float& cos_o)
	{
		//float x1 = 0.5f;
		//float x2 = 0.5f;
		//float cos_i1 = 1.0f - cos_i;
		//float cos_o1 = 1.0f - cos_o;

		float lambdaV = cos_i < 0.5f ? expf(Charlie_L(cos_i)) : expf(2.0f * Charlie_L(0.5f) - Charlie_L(1.0f - cos_i));
		float lambdaL = cos_o < 0.5f ? expf(Charlie_L(cos_o)) : expf(2.0f * Charlie_L(0.5f) - Charlie_L(1.0f - cos_o));

		return 1.0f / ((1.0f + lambdaV + lambdaL) * (4.0f * cos_i * cos_o));
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float phi = 2.0f * pi * randf();

		float cos_theta = 1.0f - randf();
		float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		onb local(n);

		float cos_phi, sin_phi;

		FTA::sincos(phi, &sin_phi, &cos_phi);

		vec3 direction_out = (sin_theta * (local.u * cos_phi + local.v * sin_phi) + n * cos_theta).norm();
		
		sampling.pdf = i2pi;
		sampling.isSpecular = false;
		sampling.weight = eval(direction_in, direction_out, n, original_n, color);
		sampling.direction = direction_out;

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);
		
		if (cos_i < 0.0f)
			return vec3(0.0f);

		float cos_o = direction_out.dot(n);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, cos_i);

		float D = Charlie_D(cos_h);

		float G = Charlie_G(cos_i, cos_o);

		//return D * G * cos_o * color;

		//float denom = 4.0f * cos_i * cos_o;

		//return color * cos_o + (F * D * G) * color * cos_o;// / abs(denom) * color;
		
		//return color * ipi + (F * D * G);// *cos_o;// / abs(denom) * color;

		//vec3 spec_brdf = (D * F * G);// / (4.0f * cos_i);
		//vec3 diff_brdf = color * cos_o;// *(1.0f - F);

		//return diff_brdf + spec_brdf;

		//if (randf() < F)
		//	return D * F * G;
		//return color;

		//return Lerp(D * F * G, color, 1 - F);

		return (color + vec3(D * F * G)) * cos_o;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return i2pi;
	}
	bool isSpecular() 
	{
		return false;
	}
};

//http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
class Cloth2 : public BSDF_Sampling
{
public:

	float ior = 1.5f;
	float roughness = 0.1f;
	float inv_roughness;// = 1.0f / roughness;
	float a, b, c, d, e;

	Cloth2()
	{
		inv_roughness = 1.0f / roughness;
		float r = 1.0f - (1.0f - roughness) * (1.0f - roughness);//1.0f - roughness * roughness;
		a = Lerp(25.3245f, 21.5473f, r);
		b = Lerp(3.32435f, 3.82987f, r);
		c = Lerp(0.16801f, 0.19823f, r);
		d = Lerp(-1.27393f, -1.97760f, r);
		e = Lerp(-4.85967f, -4.32054f, r);
	}
	Cloth2(float roughness_) : roughness(roughness_)
	{
		inv_roughness = 1.0f / roughness;
		float r = 1.0f - (1.0f - roughness) * (1.0f - roughness);
		a = Lerp(25.3245f, 21.5473f, r);
		b = Lerp(3.32435f, 3.82987f, r);
		c = Lerp(0.16801f, 0.19823f, r);
		d = Lerp(-1.27393f, -1.97760f, r);
		e = Lerp(-4.85967f, -4.32054f, r);
	}


	float Charlie_D(float& cos_h)
	{
		float cos_h2 = cos_h * cos_h;

		float sin_h2 = 1.0f - cos_h2;//maxf(1.0 - cos_h2, 0.0078125);//1.0f - cos_h2;

													 //float sin_h = sqrt14(sin_h2);

		return (2.0f + inv_roughness) * powf(sin_h2, inv_roughness * 0.5f) / (2.0f * pi);
	}

	float Charlie_L(float x)
	{
		return a / (1.0f + b * powf(x, c)) + d * x + e;
	}

	float Charlie_G(float& cos_i, float& cos_o)
	{
		//float x1 = 0.5f;
		//float x2 = 0.5f;
		//float cos_i1 = 1.0f - cos_i;
		//float cos_o1 = 1.0f - cos_o;

		float lambdaV = cos_i < 0.5f ? expf(Charlie_L(cos_i)) : expf(2.0f * Charlie_L(0.5f) - Charlie_L(1.0f - cos_i));
		float lambdaL = cos_o < 0.5f ? expf(Charlie_L(cos_o)) : expf(2.0f * Charlie_L(0.5f) - Charlie_L(1.0f - cos_o));

		//Start Terminate softening
		float cos_i1 = 1.0f - cos_i;
		float cos_i1_2 = cos_i1 * cos_i1;
		float cos_i1_4 = cos_i1_2 * cos_i1_2;
		float cos_i1_8 = cos_i1_4 * cos_i1_4;
		
		float cos_o1 = 1.0f - cos_o;
		float cos_o1_2 = cos_o1 * cos_o1;
		float cos_o1_4 = cos_o1_2 * cos_o1_2;
		float cos_o1_8 = cos_o1_4 * cos_o1_4;
		//End Terminate softening

		lambdaV = powf(lambdaV, 1.0f + 2.0f * cos_i1_8);
		lambdaL = powf(lambdaL, 1.0f + 2.0f * cos_o1_8);


		return 1.0f / ((1.0f + lambdaV + lambdaL) * (4.0f * cos_i * cos_o));
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float phi = 2.0f * pi * randf();

		float cos_theta = 1.0f - randf();
		float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		onb local(n);

		float cos_phi, sin_phi;

		FTA::sincos(phi, &sin_phi, &cos_phi);

		vec3 direction_out = (sin_theta * (local.u * cos_phi + local.v * sin_phi) + n * cos_theta).norm();

		sampling.pdf = i2pi;
		sampling.isSpecular = false;
		sampling.weight = eval(direction_in, direction_out, n, original_n, color);
		sampling.direction = direction_out;

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i < 0.0f)
			return vec3(0.0f);

		float cos_o = direction_out.dot(n);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float eta = 1.0f / ior;

		//float cos_i_h = -direction_in.dot(h);

		float F = Dielectric_Reflectance(eta, cos_i);

		float D = Charlie_D(cos_h);

		float G = Charlie_G(cos_i, cos_o);

		//return D * G * cos_o * color;

		//float denom = 4.0f * cos_i * cos_o;

		//return color * cos_o + (F * D * G) * color * cos_o;// / abs(denom) * color;

		//return color * ipi + (F * D * G);// *cos_o;// / abs(denom) * color;

		//vec3 spec_brdf = (D * F * G);// / (4.0f * cos_i);
		//vec3 diff_brdf = color * cos_o;// *(1.0f - F);

		//return diff_brdf + spec_brdf;

		//if (randf() < F)
		//	return D * F * G;
		//return color;

		//return Lerp(D * F * G, color, 1 - F);

		//return (color + vec3(D * F * G)) * cos_o;

		//vec3 diff = color * cos_o;
		
		//https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/Renderer/shaders/brdf.glsl
		vec3 diff = color * (1.0f - F) * ipi;

		vec3 spec = F * D * G * pi;

		return diff + spec;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return i2pi;
	}
	bool isSpecular()
	{
		return false;
	}
};

class Ashkhmin : public BSDF_Sampling
{
public:
	float roughness = 0.5f;
	float m2;
	float ior = 1.5f;
	Ashkhmin() { m2 = roughness * roughness; }
	Ashkhmin(float roughness_) : roughness(roughness_) { m2 = roughness * roughness; }
	float AshikhminD(float& ndoth)
	{
		//float m2 = roughness * roughness;
		float cos2h = ndoth * ndoth;
		float sin2h = 1. - cos2h;
		float sin4h = sin2h * sin2h;
		//return (sin4h + 4. * exp(-cos2h / (sin2h * m2))) / (pi * (1. + 4. * m2) * sin4h);

		float cot2 = -cos2h / (m2 * sin2h);
		return 1.0 / (pi * (4.0 * m2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
	}

	float AshikhminV(float& ndotv, float& ndotl)
	{
		return 1. / (4. * (ndotl + ndotv - ndotl * ndotv));
	}

	float fresnelSchlick(float& cosTheta, float F0)
	{
		return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float phi = 2.0f * pi * randf();

		float cos_theta = 1.0f - randf();
		float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		onb local(n);

		float cos_phi, sin_phi;

		FTA::sincos(phi, &sin_phi, &cos_phi);

		vec3 direction_out = (sin_theta * (local.u * cos_phi + local.v * sin_phi) + n * cos_theta).norm();

		sampling.pdf = i2pi;
		sampling.isSpecular = false;
		sampling.weight = eval(direction_in, direction_out, n, original_n, color);
		sampling.direction = direction_out;

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		//float F = Dielectric_Reflectance(eta, cos_i);

		float F = fresnelSchlick(cos_i, eta);

		float D = AshikhminD(cos_h);

		float G = AshikhminV(cos_i, cos_o);

		//original
		//vec3 diffuse = color * cos_o;// *ipi;
		//return diffuse + F * D * G * pi * cos_o * color;
		

		vec3 diffuse = color * cos_o;// *ipi;

		return diffuse + F * D * G * pi * cos_o;
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return i2pi;
	}
	bool isSpecular()
	{
		return false;
	}
};
//http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.621.5638&rep=rep1&type=pdf
//https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
//https://www.shadertoy.com/view/4tfBzn
class Ashkhmin2 : public BSDF_Sampling
{
public:
	float roughness = 0.1f;//0.5f
	float m2;
	float ior = 1.5f;
	Ashkhmin2() { m2 = roughness * roughness; }
	Ashkhmin2(float roughness_) : roughness(roughness_) { m2 = roughness * roughness; }
	float AshikhminD(float& ndoth)
	{
		//float m2 = roughness * roughness;
		float cos2h = ndoth * ndoth;
		float sin2h = 1.0 - cos2h;
		float sin4h = sin2h * sin2h;

		//shader toy
		return (sin4h + 4.0 * exp(-cos2h / (sin2h * m2))) / (pi * (1. + 4. * m2) * sin4h);

		//float cot2 = -cos2h / (m2 * sin2h);
		//return 1.0 / (pi * (4.0 * m2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
	}

	float AshikhminV(float& ndotv, float& ndotl)
	{
		return 1. / (4. * (ndotl + ndotv - ndotl * ndotv));
	}

	float fresnelSchlick(float& cosTheta, float F0)
	{
		return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float phi = 2.0f * pi * randf();

		float cos_theta = 1.0f - randf();
		float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		onb local(n);

		float cos_phi, sin_phi;

		FTA::sincos(phi, &sin_phi, &cos_phi);

		vec3 direction_out = (sin_theta * (local.u * cos_phi + local.v * sin_phi) + n * cos_theta).norm();

		sampling.pdf = i2pi;
		sampling.isSpecular = false;
		sampling.weight = eval(direction_in, direction_out, n, original_n, color);
		sampling.direction = direction_out;

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float cos_i_h = -direction_in.dot(h);

		float F = Dielectric_Reflectance(eta, cos_i);

		//float F = fresnelSchlick(cos_i, eta);

		float D = AshikhminD(cos_h);

		float G = AshikhminV(cos_i, cos_o);

		//original
		//vec3 diffuse = color * cos_o;// *ipi;
		//return diffuse + F * D * G * pi * cos_o * color;


		//vec3 diffuse = color * cos_o;// *ipi;

		return color * cos_o + F * D * G * pi * cos_o;

		//return color;// *cos_o + D / (4 * (cos_i + cos_o - cos_i * cos_o));
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		return i2pi;
	}
	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& sample_pdf)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float cos_i_h = -direction_in.dot(h);

		float F = Dielectric_Reflectance(eta, cos_i);

		
		float D = AshikhminD(cos_h);

		float G = AshikhminV(cos_i, cos_o);

		vec3 diffuse = color * cos_o;

		sample_pdf = i2pi;

		return diffuse + F * D * G * pi * cos_o;	
	}
	
	bool isSpecular()
	{
		return false;
	}
};

class Ashkhmin_Shirley : public BSDF_Sampling
{
public:
	float roughness = 0.5f;
	float m2;
	float ior = 1.5f;
	vec3 Specular = vec3(0.7, 0.7, 0.7);
	Ashkhmin_Shirley() { m2 = roughness * roughness; }
	Ashkhmin_Shirley(float roughness_) : roughness(roughness_) { m2 = roughness * roughness; }
	float AshikhminD(float& ndoth)
	{
		//float m2 = roughness * roughness;
		float cos2h = ndoth * ndoth;
		float sin2h = 1. - cos2h;
		float sin4h = sin2h * sin2h;
		//return (sin4h + 4. * exp(-cos2h / (sin2h * m2))) / (pi * (1. + 4. * m2) * sin4h);

		float cot2 = -cos2h / (m2 * sin2h);
		return 1.0 / (pi * (4.0 * m2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
	}


	float fresnelSchlick(float& cosTheta, float F0)
	{
		return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 direction_out;

		if (randf() < 0.5f)
			direction_out = cosine_hemisphere_Sampling_with_n(n);
		else
		{
			vec3 h = ggx_sample_H(n, roughness);
			direction_out = Reflect(direction_in, h);
		}

		onb local(n);

		direction_out = (local.u * direction_out.x + local.v * direction_out.y + n * direction_out.z).norm();


		
		sampling.pdf = pdf(direction_in, direction_out, n, original_n);
		sampling.isSpecular = false;
		sampling.weight = eval(direction_in, direction_out, n, original_n, color);
		sampling.direction = direction_out;

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 h = (-direction_in + direction_out).norm();

		float cos_h = h.dot(n);

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		
		float D = AshikhminD(cos_h);


		vec3 diffuse = color * cos_o;// *ipi;

									 //return diffuse + F * D * G * pi * cos_o;

		return color;// *cos_o + D / (4 * (cos_i + cos_o - cos_i * cos_o));
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		vec3 h = (-direction_in + direction_out).norm();

		//float pdf = 
	}
	bool isSpecular()
	{
		return false;
	}
};

//https://developer.blender.org/diffusion/B/browse/master/intern/cycles/kernel/closure/bsdf_ashikhmin_velvet.h
class Ashkmin_Shirley2 : public BSDF_Sampling
{
public:
	float roughness = 0.1f;//0.5f
	float m2;
	float inv_m2;
	float ior = 1.5f;
	Ashkmin_Shirley2() { m2 = roughness * roughness; inv_m2 = 1.0f / m2; }
	Ashkmin_Shirley2(float roughness_) : roughness(roughness_) { m2 = roughness * roughness; inv_m2 = 1.0f / m2; }

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		vec3 direction_out = uniform_hemisphere_Sampling(n);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_o = direction_out.dot(n);

		float cos_h_n = h.dot(n);

		float cos_o_h = direction_out.dot(h);

		if (abs(cos_o) > 1e-5 && abs(cos_h_n) < 1.0f - 1e-5 && cos_o_h > 1e-5)
		{
			float cos_h_n_div_cos_o = cos_h_n / cos_o;
			
			cos_h_n_div_cos_o = maxf(cos_h_n_div_cos_o, 1e-5);

			float fac1 = 2.0f * abs(cos_h_n_div_cos_o * cos_o);
			float fac2 = 2.0f * abs(cos_h_n_div_cos_o * cos_i);

			float cos_h_n2 = cos_h_n * cos_h_n;
			float sin_h_n2 = 1 - cos_h_n2;
			float sin_h_n4 = sin_h_n2 * sin_h_n2;

			float cot_h_n2 = cos_h_n2 / sin_h_n2;

			float D = expf(-cot_h_n2 * inv_m2) * inv_m2 * ipi / sin_h_n4;

			float G = minf(1.0f, minf(fac1, fac2));

		}


	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{

	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{

	}
	bool isSpecular()
	{
		return false;
	}
};

//https://cgg.mff.cuni.cz/~jaroslav/papers/2010-anisobrdf/2010-anisobrdf.pdf
//this function has different brdf and weighting function

class Anisotropic_Cloth : public BSDF_Sampling
{
public:
	float mx = 2.337;//0.9f;
	float my = 2.644;//0.5f;
	float ior = 1.5f;
	float eta = 0.0666;
	float alpha = 0;//0.9f;

	Anisotropic_Cloth() { eta = 1.0f / ior; }
	Anisotropic_Cloth(float mx_, float my_) : mx(mx_), my(my_) { eta = 1.0f / ior; }
	
	vec3 Ks = vec3(0.1938, 0.0333, 0.0267);

	float q(float& tan_theta_2, float& cos_phi_m_2, float& sin_phi_m_2)
	{
		return expf(-tan_theta_2 * (cos_phi_m_2 + sin_phi_m_2));
	}

	float D_aniso(float& cos_theta_h2)
	{
		//float cos_theta_2 = 1.0f / (tan_theta_2 + 1.0f);

		float cos_theta_h4 = cos_theta_h2 * cos_theta_h2;

		//float qh = q(tan_theta_2, cos_phi_m_2, sin_phi_m_2);

		//return 1.0f / (pi * mx * my * cos_theta_4);

		return ipi / (mx * my * cos_theta_h4);
	}

	bool sample(const vec3& direction_in, vec3& n, vec3& original_n, Sampling_Struct& sampling, bool& isReflect, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		//start sample direction
		float u1 = randf();
		float u2 = randf();

		float phi_h = atanf(my / mx * tanf(2.0f * pi * u2));

		float cos_phi_h, sin_phi_h;

		FTA::sincos(phi_h, &sin_phi_h, &cos_phi_h);

		float cos_ph_m = cos_phi_h / mx;
		float sin_ph_m = sin_phi_h / my;

		float cos_ph_m2 = cos_ph_m * cos_ph_m;
		float sin_ph_m2 = sin_ph_m * sin_ph_m;

		//float theta = atanf(sqrt14(-logf(u1) / (cos_p2 + sin_p2)));

		float tan_theta_h = sqrt14(-logf(u1) / (cos_ph_m2 + sin_ph_m2));

		float theta_h = atanf(tan_theta_h);

		float cos_theta_h, sin_theta_h;

		FTA::sincos(theta_h, &cos_theta_h, &sin_theta_h);

		onb local(n);

		vec3 h = (sin_theta_h * (local.u * cos_phi_h + local.v * sin_phi_h) + cos_theta_h * n).norm();

		vec3 direction_out = Reflect(direction_in, h);
		//end sample direction

		//start compute brdf
		//1
		//F
		float cos_o = direction_out.dot(n);
		float cos_o_h = direction_out.dot(h);
		float F = Dielectric_Reflectance(eta, cos_o_h);
		//D
		float tan_theta_h2 = tan_theta_h * tan_theta_h;
		float cos_theta_h2 = cos_theta_h * cos_theta_h;
		float qh = q(cos_theta_h2, cos_ph_m2, sin_ph_m2);
		float D = D_aniso(tan_theta_h2) * qh;

		//vec3 weight = color * (1.0f - F) * ipi + Ks * F * D / (4 * cos_o_h * powf(cos_i * cos_o, alpha));

		vec3 brdf = (color * ipi * (1.0f - F) + Ks * F * D / (4 * cos_o_h * powf(cos_i * cos_o, alpha))) * cos_o;

		//end compute brdf

		//compute weight
		//float cos_theta_h2 = cos_theta_h * cos_theta_h;

		//float cos_theta_h3 = cos_theta_h2 * cos_theta_h;
		//float weight = 0.25f * ipi / (mx * my * cos_theta_h3 * cos_o_h) * qh;

		//float inv_weight = 1.0f / weight;

		/*
		//float weight = Ks * F  * cos_i / (cos_theta_h * powf(cos_i * cos_o, alpha));

		float inv_weight = 1.0f / weight;

		brdf *= inv_weight;
		*/

		//brdf = brdf / weight;

		//start compute pdf
		//use pdf = D * cos_theta / (4 * cos_oh)
		
		float sample_pdf = D * cos_theta_h / (4.0f * cos_o_h);

		/*vec3 weight = Ks * F  * cos_i / (cos_theta_h * powf(cos_i * cos_o, alpha));

		vec3 brdf2 = color * ipi * cos_o + weight;
		*/
		sampling = Sampling_Struct(direction_out, brdf, sample_pdf, false);
		

		//vec3 weight = eva
		
		//2
		/*
		float sample_pdf;

		vec3 weight = eval_and_pdf(direction_in, direction_out, n, original_n, color, sample_pdf);

		sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);
		*/

		/*float cos_o = direction_out.dot(n);
		float cos_o_h = direction_out.dot(h);
		float F = 0.041;//Dielectric_Reflectance(eta, cos_o_h);
		//D
		float tan_theta_h2 = tan_theta_h * tan_theta_h;
		float qh = q(tan_theta_h2, cos_ph_m2, sin_ph_m2);
		float D = D_aniso(tan_theta_h2) * qh;

		//vec3 weight = color * (1.0f - F) * ipi + Ks * F * D / (4 * cos_o_h * powf(cos_i * cos_o, alpha));

		vec3 weight = color * ipi + Ks * F * D / (4 * cos_o_h * powf(cos_i * cos_o, alpha));

		float sample_pdf = D * cos_theta_h / (4.0f * cos_o_h);

		weight = weight / sample_pdf;
		*/
		/*vec3 sample_weight = eval_and_pdf(direction_in, direction_out, n, original_n, color, sample_pdf);
		
		weight *= sample_weight;
		*/
		//sampling = Sampling_Struct(direction_out, weight, sample_pdf, false);

		return true;
	}
	vec3 eval(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, cos_i);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_o_h = direction_out.dot(h);

		float cos_h_n = h.dot(n);

		return Ks * F * cos_o_h * cos_i / (cos_h_n * powf(cos_i * cos_o, alpha));
		//float D = D_aniso()
	}
	float pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n)
	{
		float cos_i = -direction_in.dot(n);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_o_h = direction_out.dot(h);

		float cos_theta_h = h.dot(n);
		
		float cos_theta_h2 = cos_theta_h * cos_theta_h;

		float cos_theta_h3 = cos_theta_h2 * cos_theta_h;

		//onb local(n);

		//q(float& tan_theta_2, float& cos_phi_m_2, float& sin_phi_m_2)

		float sin_theta_h2 = 1.0f - cos_theta_h2;

		float tan_theta_h2 = sin_theta_h2 / cos_theta_h2;

		vec3 projection_vector = h - cos_theta_h * n;

		onb local(n);

		float cos_phi = projection_vector.dot(local.u);

		float cos_phi_2 = cos_phi * cos_phi;

		float sin_phi_2 = 1.0f - cos_phi_2;

		float cos_phi_m2 = cos_phi_2 / (mx * mx);
		float sin_phi_m2 = sin_phi_2 / (my * my);

		float qh = q(tan_theta_h2, cos_phi_m2, sin_phi_m2);

		return 0.25f * ipi / (mx * my * cos_theta_h3 * cos_o_h) * qh;// *
	}

	vec3 eval_and_pdf(const vec3& direction_in, const vec3& direction_out, const vec3& n, vec3& original_n, vec3& color, float& sample_pdf)
	{
		float cos_i = -direction_in.dot(n);

		if (cos_i <= 0.0f)
			return false;

		float cos_o = direction_out.dot(n);

		float eta = 1.0f / ior;

		float F = Dielectric_Reflectance(eta, cos_i);

		vec3 h = (-direction_in + direction_out).norm();

		float cos_o_h = direction_out.dot(h);

		//float cos_h_n = h.dot(n);
		float cos_theta_h = h.dot(n);

		

		//float cos_i = -direction_in.dot(n);

		//vec3 h = (-direction_in + direction_out).norm();

		//float cos_o_h = direction_out.dot(h);

		//float cos_theta_h = h.dot(n);

		float cos_theta_h2 = cos_theta_h * cos_theta_h;

		float cos_theta_h3 = cos_theta_h2 * cos_theta_h;

		//onb local(n);

		//q(float& tan_theta_2, float& cos_phi_m_2, float& sin_phi_m_2)

		float sin_theta_h2 = 1.0f - cos_theta_h2;

		float tan_theta_h2 = sin_theta_h2 / cos_theta_h2;

		vec3 projection_vector = h - cos_theta_h * n;

		onb local(n);

		float cos_phi = projection_vector.dot(local.u);

		float cos_phi_2 = cos_phi * cos_phi;

		float sin_phi_2 = 1.0f - cos_phi_2;

		float cos_phi_m2 = cos_phi_2 / (mx * mx);
		float sin_phi_m2 = sin_phi_2 / (my * my);

		float qh = q(tan_theta_h2, cos_phi_m2, sin_phi_m2);

		sample_pdf = 0.25f * ipi / (mx * my * cos_theta_h3 * cos_o_h) * qh;// *

		return color * cos_o + Ks * F * cos_o_h * cos_i / (cos_theta_h * powf(cos_i * cos_o, alpha));
	}
	bool isSpecular()
	{
		return false;
	}
};

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

float GTR1(const float& NdotH, const float& a)
{
	if (a >= 1.0f)
		return ipi;
	float a2 = a * a;
	float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
	return (a2 - 1) / (pi * logf(a2) * t);
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


#endif
