#ifndef _MATERIAL_H_
#define _MATERIAL_H_
#include "Rnd.h"
#include "Texture.h"
#include "Complex_Ior.h"
#include "fasttrigo.h"
#include "MicroFacet.h"

enum MaterialType
{
	Diffuse_type,
	Mirror_type,
	Glass_type,
	Glass_Color_type,
	Glass_No_Refract_type,
	Rough_Dielectric_type,
	Conductor_type,
	RoughConductor_type,
	RoughConductorComplex_type,
	RoughConductorSimple_type,
	Plastic_type,
	RoughPlastic_type,
	RoughPlastic_Specular_type,
	ThinSheet_type,
	ThinDielectric_type,
	SmoothCoat_type,
	//Transparent_type,
	Light_Diffuse_type,
	Disney_type
	
};

struct Material
{
	vec3 Kd = vec3(0.8, 0.8, 0.8);
	vec3 Ks;	
	vec3 Ke;
	
	
	ComplexIor complex_ior;

	//string Texname = "";
	int TexInd = 0;

	//int DiffuseInd = -1;
	//int AlphaInd = -1;
	//int BumpInd = -1;

	int type = Diffuse_type;

	bool use_texture = false;
	//block probability = 1.0f
	//alpha = 1.0f
	//when use_alpha is true
	//alpha *= sample_texture_alpha
	//when use_alpha constant

	bool alpha_material = false;
	//when use_alpha_texture is true,  
	bool use_alpha_texture = false;

	bool use_Bump = false;

	//bool use_Roughness = false;

	bool isLight = false;
	
	float roughness = 0.2f;
	float thickness = 0.1f;

	float sigma = 0.1f;
	float ior = 1.9f;
	
	//Tr = 0 Fully opaque

	float Tr = 0.0f;//Transparent, not name Transparent because name collision with Transparent Material class

	float opaque = 1.0f;

	float specular_coefficient = 10.0f;

	bool use_InterFerence = false;

	Microfacet_Distribution distribution_type = GGX;

	float gamma_texture = 3.0f;

	
	float Ni = -1;
	
	float Ns = -1;
	int d = -1;
	int illum;

	float R0;
	string Matname = "";

	vec3 Ka;
	vec3 Tf;

	
	Material() {}

};

//--------------------Smooth Coat----------------
/*
BSDF_Sample __fastcall SampleSmoothCoat(vec3& direction_in, vec3& n, vec3& scale_sigma, float& ior)
{
	float eta = 1.0f / ior;

	float cos_i = -direction_in.dot(n);

	//fi = reflectance probability
	//cos_ti = cos refraction
	float fi, cos_ti;
	Dielectric_Reflectance(eta, cos_i, fi, cos_ti);

	float average_transmitance = exp(-2.0f * scale_sigma.average());

	float sub_weight = average_transmitance * (1.0f - fi);

	float specular_weight = fi;

	float specular_prob = specular_weight / (specular_weight + sub_weight);

	if (randf() < specular_prob)
	{
		//sample.direction = reflect(direction_in, n);
		//sample.pdf = specular_prob;
		//sample.color = vec3(fi + sub_weight);
		//sample.is_specular = true;

		vec3 direction(reflect(direction_in, n));

		return BSDF_Sample(direction, vec3(fi + sub_weight), specular_prob, true);
	}

	vec3 direction_in_sub(direction_in.x * eta, -cos_ti, direction_in.z * eta);

	float sub_sample
}
*/


#endif // !_MATERIAL_H_