#ifndef _SCENE_ROUGHNESS_H_
#define _SCENE_ROUGHNESS_H_
#define _CRT_SECURE_NO_WARNINGS
//#include "GGX_Material.h"
#include "AreaLight.h"
//#include "EnvLight.h"
#include "Triangle.h"
#include "material.h"
#include "BSDF.h"
//#include "RoughPlastic.h"
#include "Enviroment.h"
//#include "All_Material.h"

#include "onb.h"
#include "PhaseFunction.h"
#include "Medium.h"
#include <unordered_map>
#include <algorithm>
#include <string>
#include <fstream>
//#include <omp.h>

#define max_depth 5
#define num_bin 12
#define max_bin 60
#define imax_bin 1 / max_bin
#define ct 1
#define ci 4

#define Light_Path_Length 2
#define Eye_Path_Length 3

using namespace std;

#define pi 3.1415926f
#define tau 6.2831853f
#define ipi 1.0f / pi
#define itau 1.0f / tau

void fixIndex(int& v, const int& n)
{
	v = v < 0 ? v + n : v > 0 ? v - 1 : -1;
}

void fixIndex(int v[3], const int& n)
{
	v[0] = v[0] < 0 ? v[0] + n : v[0] > 0 ? v[0] - 1 : -1;
	v[1] = v[1] < 0 ? v[1] + n : v[1] > 0 ? v[1] - 1 : -1;
	v[2] = v[2] < 0 ? v[2] + n : v[2] > 0 ? v[2] - 1 : -1;

}

void SkipSpace(char *&t)
{
	t += strspn(t, " \t");
}

void getdirection(const string& filepath, string& direction)
{
	size_t found = filepath.find_last_of('/\\');
	direction = filepath.substr(0, found + 1);
}

void getfilename(const string& filepath, string& filename)
{

	size_t found = filepath.find_last_of('/\\');
	filename = filepath.substr(found + 1);
}

void getdDirectionAndName(const string& filepath, string& direction, string& filename)
{
	size_t found = filepath.find_last_of('/\\');
	direction = filepath.substr(0, found + 1);
	filename = filepath.substr(found + 1);
}

static float absf(const float& x)
{
	return x > 0.0f ? x : -x;
}

struct SimpleTriangle
{
	vec3 c;
	int i;
	BBox b;
	SimpleTriangle() {}
	SimpleTriangle(vec3 c_, int i_) : c(c_), i(i_) {}
};

struct node
{
	BBox box;

	node *nodes[2];// = { NULL };

	//Da fix duoc loi bi hu cau thang
	//do range qua nho nen mot so tam giac bi mat
	//khi doi lai range = 5 thi moi thu binh tuong tro lai
	unsigned start : 21;
	unsigned range : 8;//range bao gom ca tam giac dau tien
	unsigned axis : 2; //vi du 2 tam giac 
	unsigned leaf : 1;


	node() { }
	node(int s, int r) : start(s), range(r) {}
};

struct Normal_Struct
{
	vec3 normal = vec3(0.0f, 0.0f, 0.0f);
	int count = 0;
};


void delete_tree(node* n)
{
	if (n->leaf)
	{
		free(n);
		return;
	}
	delete_tree(n->nodes[0]);
	delete_tree(n->nodes[1]);

	free(n);
}

void delete_bvh(node** n)
{
	delete_tree(*n);
	*n = NULL;
}

enum face
{
	Single_Line, Double_Line
};

void get_face_index(char*& t, const int& vs, const int& vts, const int& vns, vector<Face>& faces)
{
	string s = t;
	int length = s.find_last_of("0123456789");
	s = s.substr(0, length + 1);

	int sign = 1;
	int count = 0;
	vector<int> index;
	//vector<int> indices;
	int face_type = Single_Line;

	int num_data_per_vertex = 0;
	bool found_num_data_per_vertex = false;

	for (int i = 0; i <= length + 1; ++i)
	{
		if (s[i] == '-')
		{
			sign = -1;
		}
		else if (isdigit(s[i]))
		{
			count = 10 * count + s[i] - '0';
		}
		else if (s[i] == '/')
		{
			if (!found_num_data_per_vertex)
			{
				++num_data_per_vertex;
			}
			face_type = Single_Line;
			index.emplace_back(sign * count);
			sign = 1;
			count = 0;

			if (s[i + 1] == '/')
			{
				face_type = Double_Line;
				++i;
			}
		}
		else if (s[i] == ' ')
		{

			index.emplace_back(sign * count);
			sign = 1;
			count = 0;
			if (!found_num_data_per_vertex)
			{
				++num_data_per_vertex;
				found_num_data_per_vertex = true;
			}
		}
		else if (i == length + 1)
		{
			index.emplace_back(sign * count);
			sign = 1;
			break;
		}
	}

	//cout << index.size() << "\n";
	//cout << "\n";
	int size = index.size();

	//cout << num_data_per_vertex << "\n";
	//if (size % 2 == 0 && size % 3 == 0) //6 12 18 24 30
	//{
	if (num_data_per_vertex == 3) // v/vt/vn case   12 18 24 30 
	{
		//cout << "line 3\n";
		for (int i = 0; i < size; i += 3)
		{
			fixIndex(index[i], vs);
			fixIndex(index[i + 1], vts);
			fixIndex(index[i + 2], vns);
		}

		int start_v = 0;
		int start_vt = 1;
		int start_vn = 2;

		int num_Triangle = size / 3 - 2;

		for (int i = 0; i < num_Triangle; ++i)
		{
			Face f(index[start_v], index[start_vt], index[start_vn], index[3 * i + 3], index[3 * i + 4], index[3 * i + 5], index[3 * i + 6], index[3 * i + 7], index[3 * i + 8]);
			faces.emplace_back(f);
		}
	}
	else if (num_data_per_vertex == 2)  //  v/vt or v//vn
	{
		//cout << "line 2\n";
		if (face_type == Single_Line) // v / vt
		{
			//cout << "single\n";
			for (int i = 0; i < size; i += 2)
			{
				fixIndex(index[i], vs);
				fixIndex(index[i + 1], vts);
			}

			int num_Triangle = size / 2 - 2;

			int start_v = 0;
			int start_vt = 1;

			for (int i = 0; i < num_Triangle; ++i)
			{
				//01 23 45    01 45 67
				Face f(index[start_v], index[start_vt], -1, index[2 * i + 2], index[2 * i + 3], -1, index[2 * i + 4], index[2 * i + 5], -1);
				faces.emplace_back(f);
			}
		}
		else if (face_type == Double_Line)// v // vn
		{
			//cout << "double\n";
			for (int i = 0; i < size; i += 2)
			{
				fixIndex(index[i], vs);
				fixIndex(index[i + 1], vns);
			}

			int num_Triangle = size / 2 - 2;

			int start_v = 0;
			int start_vn = 1;

			for (int i = 0; i < num_Triangle; ++i)
			{
				Face f(index[start_v], -1, index[start_vn], index[2 * i + 2], -1, index[2 * i + 3], index[2 * i + 4], -1, index[2 * i + 5]);
				faces.emplace_back(f);
			}
		}
	}
}

static vec3 min_vec(const vec3& v1, const vec3& v2)
{
	return vec3(minf(v1.x, v2.x), minf(v1.y, v2.y), minf(v1.z, v2.z));
}

static vec3 max_vec(const vec3& v1, const vec3& v2)
{
	return vec3(maxf(v1.x, v2.x), maxf(v1.y, v2.y), maxf(v1.z, v2.z));
}

static float RGB_To_GrayScale(const float& R, const float& G, const float& B)
{
	return 0.299f * R + 0.587f * G + 0.114f * B;
}

/*static int clamp(const int& i, const int& low, const int& high)
{
return minf(maxf(low, i), high);
}*/

static int get_pixel_index(const int& i, const int& j, const int& w, const int& h)
{
	int u = clamp(i, 0, w - 1);
	int v = clamp(j, 0, h - 1);

	return v * w + u;
}

static vector<vec3> Bump_Map_To_Normal_Map(const int& w, const int& h, const int& n, const float& bump_strength, const vector<float>& grayscale)
{
	vector<vec3> normal_map;

	normal_map.resize(w * h);

	for (int i = 0; i < w; ++i)
	{
		for (int j = 0; j < h; ++j)
		{
			int top_left = get_pixel_index(i - 1, j - 1, w, h);
			int top = get_pixel_index(i - 1, j, w, h);
			int top_right = get_pixel_index(i - 1, j + 1, w, h);

			int left = get_pixel_index(i, j - 1, w, h);
			int center = get_pixel_index(i, j, w, h);
			int right = get_pixel_index(i, j + 1, w, h);


			int bottom_left = get_pixel_index(i + 1, j - 1, w, h);
			int bottom = get_pixel_index(i + 1, j, w, h);
			int bottom_right = get_pixel_index(i + 1, j + 1, w, h);

			const float tl = grayscale[top_left];
			const float t = grayscale[top];
			const float tr = grayscale[top_right];

			const float l = grayscale[left];
			const float c = grayscale[center];
			const float r = grayscale[right];

			const float bl = grayscale[bottom_left];
			const float b = grayscale[bottom];
			const float br = grayscale[bottom_right];

			// sobel filter
			const float dx = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
			const float dy = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
			const float dz = 1.0f / (bump_strength);

			vec3 n(dx, dy, dz);

			/*
			n.normalize();

			n = vec3((n.x + 1.0f) * 0.5f, (n.y + 1.0f) * 0.5f, (n.z + 1.0f) * 0.5f);

			normal_map[j * w + i] = n.norm();

			*/

			n.normalize();

			n = vec3((n.x + 1.0f) * 0.5f, (n.y + 1.0f) * 0.5f, (n.z + 1.0f) * 0.5f);

			normal_map[j * w + i] = n;

		}
	}
	return normal_map;
}

struct vertex_node
{
	vec3 position;
	vec3 normal;
	vec3 color;
	vec3 radiance;
	int triangle_index;
};

const float sigma_a_ = 0.006;//0.09
const float sigma_s_ = 0.036;//0.009;//0.05
const float sigma_h_ = 1.0f;
const float g_ = -0.5f;

struct Scene
{
	Scene(const string& filename)
	{
		sigma_a = sigma_a_;
		sigma_s = sigma_s_;
		sigma_t = sigma_a + sigma_s;
		sigma_h = sigma_h_;
		g = g_;

		ReadObj(filename, "");
	}
	Scene(const string& filename, string enviroment_path, float rotation = 0)
	{
		sigma_a = sigma_a_;
		sigma_s = sigma_s_;
		sigma_t = sigma_a + sigma_s;
		sigma_h = sigma_h_;
		g = g_;

		rotation_u = rotation;
		ReadObj(filename, enviroment_path);
	}

	Scene(const string& filename, bool use_area_light_, bool use_directional_light_, vec3 light_direction_, float directional_blur_radius_ = 0.0f, bool use_sun_color_ = true, bool compute_sky_color_ = true, string enviroment_path = "", float rotation = 0)
	{
		sigma_a = sigma_a_;
		sigma_s = sigma_s_;
		sigma_t = sigma_a + sigma_s;
		sigma_h = sigma_h_;
		g = g_;

		//cout << "use env\n";
		rotation_u = rotation;
		use_area_light = use_area_light_;
		use_sun_color = use_sun_color_;
		compute_sky_color = compute_sky_color_;

		directional_blur_radius = directional_blur_radius_ - floorf(directional_blur_radius_);//only get the fractional part 

		blur_directional_light = directional_blur_radius > 0.0f;

		inifinite_light_direction = light_direction_.norm();
		use_directional_light = use_directional_light_;
		ReadObj(filename, enviroment_path);

		if (enviroment_path != "")
		{
			//cout << "a\n";
			compute_sky_color = false;
		}
		else
		{
			use_enviroment = false;
		}
	}

	Scene(const string& filename, vec3& delta_light_position_, vec3& delta_light_Ke_)
	{
		sigma_a = sigma_a_;
		sigma_s = sigma_s_;
		sigma_t = sigma_a + sigma_s;
		sigma_h = sigma_h_;
		g = g_;

		delta_light_position = delta_light_position_;
		delta_light_Ke = delta_light_Ke_;

		ReadObj(filename, "");
	}

	float sigma_a;// = 0.045;
	float sigma_s;// = 0.025;
	float sigma_t;// = 0.07;
	float sigma_h;// = 2.0
	float g;// = 0.6;

	Medium medium;

	vector<Triangle> trs;
	vector<Material> mats;
	vector<BSDF_Sampling*> bsdfs;
	vector<vec2> vt;
	vector<vec3> vn;
	vector<Texture> texs;

	unsigned int first_shadow_node = 0;
	unsigned int second_shadow_node = 1;

	//AREA LIGHT
	AreaLight Ls;

	unordered_map<int, int> light_map;

	//DIRECTIONAL LIGHT

	//default
	//sun color = (500, 400, 100)
	//sky color = (50, 80, 100)

	//these part are for directional light only
	//start of directional light variable
	bool use_area_light = true;
	bool use_directional_light = false;
	bool use_sun_color = true;
	bool compute_sky_color = true;
	bool blur_directional_light = false;


	float directional_blur_radius = 0.0f;

	vec3 sun_color = vec3(30, 20, 10);

	//vec3 sun_color = vec3(50, 40, 10);//vec3(5, 5, 5);//(5, 4, 1);

	//Dinning Room use this color
	//vec3 sun_color = vec3(30, 30, 30);

	vec3 inifinite_light_direction;
	//end of directional light variable;

	//DELTA LIGHT OR POINT LIGHT

	vec3 delta_light_position;
	vec3 delta_light_Ke;

	int leaf = 4;

	//enviroment area
	bool use_enviroment = false;
	bool blur_enviroment = false;

	float enviroment_blur_radius = 0.001f;
	float first_bounce_blur_radius = 6.6f;

	Enviroment Env;

	//float Env_Scale = 10.0f;

	float Env_Scale = 20.0f;//20.0f

	//120 = best so far
	float rotation_u = 0.0f;// = -160 / 360.0f;

							//normal map is disable by default to save space
							//normal map is not always desirable since not having normal map result in better image
							//turn on to use
	bool use_bump_map = false;

	//normal mapping strength
	//default 1.0, increase this for greater deep

	float bump_strength = 0.6f;

	vec3 v_min = vec3(1e20);
	vec3 v_max = vec3(-1e20);

	string direction;

	void delete_before_render()
	{
		//vector<vec3>().swap(v);
		//vector<vec3>().swap(vn);
	}

	void delete_struct()
	{
		//vector<vec3>().swap(v);
		vector<vec2>().swap(vt);
		vector<vec3>().swap(vn);
		vector<Triangle>().swap(trs);
		vector<Material>().swap(mats);
		for (auto& v : texs)
		{
			vector<float>().swap(v.a);
			vector<float>().swap(v.b);
			vector<float>().swap(v.c);
			vector<float>().swap(v.r);
		}

		vector<Texture>().swap(texs);
		vector<vec3>().swap(Env.c);
	}


	void ReadMtl(const string& filename, unordered_map<string, int>& mtlMap, const string& EnviromentName = "")//mtlMap chi dung cho giai doan doc file, ender thi khong can)
	{
		//only HDR file extension support

		if (EnviromentName != "")
		{
			//cout << "use env\n";

			use_enviroment = true;
			ifstream f(direction + EnviromentName);

			int extension_position = EnviromentName.find_last_of(".");

			string extension = EnviromentName.substr(extension_position + 1);

			//cout << extension << "\n";

			/*if (extension != "hdr")
			{
			cout << "Unsupport Enviroment Extension\n";
			use_eviroment = false;
			}
			else
			{*/
			int w, h, n;

			unsigned char* c = stbi_load((direction + EnviromentName).c_str(), &w, &h, &n, 3);

			//cout << direction + EnviromentName << "\n";

			vector<vec3> enviroment_value;

			enviroment_value.resize(w * h);

			for (int i = 0; i < w * h; ++i)
			{
				//float x = powf(c[3 * i] / 255.0f, 1.0f / 2.2f);
				//float y = powf(c[3 * i + 1] / 255.0f, 1.0f / 2.2f);
				//float z = powf(c[3 * i + 2] / 255.0f, 1.0f / 2.2f);

				float x = c[3 * i] / 255.0f;
				float y = c[3 * i + 1] / 255.0f;
				float z = c[3 * i + 2] / 255.0f;

				enviroment_value[i] = vec3(Env_Scale * x, Env_Scale * y, Env_Scale * z);
			}

			Env.width = w;
			Env.height = h;
			Env.c = enviroment_value;

			Env.CDF(w, h, enviroment_value);

			free(c);

			vector<vec3>().swap(enviroment_value);
			//}
		}
		//cout << "texture path:" << direction + filename << "\n";
		ifstream f(direction + filename);
		if (!f)
			cout << "Mtl file not exist\n";



		char line[256];
		int numberOfMaterial = 0;
		while (f.getline(line, 256))
		{
			if (line[0] == 'n')
				numberOfMaterial++;
		}

		mats.resize(numberOfMaterial);
		bsdfs.resize(numberOfMaterial);
		f.clear();
		f.seekg(0, ios::beg);

		int countTexture = -1;
		int countMaterial = -1;

		char mtlname[64];
		char texturename[64];
		bool getdirection = true;
		unordered_map<string, int> temp_tex;

		Texture tex;

		bool found_texture = false;
		while (f.getline(line, 256))
		{
			char* t = line;

			SkipSpace(t);
			//bool meet_kd = false;

			if (strncmp(t, "newmtl", 6) == 0)
			{
				
				found_texture = false;
				countMaterial++;
				sscanf_s(t += 7, "%s", mtlname);

				mtlMap[mtlname] = countMaterial;
				mats[countMaterial].Matname = mtlname;
			}
			else if (strncmp(t, "type", 4) == 0)
			{
				char type[64];
				sscanf_s(t += 5, "%s", type);
				if (strncmp(type, "Mirror", 6) == 0)
					mats[countMaterial].type = Mirror_type;
				else if (strncmp(type, "GlassNoRefract", 14) == 0)
					mats[countMaterial].type = Glass_No_Refract_type;
				else if (strncmp(type, "GlassColor", 10) == 0)
					mats[countMaterial].type = Glass_Color_type;
				else if (strncmp(type, "Glass", 5) == 0)
					mats[countMaterial].type = Glass_type;
				else if (strncmp(type, "RoughDielectric", 15) == 0)
					mats[countMaterial].type = Rough_Dielectric_type;
				else if (strncmp(type, "Conductor", 9) == 0)
				{
					mats[countMaterial].type = Conductor_type;
					t += 10;
					char complex_ior_name[64];
					sscanf_s(t, "%s", complex_ior_name);

					string ior_name = complex_ior_name;

					//cout << "Conductor Test\n";
					//cout << ior_name << "\n----------\n";

					for (int i = 0; i < 40; ++i)
					{
						if (ior_name == Ior_List[i].name)
						{
							mats[countMaterial].complex_ior = Ior_List[i];
							break;
						}
					}
				}
				/*else if (strncmp(type, "RoughConductorComplex", 21) == 0)
				{
				//cout << "Complex\n";
				mats[countMaterial].type = RoughConductorComplex_type;
				t += 22;
				char complex_ior_name[64];
				sscanf_s(t, "%s", complex_ior_name);//vi sao ko co t thi no ra GGX

				string ior_name = complex_ior_name;

				//cout << "test here!\n";
				//cout << complex_ior_name << "\n";
				//cout << ior_name << "\n";

				for (int i = 0; i < 40; ++i)
				{
				if (ior_name == Ior_List[i].name)
				{
				mats[countMaterial].complex_ior = Ior_List[i];
				break;
				}
				}
				}*/
				else if (strncmp(type, "RoughConductor", 14) == 0)
				{
					//cout << "a\n";
					mats[countMaterial].type = RoughConductor_type;
					t += 15;
					char complex_ior_name[64];
					sscanf_s(t, "%s", complex_ior_name);//vi sao ko co t thi no ra GGX

					string ior_name = complex_ior_name;

					//cout << "test here!\n";
					//cout << complex_ior_name << "\n";
					//cout << ior_name << "\n";

					for (int i = 0; i < 40; ++i)
					{
						if (ior_name == Ior_List[i].name)
						{
							mats[countMaterial].complex_ior = Ior_List[i];
							break;
						}
					}
				}
				/*else if (strncmp(type, "RoughConductorSimple", 20) == 0)
				{
				//Al by deafult
				//not use any way
				mats[countMaterial].type = RoughConductorSimple_type;
				mats[countMaterial].complex_ior = Ior_List[2];
				}*/
				else if (strncmp(type, "Plastic", 7) == 0)
					mats[countMaterial].type = Plastic_type;
				else if (strncmp(type, "RoughPlastic", 12) == 0)
				{
					if (strncmp(type, "RoughPlasticSpecular", 20) == 0)
					{
						cout << "Yo!\n";
						mats[countMaterial].type = RoughPlastic_Specular_type;
					}
					else
						mats[countMaterial].type = RoughPlastic_type;
				}
			
				else if (strncmp(type, "ThinSheet", 9) == 0)
					mats[countMaterial].type = ThinSheet_type;
				else if (strncmp(type, "ThinDielectric", 14) == 0)
				{
					//cout << "Im so thin!\n";
					mats[countMaterial].type = ThinDielectric_type;
				}
				else if (strncmp(type, "SmoothCoat", 10) == 0)
				{
					mats[countMaterial].type = SmoothCoat_type;
				}
				/*else if (strncmp(type, "Transparent", 11) == 0)
				{
					mats[countMaterial].type = Transparent_type;
				}*/
				else if (strncmp(type, "LightDiffuse", 12) == 0)
					mats[countMaterial].type = Light_Diffuse_type;
				else if(strncmp(type, "Disney", 6) == 0)
					mats[countMaterial].type = Disney_type;
			}
			else if (strncmp(t, "ior", 3) == 0)
			{
				//float ior;
				//sscanf_s(t += 4, "%f", &ior);
				//mats[countMaterial].ior = ior;

				sscanf_s(t += 4, "%f", &mats[countMaterial].ior);
			}
			else if (strncmp(t, "gammar_texture", 14) == 0)
				sscanf_s(t += 15, "%f", &mats[countMaterial].gamma_texture);
			else if (strncmp(t, "roughness", 9) == 0)
			{
				sscanf_s(t += 10, "%f", &mats[countMaterial].roughness);
			}
			else if (strncmp(t, "thickness", 9) == 0)
			{
				sscanf_s(t += 10, "%f", &mats[countMaterial].thickness);
			}
			else if (strncmp(t, "sigma", 5) == 0)
				sscanf_s(t += 6, "%f", &mats[countMaterial].sigma);
			else if (strncmp(t, "spec_coeff", 10) == 0)
				sscanf_s(t += 11, "%f", &mats[countMaterial].specular_coefficient);
			else if (strncmp(t, "distribution", 12) == 0)
			{
				char distribution[32];
				sscanf_s(t += 13, "%s", distribution);

				if (strncmp(distribution, "GGX", 3) == 0)
					mats[countMaterial].distribution_type = GGX;
				if (strncmp(distribution, "Phong", 5) == 0)
					mats[countMaterial].distribution_type = Phong;
				if (strncmp(distribution, "Beckmann", 8) == 0)
					mats[countMaterial].distribution_type = Beckmann;
			}
			else if (t[0] == 'N')
			{
				if (t[1] == 'i')
				{
					sscanf_s(t += 3, "%f", &mats[countMaterial].Ni);
					float Ni = mats[countMaterial].Ni;
					float R0 = (Ni - 1) / (Ni + 1);
					mats[countMaterial].R0 = R0 * R0;
				}
				else if (t[1] == 's')
					sscanf_s(t += 3, "%f", &mats[countMaterial].Ns);
			}
			else if (t[0] == 'd')
				sscanf_s(t += 2, "%d", &mats[countMaterial].d);
			else if (t[0] == 'T')
			{
				if (t[1] == 'r')
				{
					mats[countMaterial].alpha_material = true;
					float Tr;
					sscanf_s(t += 3, "%f", &Tr);
					//cout << Tr << "\n";
					mats[countMaterial].Tr = Tr;
					mats[countMaterial].opaque = 1.0f - Tr;
				}
				else if (t[1] == 'f')
				{
					//this can be used as well for color filter
					//however, all color channel are mostly filter with the same value and mostly 1 
					//so I only use single coefficient for maximum perfomance
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Tf.x, &mats[countMaterial].Tf.y, &mats[countMaterial].Tf.z);
				}
			}
			else if (t[0] == 'i')
			{
				int illum;
				sscanf_s(t += 6, "%d", &illum);
				mats[countMaterial].illum = illum;
				//mats[countMaterial].type = Diffuse;//|= illum == 7 ? Fres : illum == 5 ? PRefl : NonS;
			}
			else if (t[0] == 'K')
			{
				if (t[1] == 'a')
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ka.x, &mats[countMaterial].Ka.y, &mats[countMaterial].Ka.z);
				else if (t[1] == 'd')
				{
				
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Kd.x, &mats[countMaterial].Kd.y, &mats[countMaterial].Kd.z);
				}
				else if (t[1] == 's')
				{
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ks.x, &mats[countMaterial].Ks.y, &mats[countMaterial].Ks.z);
				}
				else if (t[1] == 'e')
				{
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ke.x, &mats[countMaterial].Ke.y, &mats[countMaterial].Ke.z);
					if (mats[countMaterial].Ke.maxc() > 0)
					{
						//cout << "Light Detected! \n";

						//cout << mats[countMaterial].Matname << "\n";
						mats[countMaterial].isLight = true;
					}
					else
					{
						mats[countMaterial].isLight = false;
					}
				}
				
			}

			else if (strncmp(t, "map_Kd", 6) == 0)
			{
				//meet_kd = true;
				found_texture = true;
				mats[countMaterial].use_texture = true;
				sscanf_s(t += 7, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				if (temp_tex.find(realname) == temp_tex.end())
				{
					//cout << realname << "\n";

					int w, h, n;
					/*
					unsigned char* c = stbi_load((direction + realname).c_str(), &w, &h, &n, 0);

					//if (c == NULL)
					//	cout << "Not Read\n";

					//cout << (direction + realname) << "\n";
					tex.w = w;
					tex.h = h;
					tex.n = n;

					//cout << w << " " << h << " " << n << "\n";
					tex.c.resize(n * w * h);

					for (int e = 0; e < n * w * h; ++e)
					{

					tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					//tex.c[e] = powf(c[e] / 255.0f, mats[countMaterial].gamma_texture);
					//tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					//tex.c[e] = c[e] / 255.0f;
					}
					*/

					unsigned char* c = stbi_load((direction + realname).c_str(), &w, &h, &n, 0);

					tex.w = w;
					tex.h = h;
					tex.n = n;

					tex.c.resize(n * w * h);
					/*
					for (int e = 0; e < n * w * h; ++e)
					{
						tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					}
					*/
					//cout << realname << "\n";
					
					if (realname == "Lux_study_big.png")
					{
						//cout << realname << "\n";
						for (int e = 0; e < n * w * h; ++e)
						{
							tex.c[e] = powf(c[e] / 255.0f, 1.4f);
							//tex.c[e] = powf(c[e] / 255.0f, 4.0f);
							//tex.c[e] = powf(c[e] / 255.0f, 1.2f);
							//tex.c[e] = powf(c[e] / 255.0f, 1.2f);//1.7 dark
						}
					}
					else if (realname == "Lux.jpg")
						//if(realname == "jean.png" || realname == "lightning_farron.png" || realname == "ahri.png" || realname == "Ruby_Rose.png")
					{
						//cout << realname << "\n";
						for (int e = 0; e < n * w * h; ++e)
						{
							tex.c[e] = c[e] / 255.0f;
							//tex.c[e] = powf(c[e] / 255.0f, 1.2f);
							//tex.c[e] = powf(c[e] / 255.0f, 1.2f);//1.7 dark
						}
					}
					
					else
					{
						for (int e = 0; e < n * w * h; ++e)
						{
							tex.c[e] = powf(c[e] / 255.0f, 3.4f);
						}
					}

					//vector<float>().swap(tex.c);

					//tex.c = value;

					free(c);
					//free(value);


					//t.Texname = realname;
					++countTexture;

					texs.emplace_back(tex);
					temp_tex[realname] = countTexture;
					mats[countMaterial].TexInd = countTexture;
					//mats[countMaterial].Texname = realname;
				}
				else
				{
					int ind = temp_tex[realname];
					mats[countMaterial].TexInd = ind;
				}
			}

			else if (strncmp(t, "map_d", 5) == 0)
			{
				//found_texture = true;
				//mats[countMaterial].alpha_material = true;
				mats[countMaterial].use_alpha_texture = true;
				sscanf_s(t += 6, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				int w2, h2, n2;
				//stbi_load((direction + realname).c_str(), &w2, &h2, &n2, 3);

				unsigned char* a = stbi_load((direction + realname).c_str(), &w2, &h2, &n2, 0);

				tex.w2 = w2;
				tex.h2 = h2;
				tex.n2 = n2;

				tex.a.resize(n2 * w2 * h2);

				//int i = 0;
				for (int e = 0; e < n2 * w2 * h2; ++e)
				{
					tex.a[e] = a[e] / 255.0f;
				}

				free(a);
				texs[countTexture] = tex;
			}
			
			else if (strncmp(t, "map_bump", 8) == 0 && use_bump_map)
			{
				/*if (!found_texture)
				{
					++countTexture;

					
				}*/
				mats[countMaterial].use_Bump = true;
				sscanf(t += 9, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				//cout << realname << "\n";

				int w3, h3, n3;
				//stbi_load((direction + realname).c_str(), &w3, &h3, &n3, 3);

				unsigned char* bump = stbi_load((direction + realname).c_str(), &w3, &h3, &n3, 0);

				if (temp_tex.find(realname) != temp_tex.end())
					cout << realname << "\n";

				//cout << n3 << "\n";

				tex.w3 = w3;
				tex.h3 = h3;
				tex.n3 = 3;

				tex.b.resize(3 * w3 * h3);

				vector<float> gray_scale;

				gray_scale.resize(w3 * h3);

				bool is_normal_map;

				if (n3 == 1)
				{
					is_normal_map = false;
					for (int e = 0; e < w3 * h3; ++e)
					{
						gray_scale[e] = bump[e] / 255.0f;
					}
				}
				else
				{
					//n3 = 3 or n3 = 4 can be a bump map or normal map
					//a texture is normal map if 90% of its pixel have dominance blue channel
					//bump map is always greyscale and will never pass this test
					//check if this texture is bump map or normal map
					//if it is bump map, we have to convert to norml map

					//check Bump Map or Normal Map
					int count = 0;
					for (int i = 0; i < w3; ++i)
					{
						for (int j = 0; j < h3; ++j)
						{
							int index = (j * w3 + i);

							float r = bump[n3 * index] / 255.0f;
							float g = bump[n3 * index + 1] / 255.0f;
							float b = bump[n3 * index + 2] / 255.0f;

							gray_scale[index] = (r + g + b) * 0.3333f;//0.299f * r + 0.587f * g + 0.114f * b;

							if (b > g && b > r)
								++count;
						}
					}
					//truly normal map
					if (count > 0.9 * w3 * h3)
					{
						is_normal_map = true;
						for (int i = 0; i < w3 * h3; ++i)
						{
							float r = bump[n3 * i] / 255.0f;
							float g = bump[n3 * i + 1] / 255.0f;
							float b = bump[n3 * i + 2] / 255.0f;

							tex.b[3 * i] = r;
							tex.b[3 * i + 1] = g;
							tex.b[3 * i + 2] = b;
						}
					}
					else
						is_normal_map = false;
				}
				//here this is a bump map convert it to a normal map before storing
				if (!is_normal_map)
				{
					vector<vec3> normal_map = Bump_Map_To_Normal_Map(w3, h3, n3, bump_strength, gray_scale);

					for (int i = 0; i < w3 * h3; ++i)
					{
						tex.b[3 * i] = normal_map[i].x;
						tex.b[3 * i + 1] = normal_map[i].y;
						tex.b[3 * i + 2] = normal_map[i].z;
					}
				}
				if (!found_texture)
				{
					texs.emplace_back(tex);
					++countTexture;
				}//*/
				//else
				//{*/
					vector<float>().swap(gray_scale);
					free(bump);
					texs[countTexture] = tex;
				//}
			}
			
			
			/*
			else if (strncmp(t, "map_Kr", 6) == 0)
			{
				//cout << "Kr here\n";
				mats[countMaterial].use_Roughness = true;
				sscanf_s(t += 7, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				if (temp_tex.find(realname) == temp_tex.end())
				{
					//cout << realname << "\n";

					int w4, h4, n4;


					unsigned char* r = stbi_load((direction + realname).c_str(), &w4, &h4, &n4, 0);
					int count = 0;
					for (int i = 0; i < w4 * h4; ++i)
					{
						float x = r[n4 * i] / 255.0f;
						float y = r[n4 * i + 1] / 255.0f;
						float z = r[n4 * i + 2] / 255.0f;
						//if (x != y || y != z || z != x)
						//	++count;
						//cout << x << " " << y << " " << z << "\n";
					}

					tex.w4 = w4;
					tex.h4 = h4;
					tex.n4 = 1;

					tex.r.resize(w4 * h4);

					for (int e = 0; e < n4 * w4 * h4; ++e)
					{
						tex.r[e] = r[e] / 255.0f;
					}


					//vector<float>().swap(tex.c);

					//tex.c = value;

					free(r);
					//free(value);


					//t.Texname = realname;
					++countTexture;

					texs.emplace_back(tex);
					temp_tex[realname] = countTexture;
					mats[countMaterial].TexInd = countTexture;
					//mats[countMaterial].Texname = realname;
				}
				else
				{
					int ind = temp_tex[realname];
					mats[countMaterial].TexInd = ind;
				}
			}
			*/
			else if (strncmp(t, "enable_interference", 19) == 0)
			{
				char b[8];
				sscanf_s(t += 20, "s", b);

				if (strncmp(b, "false", 5) == 0)
					mats[countMaterial].use_InterFerence = false;
				if (strncmp(b, "true", 4) == 0)
					mats[countMaterial].use_InterFerence = true;
			}
		}


		for (int i = 0; i < numberOfMaterial; ++i)
		{
			if (!mats[i].isLight)
			{
				if (mats[i].type == Diffuse_type)
					bsdfs[i] = new Diffuse();
				else if (mats[i].type == Mirror_type)
				{
					bsdfs[i] = new Mirror();
				}
				else if (mats[i].type == Glass_type)
					//bsdfs[i] = new Glass(mats[i].ior);
					//bsdfs[i] = new Glass_Fresnel_Rough(mats[i].ior, 0.02);

					//bsdfs[i] = new Rough_Dielectric_Original2_Final_V(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new ThinDielectric(mats[i].ior);

					//failed
					//bsdfs[i] = new ThinSheet2();
					//bsdfs[i] = new ThinSheet(1.5f, 1.0f, 0.0f, true);

					//bsdfs[i] = new Glass_Green_Tint(mats[i].ior);
					//bsdfs[i] = new Glass_Green(mats[i].ior);
					//bsdfs[i] = new Glass(mats[i].ior);
					//bsdfs[i] = new Glass_Schlick(mats[i].ior);


					bsdfs[i] = new Glass_Fresnel(mats[i].ior);
				else if (mats[i].type == Glass_No_Refract_type)
					bsdfs[i] = new Glass_No_Refract(mats[i].ior);
				else if (mats[i].type == Glass_Color_type)
					bsdfs[i] = new Glass_Color(mats[i].Kd, mats[i].ior);
				//bsdfs[i] = new Glass_Beer_Law(mats[i].ior);
				else if (mats[i].type == Rough_Dielectric_type)
				{
					if (mats[i].roughness >= 0.001f)
						bsdfs[i] = new Rough_Glass_Fresnel(mats[i].ior, mats[i].roughness);
					else
						bsdfs[i] = new Glass_Fresnel(mats[i].ior);
				}
				//bsdfs[i] = new Glass_Fresnel(mats[i].ior);

				//bsdfs[i] = new Glass(mats[i].ior);

				//bsdfs[i] = new Glass_Fresnel_Rough(mats[i].ior, mats[i].roughness);



				//bsdfs[i] = new ThinDielectric(mats[i].ior);

				//bsdfs[i] = new Rough_Dielectric_Benedik(mats[i].ior, mats[i].roughness);

				//bsdfs[i] = new Rough_Dielectric_Paper(mats[i].ior, mats[i].roughness);

				//bsdfs[i] = new Rough_Dielectric_Sang(mats[i].roughness, mats[i].ior);

				//bsdfs[i] = new Mirror();
				//bsdfs[i] = new Rough_Dielectric_Original2_Final_V(mats[i].ior, mats[i].roughness);

				//bsdfs[i] = new Rough_Dielectric_Original_Good_Upgrade(1.5f, 0.12f);

				//bsdfs[i] = new Rough_Dielectric(1.5, 0.12);

				//bsdfs[i] = new Rough_Dielectric(mats[i].ior, mats[i].roughness);

				//bsdfs[i] = new Rough_Dielectric_Y2(mats[i].ior, mats[i].roughness);

				//bsdfs[i] = new Rough_Dielectric_Y2(mats[i].roughness, mats[i].ior);
				else if (mats[i].type == Conductor_type)
					bsdfs[i] = new Conductor(mats[i].complex_ior);
				else if (mats[i].type == RoughConductor_type || mats[i].type == RoughConductorComplex_type)
				{
					//bsdfs[i] = new Rough_Conductor_TungSten_Absolute(mats[i].roughness, mats[i].complex_ior);


					//failed
					//bsdfs[i] = new Final_Rough_Conductor_Absolute(mats[i].roughness, mats[i].complex_ior);
					
					//not exist
					//bsdfs[i] = new Rough_Conductor_TungSten_fix_black(mats[i].roughness, mats[i].complex_ior);
					//bsdfs[i] = new Final_Rough_Conductor(mats[i].roughness, mats[i].complex_ior);

					//if (!mats[i].use_Roughness)
					//{
					//bsdfs[i] = new Diffuse();
					
					//bsdfs[i] = new Rough_Conductor_TungSten_Fix_Black_Speckled(mats[i].roughness, mats[i].complex_ior);
					
					//bsdfs[i] = new Diffuse();

					bsdfs[i] = new Rough_Conductor_TungSten(mats[i].roughness, mats[i].complex_ior);
						
					/*}
					else
					{
						//cout << "here\n";
						//bsdfs[i] = new Final_Rough_Conductor_Roughness_Mapping(mats[i].complex_ior);
						bsdfs[i] = new Final_Rough_Conductor(mats[i].roughness, mats[i].complex_ior);
						
					}*/

					//bsdfs[i] = new Rough_Conductor_Yoan_Blien(mats[i].roughness)
					//bsdfs[i] = new Final_Rough_Conductor_modify(mats[i].roughness, mats[i].complex_ior);



					//bsdfs[i] = new Conductor(mats[i].complex_ior);





					//bsdfs[i] = new Rough_Dielectric_Y(mats[i].roughness, mats[i].ior);

					//bsdfs[i] = new Rough_Dielectric_Sang(mats[i].roughness, mats[i].ior);

					//bsdfs[i] = new Rough_Dielectric(mats[i].roughness, mats[i].ior);



					//bsdfs[i] = new Rough_Conductor_Visible_Normal(mats[i].roughness, mats[i].complex_ior);



					//bsdfs[i] = new Glossy(mats[i].roughness, mats[i].complex_ior);

					//bsdfs[i] = new GGX_Material(mats[i].ior, mats[i].roughness);


					//bsdfs[i] = new Rough_Conductor(mats[i].roughness, mats[i].complex_ior);

					//bsdfs[i] = new Rough_Conductor_Paper(mats[i].roughness, mats[i].complex_ior);


				}
				else if (mats[i].type == RoughConductorSimple_type)
				{
					//final_rough_conductor chi tinh theo color
					//khong tinh theo complex_ior
					bsdfs[i] = new Final_Rough_Conductor(mats[i].roughness, mats[i].complex_ior);
				}
				else if (mats[i].type == Plastic_type)
				{
					//bsdfs[i] = new Plastic_Complex(mats[i].ior, mats[i].thickness);
					//bsdfs[i] = new Plastic_Diffuse_Thickness(mats[i].ior);

					//bsdfs[i] = new Plastic_Specular(mats[i].ior);


					bsdfs[i] = new Plastic_Diffuse(mats[i].ior);

					//bsdfs[i] = new New_Plastic(mats[i].roughness, mats[i].distribution_type, mats[i].ior, mats[i].Ks);

					//bsdfs[i] = new Plastic_Complex(mats[i].ior, 0.0f);
				}
				else if (mats[i].type == RoughPlastic_type)
				{
					//bsdfs[i] = new Rough_Plastic_Tungsten(mats[i].ior, mats[i].roughness, mats[i].thickness);

					//Good
					bsdfs[i] = new Rough_Plastic_Diffuse(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new Diffuse();

					//bsdfs[i] = new Rough_Plastic_Diffuse_Thickness(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new Rough_Plastic_Diffuse_Thickness(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new RoughPlastic_Original_Diffuse_Version(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new Diffuse();

					//bsdfs[i] = new RoughPlastic_Original_Glossy_Version(mats[i].ior, mats[i].roughness);

					//bsdfs[i] = new Rough_Plastic_Specular(mats[i].ior, mats[i].roughness);


					//bsdfs[i] = new Rough_Plastic_Specular(mats[i].ior, mats[i].roughness);


					//float d = mats[i].Kd.maxc();
					//float s = mats[i].Ks.maxc();

					//float wd = d / (d + s);
					//float ws = 1.0f - wd;


					//bsdfs[i] = new Rough_Plastic(mats[i].ior, mats[i].roughness);		
				}
				else if (mats[i].type == RoughPlastic_Specular_type)
				{
					bsdfs[i] = new Rough_Plastic_Specular(mats[i].ior, mats[i].roughness);
				}
				else if (mats[i].type == ThinDielectric_type)
					bsdfs[i] = new ThinDielectric(mats[i].ior);
				else if(mats[i].type == ThinSheet_type)
					bsdfs[i] = new ThinSheet(mats[i].ior, 1.0f, 0.0f, false);
					//bsdfs[i] = new ThinDielectric(mats[i].ior);
					//bsdfs[i] = new Glass_Fresnel(mats[i].ior);
				else if (mats[i].type == SmoothCoat_type)
				{
					//BSDF_Sampling* sub_material = new Diffuse();//Plastic_Diffuse(1.9f);
					//bsdfs[i] = new SmoothCoat(mats[i].ior, mats[i].sigma, sub_material);

					bsdfs[i] = new SmoothCoatTungsten();
				}
				/*else if (mats[i].type == Transparent_type)
				{
					mats[i].alpha_material = true;
					bsdfs[i] = new Transparent(mats[i].Tr);
				}*/
				else if (mats[i].type == Light_Diffuse_type)
				{
					bsdfs[i] = new Light_Diffuse(mats[i].Kd);
				}
				else if (mats[i].type == Disney_type)
				{
					bsdfs[i] = new Disney_Clear_Coat();
				}
			}
		}
	}

	void ReadMtl2(const string& filename, unordered_map<string, int>& mtlMap, const string& EnviromentName = "")//mtlMap chi dung cho giai doan doc file, ender thi khong can)
	{
		//only HDR file extension support

		if (EnviromentName != "")
		{
			//cout << "use env\n";

			use_enviroment = true;
			ifstream f(direction + EnviromentName);

			int extension_position = EnviromentName.find_last_of(".");

			string extension = EnviromentName.substr(extension_position + 1);

			//cout << extension << "\n";

			/*if (extension != "hdr")
			{
			cout << "Unsupport Enviroment Extension\n";
			use_eviroment = false;
			}
			else
			{*/
			int w, h, n;

			unsigned char* c = stbi_load((direction + EnviromentName).c_str(), &w, &h, &n, 3);

			cout << direction + EnviromentName << "\n";

			vector<vec3> enviroment_value;

			enviroment_value.resize(w * h);

			for (int i = 0; i < w * h; ++i)
			{
				float x = c[3 * i] / 255.0f;
				float y = c[3 * i + 1] / 255.0f;
				float z = c[3 * i + 2] / 255.0f;

				enviroment_value[i] = vec3(Env_Scale * x, Env_Scale * y, Env_Scale * z);
			}

			Env.width = w;
			Env.height = h;
			Env.c = enviroment_value;

			Env.CDF(w, h, enviroment_value);

			free(c);

			vector<vec3>().swap(enviroment_value);
			//}
		}
		//cout << "texture path:" << direction + filename << "\n";
		ifstream f(direction + filename);
		if (!f)
			cout << "Mtl file not exist\n";



		char line[256];
		int numberOfMaterial = 0;
		while (f.getline(line, 256))
		{
			if (line[0] == 'n')
				numberOfMaterial++;
		}

		mats.resize(numberOfMaterial);
		bsdfs.resize(numberOfMaterial);
		f.clear();
		f.seekg(0, ios::beg);

		int countTexture = -1;
		int countMaterial = -1;

		char mtlname[64];
		char texturename[64];
		bool getdirection = true;
		unordered_map<string, int> temp_tex;

		Texture tex;

		while (f.getline(line, 256))
		{
			char* t = line;

			SkipSpace(t);

			if (strncmp(t, "newmtl", 6) == 0)
			{
				countMaterial++;
				sscanf_s(t += 7, "%s", mtlname);

				mtlMap[mtlname] = countMaterial;
				mats[countMaterial].Matname = mtlname;
			}
			
			else if (strncmp(t, "ior", 3) == 0)
			{
				//float ior;
				//sscanf_s(t += 4, "%f", &ior);
				//mats[countMaterial].ior = ior;

				sscanf_s(t += 4, "%f", &mats[countMaterial].ior);
			}
			else if (strncmp(t, "gammar_texture", 14) == 0)
				sscanf_s(t += 15, "%f", &mats[countMaterial].gamma_texture);
			else if (strncmp(t, "roughness", 9) == 0)
			{
				sscanf_s(t += 10, "%f", &mats[countMaterial].roughness);
			}
			else if (strncmp(t, "thickness", 9) == 0)
			{
				sscanf_s(t += 10, "%f", &mats[countMaterial].thickness);
			}
			else if (strncmp(t, "sigma", 5) == 0)
				sscanf_s(t += 6, "%f", &mats[countMaterial].sigma);
			else if (strncmp(t, "spec_coeff", 10) == 0)
				sscanf_s(t += 11, "%f", &mats[countMaterial].specular_coefficient);
			else if (strncmp(t, "distribution", 12) == 0)
			{
				char distribution[32];
				sscanf_s(t += 13, "%s", distribution);

				if (strncmp(distribution, "GGX", 3) == 0)
					mats[countMaterial].distribution_type = GGX;
				if (strncmp(distribution, "Phong", 5) == 0)
					mats[countMaterial].distribution_type = Phong;
				if (strncmp(distribution, "Beckmann", 8) == 0)
					mats[countMaterial].distribution_type = Beckmann;
			}
			else if (t[0] == 'N')
			{
				if (t[1] == 'i')
				{
					sscanf_s(t += 3, "%f", &mats[countMaterial].Ni);
					float Ni = mats[countMaterial].Ni;
					float R0 = (Ni - 1) / (Ni + 1);
					mats[countMaterial].R0 = R0 * R0;
				}
				else if (t[1] == 's')
					sscanf_s(t += 3, "%f", &mats[countMaterial].Ns);
			}
			else if (t[0] == 'd')
				sscanf_s(t += 2, "%d", &mats[countMaterial].d);
			else if (t[0] == 'T')
			{
				if (t[1] == 'r')
				{
					mats[countMaterial].alpha_material = true;
					float Tr;
					sscanf_s(t += 3, "%f", &Tr);
					//cout << Tr << "\n";
					mats[countMaterial].Tr = Tr;
					mats[countMaterial].opaque = 1.0f - Tr;
				}
				else if (t[1] == 'f')
				{
					//this can be used as well for color filter
					//however, all color channel are mostly filter with the same value and mostly 1 
					//so I only use single coefficient for maximum perfomance
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Tf.x, &mats[countMaterial].Tf.y, &mats[countMaterial].Tf.z);
				}
			}
			else if (t[0] == 'i')
			{
				int illum;
				sscanf_s(t += 6, "%d", &illum);
				mats[countMaterial].illum = illum;
				//mats[countMaterial].type = Diffuse;//|= illum == 7 ? Fres : illum == 5 ? PRefl : NonS;
			}
			else if (t[0] == 'K')
			{
				if (t[1] == 'a')
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ka.x, &mats[countMaterial].Ka.y, &mats[countMaterial].Ka.z);
				else if (t[1] == 'd')
				{

					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Kd.x, &mats[countMaterial].Kd.y, &mats[countMaterial].Kd.z);
				}
				else if (t[1] == 's')
				{
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ks.x, &mats[countMaterial].Ks.y, &mats[countMaterial].Ks.z);
				}
				else if (t[1] == 'e')
				{
					sscanf_s(t += 3, "%f %f %f", &mats[countMaterial].Ke.x, &mats[countMaterial].Ke.y, &mats[countMaterial].Ke.z);
					if (mats[countMaterial].Ke.maxc() > 0)
					{
						//cout << "Light Detected! \n";

						//cout << mats[countMaterial].Matname << "\n";
						mats[countMaterial].isLight = true;
					}
					else
					{
						mats[countMaterial].isLight = false;
					}
				}

			}

			else if (strncmp(t, "map_Kd", 6) == 0)
			{
				mats[countMaterial].use_texture = true;
				sscanf_s(t += 7, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				if (temp_tex.find(realname) == temp_tex.end())
				{
					//cout << realname << "\n";

					int w, h, n;
					/*
					unsigned char* c = stbi_load((direction + realname).c_str(), &w, &h, &n, 0);

					//if (c == NULL)
					//	cout << "Not Read\n";

					//cout << (direction + realname) << "\n";
					tex.w = w;
					tex.h = h;
					tex.n = n;

					//cout << w << " " << h << " " << n << "\n";
					tex.c.resize(n * w * h);

					for (int e = 0; e < n * w * h; ++e)
					{

					tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					//tex.c[e] = powf(c[e] / 255.0f, mats[countMaterial].gamma_texture);
					//tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					//tex.c[e] = c[e] / 255.0f;
					}
					*/

					unsigned char* c = stbi_load((direction + realname).c_str(), &w, &h, &n, 3);

					tex.w = w;
					tex.h = h;
					tex.n = 3;

					tex.c.resize(3 * w * h);

					for (int e = 0; e < 3 * w * h; ++e)
					{
						tex.c[e] = powf(c[e] / 255.0f, 2.6f);
					}


					//vector<float>().swap(tex.c);

					//tex.c = value;

					free(c);
					//free(value);


					//t.Texname = realname;
					++countTexture;

					texs.emplace_back(tex);
					temp_tex[realname] = countTexture;
					mats[countMaterial].TexInd = countTexture;
					//mats[countMaterial].Texname = realname;
				}
				else
				{
					int ind = temp_tex[realname];
					mats[countMaterial].TexInd = ind;
				}
			}

			else if (strncmp(t, "map_d", 5) == 0)
			{
				//mats[countMaterial].alpha_material = true;
				mats[countMaterial].use_alpha_texture = true;
				sscanf_s(t += 6, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				int w2, h2, n2;
				//stbi_load((direction + realname).c_str(), &w2, &h2, &n2, 3);

				unsigned char* a = stbi_load((direction + realname).c_str(), &w2, &h2, &n2, 0);

				//tex.w2 = w2;
				//tex.h2 = h2;
				tex.n2 = n2;

				tex.a.resize(n2 * w2 * h2);

				//int i = 0;
				for (int e = 0; e < n2 * w2 * h2; ++e)
				{
					tex.a[e] = a[e] / 255.0f;
				}

				free(a);
				texs[countTexture] = tex;
			}

			else if (strncmp(t, "map_bump", 8) == 0 && use_bump_map)
			{
				mats[countMaterial].use_Bump = true;
				sscanf(t += 9, "%s", texturename);
				string realname;

				getfilename(string(texturename), realname);

				//cout << realname << "\n";

				int w3, h3, n3;
				//stbi_load((direction + realname).c_str(), &w3, &h3, &n3, 3);

				unsigned char* bump = stbi_load((direction + realname).c_str(), &w3, &h3, &n3, 0);

				//cout << n3 << "\n";

				//tex.w3 = w3;
				//tex.h3 = h3;
				tex.n3 = 3;

				tex.b.resize(3 * w3 * h3);

				vector<float> gray_scale;

				gray_scale.resize(w3 * h3);

				bool is_normal_map;

				if (n3 == 1)
				{
					is_normal_map = false;
					for (int e = 0; e < w3 * h3; ++e)
					{
						gray_scale[e] = bump[e] / 255.0f;
					}
				}
				else
				{
					//n3 = 3 or n3 = 4 can be a bump map or normal map
					//a texture is normal map if 90% of its pixel have dominance blue channel
					//bump map is always greyscale and will never pass this test
					//check if this texture is bump map or normal map
					//if it is bump map, we have to convert to norml map

					//check Bump Map or Normal Map
					int count = 0;
					for (int i = 0; i < w3; ++i)
					{
						for (int j = 0; j < h3; ++j)
						{
							int index = (j * w3 + i);

							float r = bump[3 * index] / 255.0f;
							float g = bump[3 * index + 1] / 255.0f;
							float b = bump[3 * index + 2] / 255.0f;

							gray_scale[index] = 0.299f * r + 0.587f * g + 0.114f * b;

							if (b > g && b > r)
								++count;
						}
					}
					//truly normal map
					if (count > 0.9 * w3 * h3)
					{
						is_normal_map = true;
						for (int i = 0; i < w3 * h3; ++i)
						{
							float r = bump[3 * i] / 255.0f;
							float g = bump[3 * i + 1] / 255.0f;
							float b = bump[3 * i + 2] / 255.0f;

							tex.b[3 * i] = r;
							tex.b[3 * i + 1] = g;
							tex.b[3 * i + 2] = b;
						}
					}
					else
						is_normal_map = false;
				}
				//here this is a bump map convert it to a normal map before storing
				if (!is_normal_map)
				{
					vector<vec3> normal_map = Bump_Map_To_Normal_Map(w3, h3, n3, bump_strength, gray_scale);

					for (int i = 0; i < w3 * h3; ++i)
					{
						tex.b[3 * i] = normal_map[i].x;
						tex.b[3 * i + 1] = normal_map[i].y;
						tex.b[3 * i + 2] = normal_map[i].z;
					}
				}

				vector<float>().swap(gray_scale);
				free(bump);
				texs[countTexture] = tex;
			}
			/*
			else if (strncmp(t, "map_Kr", 6) == 0)
			{
			//cout << "Kr here\n";
			mats[countMaterial].use_Roughness = true;
			sscanf_s(t += 7, "%s", texturename);
			string realname;

			getfilename(string(texturename), realname);

			if (temp_tex.find(realname) == temp_tex.end())
			{
			//cout << realname << "\n";

			int w4, h4, n4;


			unsigned char* r = stbi_load((direction + realname).c_str(), &w4, &h4, &n4, 0);
			int count = 0;
			for (int i = 0; i < w4 * h4; ++i)
			{
			float x = r[n4 * i] / 255.0f;
			float y = r[n4 * i + 1] / 255.0f;
			float z = r[n4 * i + 2] / 255.0f;
			//if (x != y || y != z || z != x)
			//	++count;
			//cout << x << " " << y << " " << z << "\n";
			}

			tex.w4 = w4;
			tex.h4 = h4;
			tex.n4 = 1;

			tex.r.resize(w4 * h4);

			for (int e = 0; e < n4 * w4 * h4; ++e)
			{
			tex.r[e] = r[e] / 255.0f;
			}


			//vector<float>().swap(tex.c);

			//tex.c = value;

			free(r);
			//free(value);


			//t.Texname = realname;
			++countTexture;

			texs.emplace_back(tex);
			temp_tex[realname] = countTexture;
			mats[countMaterial].TexInd = countTexture;
			//mats[countMaterial].Texname = realname;
			}
			else
			{
			int ind = temp_tex[realname];
			mats[countMaterial].TexInd = ind;
			}
			}
			*/
			else if (strncmp(t, "enable_interference", 19) == 0)
			{
				char b[8];
				sscanf_s(t += 20, "s", b);

				if (strncmp(b, "false", 5) == 0)
					mats[countMaterial].use_InterFerence = false;
				if (strncmp(b, "true", 4) == 0)
					mats[countMaterial].use_InterFerence = true;
			}
		}


		for (int i = 0; i < numberOfMaterial; ++i)
		{
			if (!mats[i].isLight)
			{				
				bsdfs[i] = new Diffuse();		
			}
		}
	}

	
	void ReadObj(const string& p, const string& enviroment_path)
	{
		string filename;
		getdDirectionAndName(p, direction, filename);
		getdirection(p, direction);


		ifstream f(p);
		if (!f)
			cout << "obj file not exist";
		char line[1024], name[256];
		char mtlib[64];
		vec3 vtn;

		int mtl = -1;
		bool isLight = false;
		int count = 0;
		unordered_map<string, int> mtlMap;
		bool read_mtl = false;

		int count_light = 0;
		int num_v = 0;
		int num_vt = 0;
		int num_vn = 0;

		int fi = -1;
		int li = 0;

		vec3 v_min(1e20);
		vec3 v_max(-1e20);

		//vec3 v1(1e20);
		//vec3 v2(-1e20);

		vector<vec3> v;
		//vector<vec3> vn;

		while (f.getline(line, 1024))
		{
			char* t = line;
			int prev_space = strspn(t, " \t");
			t += prev_space;

			if (strncmp(t, "v", 1) == 0)
			{
				t += 1;
				float x, y, z;
				if (strncmp(t, " ", 1) == 0)
				{
					int post_space = strspn(t, " \t");
					t += post_space;
					sscanf_s(t, "%f %f %f", &x, &y, &z);

					vec3 vertex(x, y, z);

					v.emplace_back(vertex);
					++num_v;

					v_min = min_vec(v_min, vertex);
					v_max = max_vec(v_max, vertex);
				}
				else if (strncmp(t, "t", 1) == 0)
				{
					t += 1;
					int post_space = strspn(t, " \t");
					t += post_space;

					sscanf_s(t, "%f %f", &x, &y);
					vt.emplace_back(vec2(x, y));
					++num_vt;
				}
				else if (strncmp(t, "n", 1) == 0)
				{
					t += 1;
					int post_space = strspn(t, " \t");
					t += post_space;

					sscanf_s(t, "%f %f %f", &x, &y, &z);
					vn.emplace_back(vec3(x, y, z));
					++num_vn;
				}
			}
			else if (strncmp(t, "f", 1) == 0)
			{
				//t += 1;
				int post_space = strspn(t + 1, " \t");

				t += post_space + 1;


				int face_type;
				vector<Face> faces;
				get_face_index(t, num_v, num_vt, num_vn, faces);


				//cout << faces.size() << "\n";

				for (int i = 0; i < faces.size(); ++i)
				{

					++fi;
					int v0 = faces[i].v[0];
					int v1 = faces[i].v[1];
					int v2 = faces[i].v[2];

					vec3 p0(v[v0]);
					vec3 p1(v[v1]);
					vec3 p2(v[v2]);

					int vt0 = faces[i].vt[0];
					int vt1 = faces[i].vt[1];
					int vt2 = faces[i].vt[2];

					int vn0 = faces[i].vn[0];
					int vn1 = faces[i].vn[1];
					int vn2 = faces[i].vn[2];

					//cout << vn0 << " " << vn1 << " " << vn2 << "\n";

					//Triangle tri(p0, p1, p2, vt0, vt1, vt2, mtl);

					//if (!use_eviroment)
					//{
					//Triangle tri(p0, p1, p2, vt0, vt1, vt2, vn0, vn1, vn2, mtl);
					//trs.emplace_back(tri);
					//}
					//else
					//{

					//if (!isLight)//Missing this line cause Heap Crash
					//{
					Triangle tri(p0, p1, p2, vt0, vt1, vt2, vn0, vn1, vn2, mtl);
					trs.emplace_back(tri);
					//}
					/*else
					{
					Triangle tri(p0, p1, p2, -1, -1, -1, -1, -1, -1, mtl);
					trs.emplace_back(tri);
					}*/
					//}

					/*if (mats[mtl].isLight)
					{
					//cout << "Light here\n";
					//cout << mats[mtl].Matname << "\n";

					Ls.add_light(p0, p1, p2);
					Ls.Ke.emplace_back(mats[mtl].Ke);

					//cout << fi << " " << li << " " << mats[mtl].Matname << "\n";
					light_map[fi] = li;
					++li;
					}*/
					/*int mtl = trs[i].mtl;

					if (mats[mtl].Ke.maxc() > 0.0f)
					{
					vec3 p0 = trs[i].p0;
					vec3 p1 = p0 + trs[i].e1;
					vec3 p2 = p0 + trs[i].e2;

					Ls.add_light(p0, p1, p2);
					Ls.Ke.emplace_back(mats[mtl].Ke);

					light_map[i] = li;
					++li;
					}*/
				}
			}
			else if (strncmp(t, "usemtl", 6) == 0)//(t[0] == 'u')
			{
				sscanf_s(t += 7, "%s", name);
				string realname = (string)name;
				//cout << realname << "\n";

				//hang chet tiet nay sinh ra lo sai
				//no phai la mtl = mtlMap[realname];
				//chu khong phai la
				//mtlMap.find(realname)->second;

				mtl = mtlMap[realname];

				if (mtlMap.find(realname) == mtlMap.end())
					cout << realname << " not exist in obj file\n";

				//cout << name << "\n";

				//vec3 Ke = mats[mtl].Ke;
				//cout << realname << "\n";
				/*if (realname == "Spot_holder")
				{
				cout << realname << "\n";
				cout << Ke.x << " " << Ke.y << " " << Ke.z << "\n";
				cout << Ke.maxc() << "\n";
				}*/
				/*if (Ke.maxc() > 0.0f)
				{
				//cout << "light good\n";
				isLight = true;
				}
				else
				isLight = false;*/
			}
			else if (t[0] == 'm' && !read_mtl)//mtllib
			{
				sscanf_s(t += 7, "%s", mtlib);

				string realname = (string)mtlib;

				ReadMtl(realname, mtlMap, enviroment_path);
				//ReadMtl2(realname, mtlMap);// , enviroment_path);
				read_mtl = true;
			}
		}

		//add light source

		//cout << "Add Light Source\n";
		//cout << trs.size() << "\n";

		/*for (int i = 0; i < trs.size(); ++i)
		{
		int mtl = trs[i].mtl;

		if (mats[mtl].Ke.maxc() > 0.0f)
		{
		vec3 p0 = trs[i].p0;
		vec3 p1 = p0 + trs[i].e1;
		vec3 p2 = p0 + trs[i].e2;

		Ls.add_light(p0, p1, p2);
		Ls.Ke.emplace_back(mats[mtl].Ke);

		light_map[i] = li;
		++li;
		}
		}*/

		/*if (use_area_light)
		{
		if (Ls.Ke.size() == 0)
		{
		cout << "Im here\n";
		//cout << "Add Light On Top\n";
		//vertex1
		vec3 v1(v_min.x, v_max.y, v_min.z);
		//vertex2
		vec3 v2(v_min.x, v_max.y, v_max.z);
		//vertex3
		vec3 v3(v_max.x, v_max.y, v_max.z);
		//vertex4
		vec3 v4(v_max.x, v_max.y, v_min.z);

		Ls.add_light(v1, v3, v2);
		Ls.add_light(v1, v4, v3);

		//failed
		//Ls.add_light(v1, v2, v4);
		//Ls.add_light(v2, v3, v4);


		vec3 ke1(4.0f);
		vec3 ke2(4.0f);

		//Ls.Ke.resize(2);

		Ls.Ke.emplace_back(ke1);
		Ls.Ke.emplace_back(ke2);

		//Ls.Ke[0] = ke1;
		//Ls.Ke[1] = ke2;

		//still have directional light
		//but when you dont want to use enviroment
		//this function will use area light instead
		if (!use_eviroment)
		{
		int mtl = mats.size();

		Material mat;
		mat.isLight = true;
		mat.Ke = vec3(4.0f);

		mats.emplace_back(mat);

		Triangle tr1(v1, v3, v2, -1, -1, -1, -1, -1, -1, mtl);
		Triangle tr2(v1, v4, v3, -1, -1, -1, -1, -1, -1, mtl);

		trs.emplace_back(tr1);
		trs.emplace_back(tr2);
		}
		}
		Ls.norm();
		}*/
		//Ls.norm();
		//smooth normal

		/*int size = v.size();

		vn.resize(size);

		unordered_map<int, Normal_Struct> normal_map;
		for (int i = 0; i < trs.size(); ++i)
		{
		int v0 = trs[i].vn0;
		int v1 = trs[i].vn1;
		int v2 = trs[i].vn2;

		vec3 p0 = v[v0];
		vec3 p1 = v[v1];
		vec3 p2 = v[v2];

		vn[v0] = ((p1 - p0).cross(p2 - p0)).norm();

		/*vec3 p10 = (p1 - p0).norm();
		vec3 p21 = (p2 - p1).norm();
		vec3 p02 = (p0 - p2).norm();
		*/
		/*
		vec3 n0 = ((p1 - p0).cross(p2 - p0)).norm();
		vec3 n1 = ((p2 - p1).cross(p0 - p1)).norm();
		vec3 n2 = ((p0 - p2).cross(p1 - p2)).norm();

		float a0 = (p1 - p0).dot(p2 - p0);
		float a1 = (p2 - p1).dot(p0 - p1);
		float a2 = (p0 - p2).dot(p1 - p2);
		*/

		/*
		vec3 n0 = ((p10).cross(-p02)).norm();
		vec3 n1 = ((p21).cross(-p10)).norm();
		vec3 n2 = ((p02).cross(-p21)).norm();

		float a0 = (p10).dot(-p02);
		float a1 = (p21).dot(-p10);
		float a2 = (p02).dot(-p21);

		normal_map[v0].normal += a0 * n0;
		normal_map[v1].normal += a1 * n1;
		normal_map[v2].normal += a2 * n2;
		*/

		//vn[v0] = n0;

		/*normal_map[v0].normal += n0;
		normal_map[v0].count++;

		normal_map[v1].normal += n1;
		normal_map[v1].count++;

		normal_map[v2].normal += n2;
		normal_map[v2].count++;
		}
		*/

		//int i = 0;

		/*for (auto& n : normal_map)
		{
		//n.second.normal /= n.second.count;
		//vn[i++] = n.second.normal.norm();

		vn[i++] = n0;
		}*/

		vector<vec3>().swap(v);
		//vector<vec3>().swap(vn);


		//normal_map.clear();
	}

	void sah(node*& n, const int& start, const int& range, vector<BBox>& boxes, vector<SimpleTriangle>& simp)
	{
		n = new node(start, range + 1);//range //original range
		for (auto i = start; i <= start + range; ++i)//original < start + range se dan den ket qua sai
		{
			int ind = simp[i].i;
			simp[i].b = boxes[ind];
			n->box.expand(boxes[ind]);
		}
		if (range < leaf)
		{
			n->leaf = 1;
			n->axis = n->box.maxDim();
			int axis = n->axis;
			sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
				return s1.c[axis] < s2.c[axis];
			});
			return;
		}
		else
		{
			n->leaf = 0;
			n->range = 0;
			int best_split = 0, best_axis = -1;

			float best_cost = ci * range;
			float area = n->box.area();
			vec3 vmin = n->box.bbox[0], vmax = n->box.bbox[1];
			vec3 extend(vmax - vmin);

			for (int a = 0; a < 3; ++a)
			{
				sort(simp.begin() + start, simp.begin() + start + range, [a](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[a] <= s2.c[a];
				});

				//float min_box = n->box.bbox[0][a], length = n->box.bbox[1][a] - min_box;
				float length = n->box.bbox[1][a] - n->box.bbox[0][a];
				//if (length < 0.000001f)
				//	continue;

				vector<BBox> right_boxes;
				right_boxes.resize(range);

				BBox left = simp[start + 0].b;

				right_boxes[range - 1] = simp[start + range - 1].b;

				for (int j = range - 2; j >= 0; --j)
				{
					right_boxes[j] = right_boxes[j + 1].expand_box(simp[start + j].b);
				}

				float extend = length / range;

				int count_left = 1;
				int count_right = range - 1;
				float inv_a = 1.0f / area;
				for (int i = 0; i < range - 1; ++i)
				{
					float left_area = left.area();
					float right_area = right_boxes[i + 1].area();

					BBox right = right_boxes[i + 1];

					float cost = ct + ci * (left_area * count_left + right_area * count_right) * inv_a;

					if (cost < best_cost)
					{
						best_cost = cost;
						best_axis = a;
						best_split = count_left;
					}
					++count_left;
					--count_right;
					left.expand(simp[start + i + 1].b);
				}
			}
			if (best_cost == ci * range)
			{
				n->leaf = 1;
				n->range = range;
				n->axis = n->box.maxDim();
				int axis = n->axis;
				sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[axis] < s2.c[axis];
				});
				return;
			}

			n->axis = best_axis;

			sort(simp.begin() + start, simp.begin() + start + range, [best_axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
				return s1.c[best_axis] < s2.c[best_axis];
			});

			sah(n->nodes[0], start, best_split, boxes, simp);

			sah(n->nodes[1], start + best_split, range - best_split, boxes, simp);
		}
	}

	void sah2(node*& n, const int& start, const int& range, vector<BBox>& boxes, vector<SimpleTriangle>& simp)
	{
		n = new node(start, range);//range //original range
		for (auto i = start; i < start + range; ++i)//original < start + range se dan den ket qua sai
		{
			int ind = simp[i].i;
			simp[i].b = boxes[ind];
			n->box.expand(boxes[ind]);
		}
		if (range <= leaf)
		{
			n->leaf = 1;
			n->axis = n->box.maxDim();
			int axis = n->axis;
			sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
				return s1.c[axis] < s2.c[axis];
			});
			return;
		}
		else
		{
			n->leaf = 0;
			n->range = 0;
			int best_split = 0, best_axis = -1;

			float best_cost = ci * range;
			float area = n->box.area();
			vec3 vmin = n->box.bbox[0], vmax = n->box.bbox[1];
			vec3 extend(vmax - vmin);

			for (int a = 0; a < 3; ++a)
			{
				sort(simp.begin() + start, simp.begin() + start + range, [a](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[a] <= s2.c[a];
				});

				//float min_box = n->box.bbox[0][a], length = n->box.bbox[1][a] - min_box;
				float length = n->box.bbox[1][a] - n->box.bbox[0][a];
				//if (length < 0.000001f)
				//	continue;

				vector<BBox> right_boxes;
				right_boxes.resize(range);

				BBox left = simp[start + 0].b;

				right_boxes[range - 1] = simp[start + range - 1].b;

				for (int j = range - 2; j >= 0; --j)
				{
					right_boxes[j] = right_boxes[j + 1].expand_box(simp[start + j].b);
				}

				float extend = length / range;

				int count_left = 1;
				int count_right = range - 1;
				float inv_a = 1.0f / area;
				for (int i = 0; i < range - 1; ++i)
				{
					float left_area = left.area();
					float right_area = right_boxes[i + 1].area();

					BBox right = right_boxes[i + 1];

					float cost = ct + ci * (left_area * count_left + right_area * count_right) * inv_a;

					if (cost < best_cost)
					{
						best_cost = cost;
						best_axis = a;
						best_split = count_left;
					}
					++count_left;
					--count_right;
					left.expand(simp[start + i + 1].b);
				}
			}
			if (best_cost == ci * range)
			{
				n->leaf = 1;
				n->range = range;
				n->axis = n->box.maxDim();
				int axis = n->axis;
				sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[axis] < s2.c[axis];
				});
				return;
			}

			n->axis = best_axis;

			sort(simp.begin() + start, simp.begin() + start + range, [best_axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
				return s1.c[best_axis] < s2.c[best_axis];
			});

			sah2(n->nodes[0], start, best_split, boxes, simp);

			sah2(n->nodes[1], start + best_split, range - best_split, boxes, simp);
		}
	}


	//failed
	void sah_bin(node*& n, const int& start, const int& range, vector<BBox>& boxes, vector<SimpleTriangle>& simp)
	{
		n = new node(start, range);//range
		for (auto i = start; i < start + range; ++i)//<start + range
		{
			int ind = simp[i].i;
			simp[i].b = boxes[ind];
			n->box.expand(boxes[ind]);
		}
		if (range < leaf)
		{
			n->leaf = 1;
			n->axis = n->box.maxDim();
			int axis = n->axis;
			sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
				return s1.c[axis] < s2.c[axis];
			});
			return;
		}
		else
		{
			//range < bin use full swep

			/*if (range < num_bin)
			{
			n->leaf = 0;
			n->range = 0;
			int best_split = 0, best_axis = -1;

			float best_cost = ci * range;
			float area = n->box.area();
			vec3 vmin = n->box.bbox[0], vmax = n->box.bbox[1];
			vec3 extend(vmax - vmin);

			for (int a = 0; a < 3; ++a)
			{
			sort(simp.begin() + start, simp.begin() + start + range, [a](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
			return s1.c[a] <= s2.c[a];
			});

			//float min_box = n->box.bbox[0][a], length = n->box.bbox[1][a] - min_box;
			float length = n->box.bbox[1][a] - n->box.bbox[0][a];
			//if (length < 0.000001f)
			//	continue;

			vector<BBox> right_boxes;
			right_boxes.resize(range);

			BBox left = simp[start + 0].b;

			right_boxes[range - 1] = simp[start + range - 1].b;

			for (int j = range - 2; j >= 0; --j)
			{
			right_boxes[j] = right_boxes[j + 1].expand_box(simp[start + j].b);
			}

			float extend = length / range;

			int count_left = 1;
			int count_right = range - 1;
			float inv_a = 1.0f / area;
			for (int i = 0; i < range - 1; ++i)
			{
			float left_area = left.area();
			float right_area = right_boxes[i + 1].area();

			BBox right = right_boxes[i + 1];

			float cost = ct + ci * (left_area * count_left + right_area * count_right) * inv_a;

			if (cost < best_cost)
			{
			best_cost = cost;
			best_axis = a;
			best_split = count_left;
			}
			++count_left;
			--count_right;
			left.expand(simp[start + i + 1].b);
			}
			}
			if (best_cost == ci * range)
			{
			n->leaf = 1;
			n->range = range;
			n->axis = n->box.maxDim();
			int axis = n->axis;
			sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
			return s1.c[axis] < s2.c[axis];
			});
			return;
			}

			n->axis = best_axis;

			sort(simp.begin() + start, simp.begin() + start + range, [best_axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
			{
			return s1.c[best_axis] < s2.c[best_axis];
			});

			sah(n->nodes[0], start, best_split, boxes, simp);

			sah(n->nodes[1], start + best_split, range - best_split, boxes, simp);
			}
			else
			{*/
			n->leaf = 0;
			n->range = 0;
			int best_split = 0, best_axis = -1;

			float best_cost = ci * range;
			float area = n->box.area();
			vec3 vmin = n->box.bbox[0], vmax = n->box.bbox[1];
			vec3 extend(vmax - vmin);

			float inv_a = 1.0f / area;

			for (int a = 0; a < 3; ++a)
			{
				sort(simp.begin() + start, simp.begin() + start + range, [a](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[a] <= s2.c[a];
				});

				float length = n->box.bbox[1][a] - n->box.bbox[0][a];

				vector<BBox> bins;

				bins.resize(num_bin);

				float bin_length = length / num_bin;

				float i_bin_length = 1.0f / bin_length;

				//compute bound for each bin
				//slow way
				/*for (int i = 0; i < range - 1; ++i)
				{
				//float distance_to_start = simp[i].c - simp[0].c;

				int bin_index = int((simp[i].c[a] - simp[0].c[a]) * i_bin_length);

				bins[bin_index].expand(simp[i].b);
				}*/

				//fast way

				float bin_limit = bin_length;
				int bin_index = 0;
				for (int i = 0; i < range - 1; ++i)
				{
					if (simp[i].c[a] <= bin_limit)
						bins[bin_index].expand(simp[i].b);
					else
					{
						++bin_index;
						bin_limit += bin_length;
					}
				}
				//compute bin area
				vector<float> bin_left_area;
				bin_left_area.resize(num_bin - 1);

				//prefix array
				BBox current_bin_left;

				for (int i = 0; i < num_bin - 1; ++i)
				{
					current_bin_left.expand(bins[i]);

					bin_left_area[i] = current_bin_left.area();
				}

				//compute from right to lef
				BBox curren_bin_right;

				for (int i = num_bin - 1; i > 0; --i)
				{
					curren_bin_right.expand(bins[i]);

					float left_area = bin_left_area[i - 1];

					float right_area = curren_bin_right.area();

					float cost = ct + ci * (left_area * (i - 1) + right_area * (num_bin - i - 1)) * inv_a;

					if (cost < best_cost)
					{
						best_cost = cost;
						best_axis = a;
						best_split = i - 1;
					}
				}

				if (best_cost == ci * range)
				{
					n->leaf = 1;
					n->range = range;
					n->axis = n->box.maxDim();
					int axis = n->axis;
					sort(simp.begin() + start, simp.begin() + start + range, [axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
					{
						return s1.c[axis] < s2.c[axis];
					});
					return;
				}

				n->axis = best_axis;

				sort(simp.begin() + start, simp.begin() + start + range, [best_axis](const SimpleTriangle& s1, const SimpleTriangle& s2)
				{
					return s1.c[best_axis] < s2.c[best_axis];
				});

				sah(n->nodes[0], start, best_split, boxes, simp);

				sah(n->nodes[1], start + best_split, range - best_split, boxes, simp);

				//}
			}
		}
	}


	void build_bvh(node*& n, const int& l)
	{
		leaf = l;
		int s = trs.size();

		vector<BBox> boxes;
		vector<SimpleTriangle> simp;
		boxes.resize(s);
		simp.resize(s);

		for (int i = 0; i < s; ++i)
		{
			vec3 p0 = trs[i].p0;

			boxes[i].expand(p0);
			boxes[i].expand(p0 + trs[i].e1);
			boxes[i].expand(p0 + trs[i].e2);
			simp[i] = { boxes[i].c(), i };
		}

		sah2(n, 0, s, boxes, simp);


		vector<Triangle> new_trs;

		new_trs.resize(s);

		for (int i = 0; i < s; ++i)
		{
			new_trs[i] = trs[simp[i].i];
		}
		trs = new_trs;


		vector<SimpleTriangle>().swap(simp);


		vector<Triangle>().swap(new_trs);

		//Add Light Source
		if (use_area_light)
		{
			int li = 0;
			for (int i = 0; i < trs.size(); ++i)
			{
				int mtl = trs[i].mtl;

				if (mats[mtl].isLight)
				{
					vec3 p0 = trs[i].p0;
					vec3 p1 = p0 + trs[i].e1;
					vec3 p2 = p0 + trs[i].e2;

					Ls.add_light(p0, p1, p2);
					Ls.Ke.emplace_back(mats[mtl].Ke);

					light_map[i] = li++;

				}
			}

			if (Ls.Ke.size() == 0)
			{
				//cout << "Im here\n";
				//cout << "Add Light On Top\n";
				//vertex1
				vec3 v1(v_min.x, v_max.y, v_min.z);
				//vertex2
				vec3 v2(v_min.x, v_max.y, v_max.z);
				//vertex3
				vec3 v3(v_max.x, v_max.y, v_max.z);
				//vertex4
				vec3 v4(v_max.x, v_max.y, v_min.z);

				Ls.add_light(v1, v3, v2);
				Ls.add_light(v1, v4, v3);

				//failed
				//Ls.add_light(v1, v2, v4);
				//Ls.add_light(v2, v3, v4);


				vec3 ke1(4.0f);
				vec3 ke2(4.0f);

				//Ls.Ke.resize(2);

				Ls.Ke.emplace_back(ke1);
				Ls.Ke.emplace_back(ke2);

				//Ls.Ke[0] = ke1;
				//Ls.Ke[1] = ke2;

				//still have directional light
				//but when you dont want to use enviroment
				//this function will use area light instead
				if (!use_enviroment)
				{
					int mtl = mats.size();

					Material mat;
					mat.isLight = true;
					mat.Ke = vec3(4.0f);

					mats.emplace_back(mat);

					Triangle tr1(v1, v3, v2, -1, -1, -1, -1, -1, -1, mtl);
					Triangle tr2(v1, v4, v3, -1, -1, -1, -1, -1, -1, mtl);

					trs.emplace_back(tr1);
					trs.emplace_back(tr2);
				}
			}
			Ls.norm();

		}

	}


	void build_bvh_bin(node*& n, const int& l)
	{
		leaf = l;
		int s = trs.size();

		vector<BBox> boxes;
		vector<SimpleTriangle> simp;
		boxes.resize(s);
		simp.resize(s);

		for (int i = 0; i < s; ++i)
		{
			vec3 p0 = trs[i].p0;

			boxes[i].expand(p0);
			boxes[i].expand(p0 + trs[i].e1);
			boxes[i].expand(p0 + trs[i].e2);
			simp[i] = { boxes[i].c(), i };
		}

		//sah(n, 0, s, boxes, simp);
		sah_bin(n, 0, s, boxes, simp);

		vector<Triangle> new_trs;

		new_trs.resize(s);

		for (int i = 0; i < s; ++i)
		{
			new_trs[i] = trs[simp[i].i];
		}
		trs = new_trs;


		vector<SimpleTriangle>().swap(simp);


		vector<Triangle>().swap(new_trs);

	}

	//failed
	void build_bvh_no_extra_triangle_space(node*& n, const int& l)
	{
		leaf = l;
		int s = trs.size();

		vector<BBox> boxes;
		vector<SimpleTriangle> simp;
		boxes.resize(s);
		simp.resize(s);

		for (int i = 0; i < s; ++i)
		{
			vec3 p0 = trs[i].p0;

			boxes[i].expand(p0);
			boxes[i].expand(p0 + trs[i].e1);
			boxes[i].expand(p0 + trs[i].e2);
			simp[i] = { boxes[i].c(), i };
		}

		sah(n, 0, s, boxes, simp);


		/*vector<Triangle> new_trs;

		new_trs.resize(s);

		for (int i = 0; i < s; ++i)
		{
		new_trs[i] = trs[simp[i].i];
		}
		trs = new_trs;
		*/

		unordered_map<int, int> d_map;

		for (int i = 0; i < s; ++i)
		{
			d_map[simp[i].i] = i;
		}

		for (int i = 0; i < s; ++i)
		{
			if (simp[i].i != i)
			{
				int real_pos_index = d_map[i];

				swap(simp[i].i, simp[real_pos_index].i);
				swap(trs[simp[i].i], trs[real_pos_index]);

			}
		}


		vector<SimpleTriangle>().swap(simp);


		//vector<Triangle>().swap(new_trs);

	}


	bool __fastcall all_hit(node*& n, Ray& r, HitRecord& rec)
	{
		node* stack[26];

		stack[0] = n;
		float mint = 10000000.0f;
		int si = 0;
		while (si >= 0)
		{
			auto top(stack[si]);
			si--;

			float tl;

			if (top->box.hit_axis_tl(r, tl) && tl < mint)
			//if (top->box.hit_axis_tl_mint(r, tl, mint))
			{
				if (!top->leaf)
				{
					int sort_axis = r.sign[top->axis];

					stack[si + 1] = top->nodes[sort_axis];
					stack[si + 2] = top->nodes[1 - sort_axis];
					si += 2;
				}
				else
				{
					auto start = top->start, end = start + top->range;

					for (auto i = start; i < end; ++i)
					{
						trs[i].optimize_2(r, rec, &mint, i);
					}

				}
			}
			else
				continue;

		}
		if (mint == 10000000.0f)
		{
			return false;
		}

		return true;
	}

	vec2 __fastcall intp(const vec2& a, const vec2& b, const vec2& c, const float& u, const float& v)
	{
		return a * (1.0f - u - v) + b * u + c * v;
	}

	vec3 __fastcall intp(const vec3& a, const vec3& b, const vec3& c, const float& u, const float& v)
	{
		return a * (1.0f - u - v) + b * u + c * v;
	}
	bool __fastcall hit_color_alpha(node*& n, Ray& r, HitRecord& rec, vec3* color, float& alpha)
	{
		node* stack[32];

		//stack[0] = n;
		int a = r.sign[n->axis];

		stack[0] = n->nodes[a];
		stack[1] = n->nodes[1 - a];
		float mint = 10000000.0f;//1e9f;
		int si = 1;

		//ray_t new_ray(r.o, r.invd);

		while (si >= 0)
		{
			auto top(stack[si]);
			--si;

			float tl;


			if (top->box.hit_axis_tl(r, tl) && tl <= mint)
				//if (top->box.hit_axis_tl_mint(r, tl, mint))// && tl < mint)
			{

				int sort_axis = r.sign[top->axis];

				stack[si + 1] = top->nodes[sort_axis];
				stack[si + 2] = top->nodes[1 - sort_axis];

				si += 2;

				if (top->leaf)
				{
					si -= 2;
					auto start = top->start, end = start + top->range;
					
					for (auto i = start; i < end; ++i)
					{
						trs[i].optimize_2(r, rec, &mint, i);						
					}
					
					/*unsigned int start = top->start, end = start + top->range;
					for (unsigned int i = end - 1; i >= start && i > 0; --i)
					{
						trs[i].optimize_2(r, rec, &mint, i);
					}*/
				}
			}
			else
				continue;

		}
		if (mint < 10000000.0f)//1e9f)
		{
			rec.t = mint;
			uint32_t ind = rec.ti, mtl_ind = trs[ind].mtl;

			int ti = rec.ti;

			int vn0 = trs[ti].vn[0];
			int vn1 = trs[ti].vn[1];
			int vn2 = trs[ti].vn[2];

			//vec3 rec_normal = intp(vn[vn0], vn[vn1], vn[vn2], rec.u, rec.v);

			//rec.n = (vn[vn0] + vn[vn1] + vn[vn2]) / 3.0f;//intp(vn[vn0], vn[vn1], vn[vn2], rec.u, rec.v);

			//rec.n = rec_normal;

			//rec.n = (vn[vn0] + vn[vn1] + vn[vn2]) / 3.0f;//intp(vn[vn0], vn[vn1], vn[vn2], rec.u, rec.v);

			//if (vn0 < 0)
			//	cout << "negative normal index\n";

			//cout << vn0 << "\n";

			//if (trs[ti].compute_normal)
			//	cout << "compute normal\n";

			rec.n = trs[ti].compute_normal ? intp(vn[vn0], vn[vn1], vn[vn2], rec.u, rec.v).norm() : trs[ti].n.norm();

			/*if (trs[ti].compute_normal)
			{
			int vn0 = trs[ti].vn[0];
			int vn1 = trs[ti].vn[1];
			int vn2 = trs[ti].vn[2];

			rec.n = intp(vn[vn0], vn[vn1], vn[vn2], rec.u, rec.v);
			}
			else
			rec.n = trs[ti].n;
			*/
			//rec.n = trs[ti].n;
			alpha = mats[mtl_ind].opaque;
			if (mats[mtl_ind].use_texture)
			{
				int t0 = trs[ind].vt0,
					t1 = trs[ind].vt1,
					t2 = trs[ind].vt2;

				vec2 t(intp(vt[t0], vt[t1], vt[t2], rec.u, rec.v));

				int tex_ind = mats[mtl_ind].TexInd;

				*color = texs[tex_ind].ev(t);

				//opaque cua transparent material se de xu ly rieng
				

							 /*if (!mats[mtl_ind].alpha_material)
							 {
							 alpha = 1.0f;
							 //return true;
							 }
							 else
							 {
							 alpha = texs[tex_ind].ev_alpha(t);

							 //return true;
							 }*/

				if (mats[mtl_ind].use_alpha_texture)
				{
					float alpha_texture = texs[tex_ind].ev_alpha(t);
					alpha *= alpha_texture;
				}

				if (mats[mtl_ind].use_Bump)
				{
					//cout << "yes\n";
					vec3 original_rec_n = rec.n;

					//vec3 bump = texs[tex_ind].ev_bump(t);

					//vec3 bump = texs[tex_ind].ev_bump(t);

					vec3 bump = texs[tex_ind].ev_bump(t);

					bump = (2.0f * bump - vec3(1.0f));
					//bump.normalize();
					//onb local(rec.n);
					//local.u = -local.u;

					//rec.n = (rec.n + local.u * bump.x + local.v * bump.y + local.w * bump.z).norm();

					//(rec.n.x * bump.x + rec.n.y * bump.y + rec.n.z * bump.z);


					//bump 1
					//vec3 bump_normal = rec.n * bump;

					//rec.n = (rec.n * bump).norm();

					//bad
					//bump2

					//vec3 bump_normal = rec.n + rec.n * bump;

					//rec.n = bump_normal.norm();

					//wrong
					//failed
					//rec.n = (rec.n + bump.x * local.u + bump.y * local.v + bump.z * local.w).norm();
					//failed
					//rec.n = (rec.n + bump.x * local.u + bump.y * local.v).norm();

					//rec.n = (rec.n + bump.x * local.u + bump.y * local.v + bump.z * local.w).norm();

					//New Bump Map

					//vec3 P_u = bump.x * local.u;
					//vec3 P_v = bump.y * local.v;
					//rec.n = (rec.n + P_u + P_v).norm();

					//rec.n = bump;// .norm();

					//rec.n = (rec.n + bump.x * (rec.n.cross(local.u)) - bump.y * (rec.n.cross(local.v))).norm();

					//rec.n = (rec.n + bump.x * local.u.cross(rec.n) + bump.y * local.v.cross(rec.n)).norm();


					//TBN

					vec3 e1 = trs[ind].e1;
					vec3 e2 = trs[ind].e2;

					vec2 delta_uv1 = vt[t1] - vt[t0];
					vec2 delta_uv2 = vt[t2] - vt[t0];

					float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);

					vec3 T, B, N;

					N = rec.n;

					T = f * (e1 * delta_uv2.y - e2 * delta_uv1.y);
					B = f * (e2 * delta_uv1.x - e1 * delta_uv2.x);

					T = B.cross(N).norm();
					B = T.cross(N).norm();


					//failed
					//rec.n += rec.n.z * bump.z + T * bump.x + B * bump.y;
					//rec.n.normalize();

					//final_good
					//can not change with normal strength
					//rec.n += rec.n * bump.z + T * bump.x + B * bump.y;
					//rec.n.normalize();

					//can not change with strength
					//final good 2
					//rec.n += T * bump.x + B * bump.y;
					//rec.n.normalize();

					//rec.n = vec3(max(original_rec_n.x, rec.n.x), max(original_rec_n.y, rec.n.y), max(original_rec_n.z, rec.n.z));

					//rec.n.normalize();

					//final good 3
					//best version
					//the rest failed in some ways
					//darker when strength in crease


					rec.n = (rec.n * bump.z + T * bump.x + B * bump.y).norm();



					//rec.n.normalize();

					//final good 4
					//not good too
					//rec.n += rec.n + T * bump.x + B * bump.y;
					//rec.n.normalize();

					//final version
					//rec.n = (rec.n + (T * bump.x + B * bump.y)).norm();


					//test final good
					//rec.n = (rec.n + rec.n * bump.z + T * bump.x + B * bump.y).norm();

					//test final good 2
					//rec.n = (rec.n + T * bump.x + B * bump.y).norm();

					//test final good 3
					//rec.n = (rec.n * bump.z + T * bump.x + B * bump.y).norm();


					//rec.n.normalize();

					//rec.n.z += bump.z;

					//rec.n = (rec.n + bump.x * rec.n.cross(B) + bump.y * rec.n.cross(T)).norm();


					//failed
					//rec.n = rec.n * bump.z + T * bump.x + B * bump.y;
					//}
				}

				/*else
				{
				vec3 e1 = trs[ind].e1;
				vec3 e2 = trs[ind].e2;

				vec2 delta_uv1 = vt[t1] - vt[t0];
				vec2 delta_uv2 = vt[t2] - vt[t0];

				float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);

				vec3 T, B, N;

				N = rec.n;

				T = f * (e1 * delta_uv2.y - e2 * delta_uv1.y);
				B = f * (e2 * delta_uv1.x - e1 * delta_uv2.x);

				T = B.cross(N).norm();

				//rec.n = (bump.x * N + bump.y * T ).norm();


				}
				*/


				return true;

			}

			//*color = mats[mtl_ind].Kd;//(mats[mtl_ind].Kd.maxc() > 0 ? mats[mtl_ind].Kd : mats[mtl_ind].Ks);

			*color = mats[mtl_ind].Kd;//(mats[mtl_ind].Kd.maxc() > 0 ? mats[mtl_ind].Kd : mats[mtl_ind].Ks);


			return true;
		}
		return false;

	}

	//changeed
	//bool __fastcall hit_anything_range(node*& n, Ray& r, const float& d)
	bool __fastcall hit_anything_range(node*& n, Ray& r)
	{
		node* stack[32];

		stack[0] = n->nodes[second_shadow_node];
		stack[1] = n->nodes[first_shadow_node];

		int si = 1;

		//stack[0] = n;

		while (si >= 0)
		{
			auto top(stack[si--]);

			float th;

			if (top->box.hit_shadow_th(r, th))// && th >= 0.0f)
			{
				if (!top->leaf)
				{

					stack[si + 1] = top->nodes[second_shadow_node];
					stack[si + 2] = top->nodes[first_shadow_node];


					si += 2;
				}
				else
				{
					auto start = top->start, end = start + top->range;

					for (auto i = start; i < end; ++i)
						//for (auto i = end - 1; i >= start; ++i)
					{
						int mtl = trs[i].mtl;
						//if (trs[i].hit_anything(r, d))// && !mats[mtl].isLight)
						//if(trs[i].hit_anything(r, d) && !mats[mtl].isLight)
						if (trs[i].hit_anything(r) && !mats[mtl].isLight)
							return true;
					}
				}
			}
			else
				continue;
		}
		return false;
	}


	bool __fastcall hit_anything_alpha(node*& n, Ray& r)
	{
		node* stack[32];

		stack[0] = n->nodes[second_shadow_node];
		stack[1] = n->nodes[first_shadow_node];

		int si = 1;

		//stack[0] = n;

		while (si >= 0)
		{
			auto top(stack[si--]);

			float th;

			if (top->box.hit_shadow_th(r, th))// && th >= 0.0f)
			{
				if (!top->leaf)
				{

					stack[si + 1] = top->nodes[second_shadow_node];
					stack[si + 2] = top->nodes[first_shadow_node];


					si += 2;
				}
				else
				{
					auto start = top->start, end = start + top->range;

					for (auto i = start; i < end; ++i)
						//for (auto i = end - 1; i >= start; ++i)
					{
						int mtl = trs[i].mtl;
						//if (trs[i].hit_anything(r, d))// && !mats[mtl].isLight)
						//if(trs[i].hit_anything(r, d) && !mats[mtl].isLight)

						HitRecord rec;
						if (trs[i].hit_anything_alpha(r, rec) && !mats[mtl].isLight)
						{
							if (!mats[mtl].alpha_material)
								return true;
							else
							{
								//this retrun false cause all our pain
								//this is always false and the most occluded part is litted light hell fire
								//return false;
								float alpha = mats[mtl].opaque;

								if (mats[mtl].use_alpha_texture)
								{
									int t0 = trs[i].vt0,
										t1 = trs[i].vt1,
										t2 = trs[i].vt2;

									vec2 t(intp(vt[t0], vt[t1], vt[t2], rec.u, rec.v));

									int tex_ind = mats[mtl].TexInd;
									float alpha_texture = texs[tex_ind].ev_alpha(t);

									alpha *= alpha_texture;
								}

								if (randf() < alpha)
									return true;
								else
									continue;
							}
						}
					}
				}
			}
			else
				continue;
		}
		return false;
	}

	bool __fastcall hit_anything_range_alpha(node*& n, Ray& r, float& d)
	{
		node* stack[32];

		stack[0] = n->nodes[second_shadow_node];
		stack[1] = n->nodes[first_shadow_node];

		int si = 1;

		//stack[0] = n;

		while (si >= 0)
		{
			auto top(stack[si--]);

			float th;

			if (top->box.hit_shadow_th(r, th))// && th >= 0.0f)
			{
				if (!top->leaf)
				{

					stack[si + 1] = top->nodes[second_shadow_node];
					stack[si + 2] = top->nodes[first_shadow_node];


					si += 2;
				}
				else
				{
					auto start = top->start, end = start + top->range;

					for (auto i = start; i < end; ++i)
					{
						int mtl = trs[i].mtl;

						HitRecord rec;
						if (trs[i].hit_anything_alpha(r, rec) && rec.t <= d)
						{
							if (!mats[mtl].alpha_material)
								return true;
							else
							{
								float alpha = mats[mtl].opaque;

								if (mats[mtl].use_alpha_texture)
								{
									int t0 = trs[i].vt0,
										t1 = trs[i].vt1,
										t2 = trs[i].vt2;

									vec2 t(intp(vt[t0], vt[t1], vt[t2], rec.u, rec.v));

									int tex_ind = mats[mtl].TexInd;
									float alpha_texture = texs[tex_ind].ev_alpha(t);

									alpha *= alpha_texture;
								}

								if (randf() < alpha)
									return true;
								else
									continue;
							}
						}
					}
				}
			}
			else
				continue;
		}
		return false;
	}

	float __fastcall mis2(const float& pdf1, const float& pdf2)
	{
		float pdf1_2 = pdf1 * pdf1;
		return pdf1_2 / (pdf1_2 + pdf2 * pdf2);
	}

	vec3 __fastcall sibenik_tracing(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		//vec3 Le(2.0f);
		for (int i = 0; i < 60; ++i)
		{
			float alpha;
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{

				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				if (cos_incident > 0)
					rec.n = -rec.n;

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					if (specular)
					{
						//Le 111
						L += T * mats[mtl].Ke;
						//L += T * Le;
						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						//float cos_mtl = light_direction.dot(prev_normal);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];
						//float pdf_mtl = cos_mtl * ipi;

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Le 111
						L += T * mats[li].Ke * mis_weight;

						//L += T * Le * mis_weight;
						return L;
						//return vec3(0.0f);
						//break;
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (type == Diffuse_type)
				{

					//T *= color * ipi;

					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);



					//if (!hit_anything_range2(n, light_ray, length * 0.999f))
					if (!hit_anything_range(n, light_ray))
					{


						float cos_light = -light_direction.dot(Ls.normal[li]);
						float cos_mtl = light_direction.dot(rec.n);

						if (cos_light > 0.0f && cos_mtl > 0.0f)
						{
							//if (Ls.area[li] == 0)
							//	cout << "zero area light\n";

							//cos_light = max(cos_light, 0.0f);
							//cos_mtl = max(cos_mtl, 0.0f);

							//https://computergraphics.stackexchange.com/questions/4792/path-tracing-with-multiple-lights
							//
							//light sampling
							//When sampling the lights, you just select one light l with the appropriate probability P(l) and then sample the light surface / angle with its own sampling strategy(with PDF pl).The resulting PDF p will be :
							//p(x) = P(l)∗pl(x)

							float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

							//if (pdf_light == 0)
							//	cout << "pdf light zero \n";

							vec3 bsdf_eval(color * cos_mtl * ipi);
							float bsdf_pdf = cos_mtl * ipi;

							//vec3 bsdf_eval = color;
							//float bsdf_pdf = cos_mtl * ipi;

							float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

							L += T * mats[li].Ke * bsdf_eval * mis / pdf_light;
							//L += T * Le * bsdf_eval * mis / pdf_light;
						}

					}


					//onb local(new_ray.d, rec.n);
					//onb local(rec.n);

					//vec3 coord(cosine_hemisphere());
					//vec3 new_dir((coord.x * local.u + coord.y * local.v + coord.z * local.w).norm());

					vec3 new_dir = cosine_hemisphere_Sampling_with_n(rec.n);

					T *= color;

					new_ray = Ray(hit_point, new_dir);
					prev_pdf = new_dir.dot(rec.n) * ipi;
					//prev_normal = rec.n;
					specular = false;

				}
				else if (type == Mirror_type)
				{
					//float cos_incident = new_ray.d.dot(rec.n);
					vec3 new_dir((new_ray.d - 2.0f * cos_incident * rec.n));
					new_ray = Ray(hit_point, new_dir);
					prev_pdf = 1.0f;
					specular = true;
				}
				else if (type == Glass_type)
				{
					//float cos_incident = new_ray.d.dot(rec.n);
					new_ray.d.normalize();
					float glassIndex = 1.5f;

					float eta = cos_incident <= 0 ? 1.0f / glassIndex : glassIndex;
					float R;
					specular = true;


					float c1 = -new_ray.d.dot(rec.n);
					c1 = abs(c1);
					float c2 = 1.0f - eta * eta * (1.0f - c1 * c1);
					vec3 refractive_direction;

					if (c2 < 0)
					{
						R = 1.0f;
					}
					else
					{
						refractive_direction = (eta * new_ray.d + (eta * c1 - sqrt14(c2)) * rec.n).norm();


						float cos = cos_incident < 0 ? c1 : -refractive_direction.dot(rec.n);

						cos = abs(cos);
						//float x = 1.0f - cos;
						float R0 = (glassIndex - 1.0f) / (glassIndex + 1.0f);
						R0 = R0 * R0;
						R = R0 + (1.0f - R0) * powf(1 - cos, 5);
						//R = R0 + (1.0f - R0) * powf(1.0f - cos, 5);
					}

					if (randf() < R)
					{
						vec3 new_dir((new_ray.d - 2.0f * cos_incident * rec.n));
						new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, new_dir);
						prev_pdf = 1.0f;
						//prev_normal = rec.n;
					}
					else
					{
						new_ray = Ray(new_ray.o + new_ray.d * rec.t - 0.0002f * rec.n, refractive_direction);
						prev_pdf = 1.0f;
						//prev_normal = -rec.n;
					}
				}

				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

					/*float q = max(0.2f, 1.0f - T.maxc());
					T /= (1.0f - q);
					if (randf() < q)
					break;*/
				}
			}
			else
			{
				L += T;
				return L;
				//return vec3(0.0f, 1.0f, 0.0f);
				//return L;// +T;
				//break;
			}
		}
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		vec3 Le(4.0f);


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;

						L += T * Le;

						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						L += T * Le * mis_weight;

						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					Ray light_ray(hit_point, light_direction);


					float cos_light = -light_direction.dot(Ls.normal[li]);
					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					if (cos_mtl > 0.0f && cos_light > 0.0f && !hit_anything_range(n, light_ray))
					{

						float ilength = 1.0f / length;
						light_direction *= ilength;

						cos_light *= ilength;
						cos_mtl *= ilength;


						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, rec.n, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, rec.n);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						L += T * Le * bsdf_eval * mis / pdf_light;

					}


					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, rec.n, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}
					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, rec.n, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				L += T;

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				return L;

			}
		}
		return L;
	}


	vec3 __fastcall sibenik_tracing_new_artificial_light(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		vec3 prev_normal;

		Ray new_ray(r);
		bool specular = true;

		vec3 Le(30.0f);


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_n = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;

						//L += T * Le;

						vec3 radiance_value(T * Le);

						if (radiance_value.maxc() <= Le.x)
							L += radiance_value;


						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);
						//float cos_mtl = light_direction.dot(prev_normal);

						//float pdf_light = length2 / (Ls.area[li] * cos_light * cos_mtl) * Ls.pdf[li];

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						//L += T * Le * mis_weight;

						vec3 radiance_value(T * Le * mis_weight);

						if (radiance_value.maxc() <= Le.x)
							L += radiance_value;



						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					Ray light_ray(hit_point, light_direction);


					float cos_light = -light_direction.dot(Ls.normal[li]);
					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{

						float ilength = 1.0f / length;
						light_direction *= ilength;

						cos_light *= ilength;
						//cos_mtl *= ilength;


						//better than use cos_mtl
						//float pdf_light = (length * length) / (Ls.area[li] * cos_light * cos_mtl) * Ls.pdf[li];//cos_mtl

						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_n, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_n);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						//failed
						//if (bsdf_pdf > 1e-6 && pdf_light > 1e-6)
						//if (bsdf_pdf > 1e-6)

						//L += T * Le * bsdf_eval * mis / pdf_light;

						vec3 radiance_value(T * Le * bsdf_eval * mis / pdf_light);

						if (radiance_value.maxc() <= Le.x)
							L += radiance_value;


					}


					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_n, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;
					//prev_normal = rec.n;

					if (!sampling.isSpecular)
					{
						onb local(rec.n);

						vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						new_ray = Ray(new_hit_point, direction);

						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}
					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_n, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					//prev_normal = rec.n;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				L += T * Le;

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				return L;

			}
		}
		return L;
	}
	//Bidirectional Path Tracing Area

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Le_clamp(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		vec3 Le(30.0f);
		float firefly_threshold = 4.0f;

		float Le_max = Le.x * firefly_threshold;

		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)// || mats[mtl].type == Light_Diffuse_type)
				{
					//if (mats[mtl].type == Light_Diffuse_type)
					//	Le = mats[mtl].Kd;

					//Le = mats[mtl].isLight ? vec3(30.0f) : mats[mtl].Kd;

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;
						if (T.minc() >= 0.0f && T.maxc() < firefly_threshold)
							L += T * Le;

						///if (L.minc() < 0.0f)
						//	return vec3(0.0f);
						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = abs(-light_direction.dot(rec.n));

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//if (pdf_light < eps)
						//	return vec3(0.0f);

						float mis_weight = mis2(prev_pdf, pdf_light);



						//Good
						//L += T * mats[mtl].Ke * mis_weight;

						//if (T.minc() >= 0.0f)
						if (T.minc() >= 0.0f && T.maxc() < firefly_threshold)
							L += T * Le * mis_weight;

						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						//if (L.minc() < 0.0f)
						//	return vec3(0.0f);

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = minf(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);



					float cos_mtl = light_direction.dot(rec.n);

					
					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{

						
						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						
						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						vec3 weight = T * bsdf_eval * mis / pdf_light;

						if (weight.minc() >= 0.0f && weight.maxc() < firefly_threshold)
							L += weight * T;
					}


					Sampling_Struct sampling;
					bool isReflect;

					

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b || sampling.pdf <= eps)// || sampling.weight.minc() < 0.0f)// || sampling.direction.dot(rec.n) < 0.0f)// || sampling.pdf < eps)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					

					new_ray = Ray(new_hit_point, sampling.direction);
					specular = sampling.isSpecular;

					T *= sampling.weight;
				}
				else
				{
					
					Sampling_Struct sampling;
					bool isReflect;


					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b || sampling.pdf <= eps)// || sampling.weight.minc() < 0.0f)// || sampling.direction.dot(rec.n) < 0.0f)// || sampling.pdf < eps)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf() || cont_prob <= eps)
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{
				//if (T.minc() >= 0.0f)
				L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

					   //vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
					   //return clamp_L;
					   //if (L.minc() < 0.0f)
					   //	return vec3(0.0f);

				return L;

			}
		}
		//if (L.minc() < 0.0f)
		//	return vec3(0.0f);
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Le(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		vec3 Le(50.0f);
		//float firefly_threshold = 10.0f;

		//float Le_max = Le.x * firefly_threshold;

		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)// || mats[mtl].type == Light_Diffuse_type)
				{
					//if (mats[mtl].type == Light_Diffuse_type)
					//	Le = mats[mtl].Kd;

					//Le = mats[mtl].isLight ? vec3(30.0f) : mats[mtl].Kd;

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;
						if (T.minc() >= 0.0f)// && T.maxc() < firefly_threshold )
							L += T * Le;

						///if (L.minc() < 0.0f)
						//	return vec3(0.0f);
						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = abs(-light_direction.dot(rec.n));

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//if (pdf_light < eps)
						//	return vec3(0.0f);

						//float mis_weight = mis2(prev_pdf, pdf_light);



						//Good
						//L += T * mats[mtl].Ke * mis_weight;

						//if (T.minc() >= 0.0f)
						if (T.minc() >= 0.0f)// && T.maxc() < firefly_threshold)
							L += T * Le * mis2(prev_pdf, pdf_light);// / prev_pdf;
											 //L += T * Le * mis_weight;

						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						//if (L.minc() < 0.0f)
						//	return vec3(0.0f);

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = minf(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


				
					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					//if (cos_mtl > 0.0f && cos_light > 0.0f && !hit_anything_range(n, light_ray))
					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{

						//float ilength = 1.0f / length;
						//light_direction *= ilength;

						//cos_light *= ilength;
						//cos_mtl *= ilength;


						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						//L += T * mats[mtl].Ke * bsdf_eval * mis / pdf_light;

						//if (T.minc() >= 0.0f && bsdf_eval.minc() >= 0.0f)// && pdf_light > eps)
						
						
						
						if (pdf_light > eps)
							L += T * Le * bsdf_eval * mis / pdf_light;


						//vec3 weight = T * bsdf_eval * mis / pdf_light;

						//if (weight.minc() >= 0.0f);// && weight.maxc() < firefly_threshold)
						//	L += weight * T;
					}


					Sampling_Struct sampling;
					bool isReflect;

					//vec3 sample_normal = rec.n;

					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	sample_normal = -rec.n;

					/*vec3 original_normal = rec.n;

					if (bsdfs[mtl]->have_refraction() && reverse_normal)
					original_normal = -rec.n;
					*/

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b || sampling.pdf <= eps)// || sampling.weight.minc() < 0.0f)// || sampling.direction.dot(rec.n) < 0.0f)// || sampling.pdf < eps)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					/*if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}*/

					new_ray = Ray(new_hit_point, sampling.direction);
					specular = sampling.isSpecular;

					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;

					Sampling_Struct sampling;
					bool isReflect;



					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	original_normal = -rec.n;


					//bool b;
					//if (!bsdfs[mtl]->isSampleVolume())
					//b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b || sampling.pdf <= eps)// || sampling.weight.minc() < 0.0f)// || sampling.direction.dot(rec.n) < 0.0f)// || sampling.pdf < eps)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf() || cont_prob <= eps)
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{
				//if (T.minc() >= 0.0f)
					//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

					   //vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
					   //return clamp_L;
				//if (L.minc() < 0.0f)
				//	return vec3(0.0f);

				return L;

			}
		}
		//if (L.minc() < 0.0f)
		//	return vec3(0.0f);
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Ke(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;

						//if (T.minc() < 0.0f)
						//	cout << "Specular T reach light negative\n";
						L += T * Ke;

						return L;
					}
					else
					{
						//original
						/*
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						//L += T * mats[mtl].Ke * mis_weight;

						L += T * Ke * mis_weight;

						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;
						*/
						//if (mats[mtl].isLight)
						//{
							int li = light_map[rec.ti];

							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();


							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							float mis_weight = mis2(prev_pdf, pdf_light);

							//Good
							//L += T * mats[mtl].Ke * mis_weight;

						//	if (T.minc() < 0.0f)
							//	cout << "Non Specular T reach light negative\n";

							L += T * Ke * mis_weight;

							//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
							//return clamp_L;

							return L;
						//}
						/*else
						{
							//if (T.minc() < 0.0f)
							//	cout << "Non Specular T reach Diffuse light negative\n";
							L += T * Ke;
							return L;

							//use this and no light appear
							//float pdf_light = 1.0f;
							//float mis_weight = mis2(prev_pdf, pdf_light);
							//L += T * Ke * mis_weight;

						}*/
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{

						//float ilength = 1.0f / length;
						//light_direction *= ilength;

						//cos_light *= ilength;

						//cos_mtl *= ilength;

						//float cos_light = -light_direction.dot(Ls.normal[li]);
						//cos_light = abs(cos_light);

						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						///if (T.minc() < 0.0f)
						//	cout << "Sample Light T negative\n";
						//if (bsdf_eval.minc() < 0.0f)
						//	cout << "bsdf_eval Sample Light negative\n";

						L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;

					}


					Sampling_Struct sampling;
					bool isReflect;

					//vec3 sample_normal = rec.n;

					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	sample_normal = -rec.n;

					/*vec3 original_normal = rec.n;

					if (bsdfs[mtl]->have_refraction() && reverse_normal)
					original_normal = -rec.n;
					*/

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					/*if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}*/

					new_ray = Ray(new_hit_point, sampling.direction);

					//if (sampling.weight.minc() < 0.0f)
					//	cout << "Sampling Weight Negative\n";
					specular = sampling.isSpecular;

					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;

					Sampling_Struct sampling;
					bool isReflect;



					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	original_normal = -rec.n;


					//bool b;
					//if (!bsdfs[mtl]->isSampleVolume())
					//b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				//return L;

				return L;
			}
		}
		//return L;
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Ke_alpha(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					//new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.0002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;
					//cout << "Light here!\n";
					if (specular)
					{
						//Le 111
						if (T.minc() >= 0.0f)
							L += T * Ke;//mats[mtl].Le;


						return L;
					}
					else
					{
						if (mats[mtl].isLight)
						{
							/*if (light_map.find(rec.ti) == light_map.end())
							{
							cout << "Light not map\n";
							cout << rec.ti << "\n";
							cout << mats[mtl].Matname << "\n";
							}*/
							int li = light_map[rec.ti];

							//cout << rec.ti << " " << li << "\n";
							//if (li != 0)
							//cout << rec.ti << " " << li << "\n";
							//	cout << li << "\n";
							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();


							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							float mis_weight = mis2(prev_pdf, pdf_light);

							//Good
							if (T.minc() >= 0.0f)
								L += T * Ke * mis_weight;
							//L += T * mats[mtl].Ke * mis_weight;

							//L += T * Ls.Ke[li] * mis_weight;


							//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
							//return clamp_L;

							return L;

							//return vec3(1.0f, 0, 0);
						}
						else
						{
							L += T * mats[mtl].Kd;
							return L;
						}
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


					//float cos_light = -light_direction.dot(Ls.normal[li]);
					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					if (cos_mtl > 0.0f && !hit_anything_alpha(n, light_ray))
					{

						//float ilength = 1.0f / length;
						//light_direction *= ilength;

						//cos_light *= ilength;

						//cos_mtl *= ilength;

						//cos_light = abs(cos_light)

						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						if (bsdf_eval.minc() < 0.0f)
							break;
						
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						//failed at this line for dabrovic
						//work for the rest
						//if (li != 0 && li != 1)
						//	cout << li << "\n";

						//vi sao T * mats[mtl].ke co ket qua
						//con T * Ls.ke[li] va T * 4 thi sai

						//cout << Ls.Ke[li].maxc() << "\n";

						//cout << mats[mtl].Ke.maxc() << "\n";

						//L += T * mats[mtl].Ke * bsdf_eval * mis / pdf_light;
						//cout << Ls.Ke.size() << "\n";
						if (T.minc() >= 0.0f)
							L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;// *4.0f;
																		 //L = L * 4.0f;
					}


					Sampling_Struct sampling;
					bool isReflect;

					//vec3 sample_normal = rec.n;

					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	sample_normal = -rec.n;

					/*vec3 original_normal = rec.n;

					if (bsdfs[mtl]->have_refraction() && reverse_normal)
					original_normal = -rec.n;
					*/

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					prev_pdf = sampling.pdf;
					//if (prev_pdf < 1e-6f)
					//	return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					
					
					/*if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}*/

					new_ray = Ray(new_hit_point, sampling.direction);
					specular = sampling.isSpecular;

					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;

					Sampling_Struct sampling;
					bool isReflect;



					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	original_normal = -rec.n;


					//bool b;
					//if (!bsdfs[mtl]->isSampleVolume())
					//b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					/*if (mats[mtl].type == Transparent_type)
					{
						new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.0002f, sampling.direction);
						continue;
					}*/

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					if (sampling.weight.minc() < 0.0f)
						return vec3(0.0f);

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						//Twist for scene having huge occlusion part
						//L += T;
						//return L;
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				//L += T * 5;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				//return L;

				return L;
			}
		}
		//return L;
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Ke_alpha_dabrovic(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;

						L += T * Ke;

						return L;
					}
					else
					{
						//original
						/*
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						//L += T * mats[mtl].Ke * mis_weight;

						L += T * Ke * mis_weight;

						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;
						*/
						if (mats[mtl].isLight)
						{
							int li = light_map[rec.ti];

							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();


							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							float mis_weight = mis2(prev_pdf, pdf_light);

							//Good
							//L += T * mats[mtl].Ke * mis_weight;

							L += T * Ke * mis_weight;

							//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
							//return clamp_L;

							return L;
						}
						else
						{
							L += T * Ke;
							return L;

							//use this and no light appear
							//float pdf_light = 1.0f;
							//float mis_weight = mis2(prev_pdf, pdf_light);
							//L += T * Ke * mis_weight;

						}
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					//hit anything alpha = failed
					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{

						//float ilength = 1.0f / length;
						//light_direction *= ilength;

						//cos_light *= ilength;

						//cos_mtl *= ilength;

						float cos_light = -light_direction.dot(Ls.normal[li]);
						cos_light = abs(cos_light);

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * abs(cos_light)) * Ls.pdf[li];//cos_mtl

																										  //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;

					}


					Sampling_Struct sampling;
					bool isReflect;

					//vec3 sample_normal = rec.n;

					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	sample_normal = -rec.n;

					/*vec3 original_normal = rec.n;

					if (bsdfs[mtl]->have_refraction() && reverse_normal)
					original_normal = -rec.n;
					*/

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}
					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;

					Sampling_Struct sampling;
					bool isReflect;



					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	original_normal = -rec.n;


					//bool b;
					//if (!bsdfs[mtl]->isSampleVolume())
					//b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				//return L;

				return L;
			}
		}
		//return L;
		return L;
	}

	vec3 __fastcall sibenik_tracing_artificial_light_normal_Ke_alpha_roughness(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				//bool reverse_normal = false;
				float cos_incident = new_ray.d.dot(rec.n);

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				vec3 original_normal = rec.n;

				//vec3 original_n = rec.n;

				if (cos_incident > 0)
				{
					//reverse_normal = true;
					rec.n = -rec.n;
				}

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)// || mats[mtl].type == Light_Diffuse_type)
				{
					//cout << "Light here!\n";
					if (specular)
					{
						//Le 111
						L += T * mats[mtl].Ke;


						return L;
					}
					else
					{
						//if (mats[mtl].isLight)
						//{
						/*if (light_map.find(rec.ti) == light_map.end())
						{
						cout << "Light not map\n";
						cout << rec.ti << "\n";
						cout << mats[mtl].Matname << "\n";
						}*/
						int li = light_map[rec.ti];

						//cout << rec.ti << " " << li << "\n";
						//if (li != 0)
						//cout << rec.ti << " " << li << "\n";
						//	cout << li << "\n";
						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						L += T * mats[mtl].Ke * mis_weight;

						//L += T * Ls.Ke[li] * mis_weight;


						//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
						//return clamp_L;

						return L;

						//return vec3(1.0f, 0, 0);
						/*}
						else
						{
						L += T * mats[mtl].Ke;
						return L;
						}*/
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


					//float cos_light = -light_direction.dot(Ls.normal[li]);
					float cos_mtl = light_direction.dot(rec.n);

					/*if (mats[mtl].type == RoughPlastic_type)
					{
					vec3 h = (-new_ray.d + light_direction).norm();
					cos_mtl = h.dot(light_direction);
					}*/

					if (cos_mtl > 0.0f && !hit_anything_alpha(n, light_ray))
					{

						//float ilength = 1.0f / length;
						//light_direction *= ilength;

						//cos_light *= ilength;

						//cos_mtl *= ilength;

						//cos_light = abs(cos_light)

						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																									 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						//float bsdf_pdf;
						//vec3 bsdf_eval = bsdfs[mtl]->eval_and_pdf(new_ray.d, light_direction, rec.n, color, bsdf_pdf);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						//failed at this line for dabrovic
						//work for the rest
						//if (li != 0 && li != 1)
						//	cout << li << "\n";

						//vi sao T * mats[mtl].ke co ket qua
						//con T * Ls.ke[li] va T * 4 thi sai

						//cout << Ls.Ke[li].maxc() << "\n";

						//cout << mats[mtl].Ke.maxc() << "\n";

						//L += T * mats[mtl].Ke * bsdf_eval * mis / pdf_light;
						//cout << Ls.Ke.size() << "\n";
						L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;// *4.0f;
																		 //L = L * 4.0f;
					}


					Sampling_Struct sampling;
					bool isReflect;

					//vec3 sample_normal = rec.n;

					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	sample_normal = -rec.n;

					/*vec3 original_normal = rec.n;

					if (bsdfs[mtl]->have_refraction() && reverse_normal)
					original_normal = -rec.n;
					*/

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					/*if(!mats[mtl].use_Roughness)
						b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);
					else
					{
						float r = rec.r;
						//cout << r << "\n";
						b = bsdfs[mtl]->sample_roughness(new_ray.d, rec.n, original_normal, sampling, isReflect, color, r);
					}*/

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();
						//new_ray = Ray(new_hit_point, direction);

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}
					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					//if (bsdfs[mtl]->need_original_normal() && reverse_normal)
					//	rec.n = -rec.n;

					Sampling_Struct sampling;
					bool isReflect;



					//if (bsdfs[mtl]->have_refraction() && reverse_normal)
					//	original_normal = -rec.n;


					//bool b;
					//if (!bsdfs[mtl]->isSampleVolume())
					//b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					/*if (mats[mtl].type == Transparent_type)
					{
						new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.0002f, sampling.direction);
						continue;
					}*/

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				//return L;

				return L;
			}
		}
		//return L;
		return L;
	}


	vec3 __fastcall sibenik_tracing_enviroment(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				if (cos_incident > 0)
					rec.n = -rec.n;

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					float u, v;

					Env.DirectionToUV(new_ray.d, u, v);

					vec3 env_value = Env.UVToValue(u, v);

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;
						L += T * env_value;
						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						L += T * env_value * mis_weight;

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				//if (mtl < 0 || mtl >= mats.size() - 1)
				//	cout << mtl << "\n";

				if (!bsdfs[mtl]->isSpecular())
				{
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);

				
					float cos_mtl = light_direction.dot(rec.n);

					if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
						//if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					{
						//if (use_eviroment)
						//{

						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);


						/*
						float u, v;

						Env.DirectionToUV(new_ray.d, u, v);

						vec3 env_value = Env.UVToValue(u, v);
						float env_pdf = Env.UVToPdf(u, v);

						//too bright
						//float mis = mis2(prev_pdf, env_pdf);
						//L += T * env_value * mis;

						//noisy
						//Enviroment_1
						//float mis = mis2(pdf_light * env_pdf, bsdf_pdf * cont_prob);
						//L += T * env_value * bsdf_eval * mis / (pdf_light * env_pdf);
						*/


						float u, v;

						Env.DirectionToUV(new_ray.d, u, v);

						u += rotation_u;

						vec3 env_value = Env.UVToValue(u, v);

						//good
						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);
						L += T * env_value * bsdf_eval * mis / (pdf_light);


						//L += T * Le * bsdf_eval * mis / (pdf_light * env_pdf);
						//}
						/*else
						{
						float ilength = 1.0f / length;
						light_direction *= ilength;

						cos_light *= ilength;
						cos_mtl *= ilength;


						//better than use cos_mtl
						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

						//float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

						L += T * Le * bsdf_eval * mis / pdf_light;
						}*/
					}


					Sampling_Struct sampling;
					bool isReflect;

					bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();

						vec3 direction = sampling.direction;

						new_ray = Ray(new_hit_point, direction);

						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}
					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					Sampling_Struct sampling;
					bool isReflect;

					bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{
				//also correct
				if (use_enviroment)
				{

					float u, v;

					Env.DirectionToUV(new_ray.d, u, v);

					u += rotation_u;

					vec3 env_value = Env.UVToValue(u, v);

					//if (specular)
					//{
					//	L += T * env_value;
					//	return L;
					//}
					//else
					//{
					float env_pdf = Env.UVToPdf(u, v);

					float mis = mis2(prev_pdf, env_pdf);

					L += T * env_value * mis;

					return L;
					//}
				}
				else
				{
					L += T;
					return L;
				}

				//correct
				/*
				float u, v;

				Env.DirectionToUV(new_ray.d, u, v);

				u += rotation_u;

				vec3 env_value = Env.UVToValue(u, v);

				if (specular)
				{
				L += T * env_value;
				return L;
				}
				else
				{
				float env_pdf = Env.UVToPdf(u, v);

				float mis = mis2(prev_pdf, env_pdf);

				L += T * env_value * mis;

				return L;
				}
				*/
			}
		}
		return L;
	}

	vec3 __fastcall sibenik_tracing_blur_enviroment(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;


		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				//float cos_a = new_ray.d.dot(rec.n);

				//vec3 nl;
				//if (new_ray.d.dot(rec.n) >= 0)

				if (cos_incident > 0)
					rec.n = -rec.n;

				//vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					
					vec3 env_direction = new_ray.d;
					if (blur_enviroment)// && i != 0)
					{
						//float blur_radius = i > 0 ? enviroment_blur_radius : first_bounce_blur_radius;
						//vec3 outward_normal = -rec.n;

						//vec3 hit_point(new_ray.o + new_ray.d * rec.t);

						//vec3 blur_center = hit_point + outward_normal;

						float u = randf();
						float v = randf();

						float theta = tau * u;
						float phi = acosf(2.0f * v - 1.0f);

						float sin_phi, cos_phi;

						FTA::sincos(phi, &sin_phi, &cos_phi);

						float sin_theta, cos_theta;

						FTA::sincos(theta, &sin_theta, &cos_theta);

						float x = enviroment_blur_radius * sin_phi * cos_theta;
						float y = enviroment_blur_radius * sin_phi * sin_theta;
						float z = enviroment_blur_radius * cos_phi;

						//float x = blur_radius * sin_phi * cos_theta;
						///float y = blur_radius * sin_phi * sin_theta;
						//float z = blur_radius * cos_phi;

						//original

						//vec3 env_point = hit_point + outward_normal + vec3(x, y, z);
						//vec3 env_direction = (env_point - hit_point).norm();

						//optimize

						vec3 outward_normal = -rec.n;

						vec3 env_direction = (outward_normal + vec3(x, y, z)).norm();

						
					}


					float u, v;

					Env.DirectionToUV(env_direction, u, v);

					vec3 env_value = Env.UVToValue(u, v);

					if (specular)
					{
						//Le 111
						//L += T * mats[mtl].Ke;
						L += T * env_value;
						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						//Good
						L += T * env_value * mis_weight;

						return L;

					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				//if (mtl < 0 || mtl >= mats.size() - 1)
				//	cout << mtl << "\n";

				if (!bsdfs[mtl]->isSpecular())
				{
					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);


					float cos_mtl = light_direction.dot(rec.n);

					//if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
					if(cos_mtl > 0.0f && !hit_anything_alpha(n, light_ray))
					{
						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						vec3 env_direction = new_ray.d;

						if (blur_enviroment)
						{
							float u = randf();
							float v = randf();

							float theta = tau * u;
							float phi = acosf(2.0f * v - 1.0f);

							float sin_phi, cos_phi;

							FTA::sincos(phi, &sin_phi, &cos_phi);

							float sin_theta, cos_theta;

							FTA::sincos(theta, &sin_theta, &cos_theta);

							float x = enviroment_blur_radius * sin_phi * cos_theta;
							float y = enviroment_blur_radius * sin_phi * sin_theta;
							float z = enviroment_blur_radius * cos_phi;						

							//original

							//vec3 env_point = light_point + new_ray.d + vec3(x, y, z);
							//env_direction = (env_point - light_point).norm();

							//optimize
							env_direction = (new_ray.d + vec3(x, y, z)).norm();
						}
						

						float u, v;

						Env.DirectionToUV(env_direction, u, v);

						u += rotation_u;

						vec3 env_value = Env.UVToValue(u, v);

						
						float mis = mis2(pdf_light, bsdf_pdf * cont_prob);
						L += T * env_value * bsdf_eval * mis / pdf_light;

					}


					Sampling_Struct sampling;
					bool isReflect;

					bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					/*if (!sampling.isSpecular)
					{
						//onb local(rec.n);

						//vec3 direction = (local.u * sampling.direction.x + local.v * sampling.direction.y + local.w * sampling.direction.z).norm();

						vec3 direction = sampling.direction;

						new_ray = Ray(new_hit_point, direction);

						specular = false;
					}
					else
					{

						new_ray = Ray(new_hit_point, sampling.direction);
						specular = true;
					}*/

					new_ray = Ray(new_hit_point, sampling.direction);
					specular = sampling.isSpecular;

					T *= sampling.weight;
					//T *= sampling.weight;
				}
				else
				{
					Sampling_Struct sampling;
					bool isReflect;

					bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					//Quan Trọng
					//Importance
					//sampling.weight và sampling.eval là hòan tòan khácc nhau

					//vú dị: Diffuse

					//Sampling.weight = color
					//Sampling.eval = color * cos_o * ipi

					//sampling weight tính với tia sample cho bước tiếp theo
					//sampling.eval tính với tia sample light
					//nếu dùng công thức sampling.eval cho sampling.weight thì tia sẽ yếu
					//ảnh sẽ tối và russian roulette sẽ kết thúc sớm
					//dẫn đến ảnh tối hơn và kết thúc rất sớm

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{
				//also correct
				if (use_enviroment)
				{

					float u, v;

					Env.DirectionToUV(new_ray.d, u, v);

					u += rotation_u;

					vec3 env_value = Env.UVToValue(u, v);

					//if (specular)
					//{
					//	L += T * env_value;
					//	return L;
					//}
					//else
					//{
					float env_pdf = Env.UVToPdf(u, v);

					float mis = mis2(prev_pdf, env_pdf);

					L += T * env_value * mis;

					return L;
					//}
				}
				else
				{
					L += T;
					return L;
				}

				//correct
				/*
				float u, v;

				Env.DirectionToUV(new_ray.d, u, v);

				u += rotation_u;

				vec3 env_value = Env.UVToValue(u, v);

				if (specular)
				{
				L += T * env_value;
				return L;
				}
				else
				{
				float env_pdf = Env.UVToPdf(u, v);

				float mis = mis2(prev_pdf, env_pdf);

				L += T * env_value * mis;

				return L;
				}
				*/
			}
		}
		return L;
	}

	vec3 __fastcall sibenik_tracing_directional_lighting(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		//vec3 Ke = 

		for (int i = 0; i < 60; ++i)
		{
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}

				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;


				if (cos_incident > 0)
				{

					rec.n = -rec.n;
				}


				int mtl = trs[rec.ti].mtl;

				if ((mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type))
				{
					if (use_area_light)
					{
						cout << "use\n";
						vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

						if (specular)
						{

							L += T * Ke;// *0.1f;

							return L;
						}
						else
						{
							int li = light_map[rec.ti];

							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();


							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							float mis_weight = mis2(prev_pdf, pdf_light);

							//Good
							//L += T * mats[mtl].Ke * mis_weight;

							L += T * Ke * mis_weight;// *0.1f;


							return L;

						}
					}
					else
					{
						//return vec3(0.0f);
						L += T;// *0.1f;// *0.0000001f;
						return L;
					}
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				//float cont_prob = max(min(T.maxc(), 1.0f), 0.01f);

				float cont_prob = min(T.maxc(), 1.0f);

				//sample direct lighting				

				if (!bsdfs[mtl]->isSpecular())
				{
					if (use_directional_light)
					{
						vec3 sun_direction = inifinite_light_direction;
						if (blur_directional_light)
						{
							//from hit point move 1 unit in light direction
							//create a sphere of radius equal to blur radius
							//sample random point on sphere, minus these point with hitpoint
							//this will be the new sun_direction

							vec3 blur_center = hit_point + sun_direction;

							float u = randf();
							float v = randf();

							float theta = tau * u;
							float phi = acosf(2.0f * v - 1.0f);

							float sin_phi, cos_phi;

							FTA::sincos(phi, &sin_phi, &cos_phi);

							float sin_theta, cos_theta;

							FTA::sincos(theta, &sin_theta, &cos_theta);

							float x = directional_blur_radius * sin_phi * cos_theta;
							float y = directional_blur_radius * sin_phi * sin_theta;
							float z = directional_blur_radius * cos_phi;

							//original

							vec3 sample_point = blur_center + vec3(x, y, z);
							sun_direction = (sample_point - hit_point).norm();

							//wrong
							//sun_direction = vec3(x, y, z);
							//sun_direction.normalize();

						}
						//Ray directional_lighting(hit_point, inifinite_light_direction);
						Ray directional_lighting(hit_point, sun_direction);

						if (!hit_anything_range(n, directional_lighting))
						{
							vec3 directional_bsdf = bsdfs[mtl]->eval(new_ray.d, sun_direction, rec.n, original_normal, color);

							//MIS
							//float directional_pdf = bsdfs[mtl]->pdf(new_ray.d, inifinite_light_direction, rec.n, original_normal);
							//float sun_pdf = 1.0f;//there is only one way to sample this direction

							//float mis = mis2(sun_pdf, directional_pdf * cont_prob);


							//L += T * sun_color * directional_bsdf * mis;

							//No MIS
							L += T * sun_color * directional_bsdf;

						}
					}
					if (use_area_light)
					{
						int li = Ls.sample();
						vec3 light_point(Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length();

						float ilength = 1.0f / length;
						light_direction *= ilength;

						Ray light_ray(hit_point, light_direction);


						float cos_mtl = light_direction.dot(rec.n);


						if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
						{
							float cos_light = abs(-light_direction.dot(Ls.normal[li]));


							float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];//cos_mtl

																										 //float pdf_light = (length * length) / (Ls.area[li] * cos_mtl) * Ls.pdf[li];

							vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
							float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);


							float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

							//cout << li << "\n";

							//cout << Ls.Ke.size() << "\n";

							/*if (use_eviroment)
							{
								vec3 env_direction = new_ray.d;
								if (blur_enviroment)// && i != 0)
								{
									//vec3 outward_normal = -rec.n;

									//vec3 hit_point(new_ray.o + new_ray.d * rec.t);

									//vec3 blur_center = hit_point + outward_normal;

									float u = randf();
									float v = randf();

									float theta = tau * u;
									float phi = acosf(2.0f * v - 1.0f);

									float sin_phi, cos_phi;

									FTA::sincos(phi, &sin_phi, &cos_phi);

									float sin_theta, cos_theta;

									FTA::sincos(theta, &sin_theta, &cos_theta);

									float x = enviroment_blur_radius * sin_phi * cos_theta;
									float y = enviroment_blur_radius * sin_phi * sin_theta;
									float z = enviroment_blur_radius * cos_phi;

									//original

									//vec3 env_point = hit_point + outward_normal + vec3(x, y, z);
									//vec3 env_direction = (env_point - hit_point).norm();

									//optimize

									vec3 outward_normal = -rec.n;

									env_direction = (outward_normal + vec3(x, y, z)).norm();


								}*

								float u, v;

								Env.DirectionToUV(env_direction, u, v);

								u += rotation_u;

								vec3 env_value = Env.UVToValue(u, v);

								L += T * env_value;
								return L;
							}*/
							L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;// * 0.1f

						}
					}

					Sampling_Struct sampling;
					bool isReflect;


					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;

					new_ray = Ray(new_hit_point, sampling.direction);

					specular = sampling.isSpecular;


					T *= sampling.weight;
				}
				else
				{

					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						//Twist for scene having huge occlusion part
						//L += T * 0.2f;
						//return L;
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{

				//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				//vec3 clamp_L = clamp_vector(L, 0.0f, 1.0f);
				//return clamp_L;

				//return L;

				//if use enviroment compute this value

				//compute sky color will create better result

				//this is from Ray Tracing In One Weekend


				if (i == 0)
				{
					if (compute_sky_color)
					{
						vec3 sky_direction = new_ray.d;

						float t = 0.5f * (sky_direction.x + 1.0f);// original.y

						//vec3 sky_color = (1.0f - t) * vec3(1.0f) + t * vec3(0.5f, 0.7f, 4.0f);

						//vec3 sky_color = (1.0f - t) * vec3(1.0f) + t * vec3(40, 1, 1);//vec3(0.2f, 0.4f, 6.0f);

						//original
						//vec3 sky_color = (1.0f - t) * vec3(1.0f) + t * vec3(0.2f, 0.4f, 6.0f);

						vec3 sky_color = (1.0f - t) * vec3(40.0f) + t * vec3(40.2f, 40.4f, 42.2f);

						L += T * sky_color;

						return L;
					}
					else
					{
						//cout << "b";
						vec3 env_direction = new_ray.d;
						if (blur_enviroment)// && i != 0)
						{
							//vec3 outward_normal = -rec.n;

							//vec3 hit_point(new_ray.o + new_ray.d * rec.t);

							//vec3 blur_center = hit_point + outward_normal;

							float u = randf();
							float v = randf();

							float theta = tau * u;
							float phi = acosf(2.0f * v - 1.0f);

							float sin_phi, cos_phi;

							FTA::sincos(phi, &sin_phi, &cos_phi);

							float sin_theta, cos_theta;

							FTA::sincos(theta, &sin_theta, &cos_theta);

							float x = enviroment_blur_radius * sin_phi * cos_theta;
							float y = enviroment_blur_radius * sin_phi * sin_theta;
							float z = enviroment_blur_radius * cos_phi;

							//original

							//vec3 env_point = hit_point + outward_normal + vec3(x, y, z);
							//vec3 env_direction = (env_point - hit_point).norm();

							//optimize

							vec3 outward_normal = -rec.n;

							env_direction = (outward_normal + vec3(x, y, z)).norm();


						}

						float u, v;

						Env.DirectionToUV(env_direction, u, v);

						u += rotation_u;

						vec3 env_value = Env.UVToValue(u, v);

						L += T * env_value;
						return L;

						/*//if (specular)
						//{
						//	L += T * env_value;
						//	return L;
						//}
						//else
						//{
						float env_pdf = Env.UVToPdf(u, v);

						float mis = mis2(prev_pdf, env_pdf);

						L += T * env_value * mis;

						return L;*/
					}
				}
				else
				{
					L += T;// *0.1f;// *0.00001f;
					return L;
				}
				/*if (specular)
				{

				L += T * sky_color;

				return L;
				}
				else
				{
				int li = light_map[rec.ti];

				vec3 light_direction(new_ray.d * rec.t);

				float length2 = light_direction.length2();

				light_direction.normalize();


				float cos_light = -light_direction.dot(rec.n);

				float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

				float mis_weight = mis2(prev_pdf, pdf_light);

				//Good
				//L += T * mats[mtl].Ke * mis_weight;

				L += T * sky_color * mis_weight;

				return L;

				}*/

			}
		}
		//return L;
		return L;
	}

	vec3 __fastcall sibenik_tracing_point_light_only(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		vec3 Ke = delta_light_Ke;

		for (int i = 0; i < 60; ++i)
		{
			//if (hit_color_alpha_pointer(n, new_ray, rec, &color, alpha))
			float alpha = 1.0f;
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				if (randf() > alpha)
				{
					new_ray = Ray(new_ray.o + new_ray.d * rec.t * 1.000002f, new_ray.d);
					continue;
				}


				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;


				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					return vec3(0.0f);
				}

				vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);

				int type = mats[mtl].type;

				float cont_prob = min(T.maxc(), 1.0f);

				if (!bsdfs[mtl]->isSpecular())
				{

					vec3 light_point = delta_light_position;

					vec3 light_direction(light_point - hit_point);



					float length = light_direction.length();

					float ilength = 1.0f / length;
					light_direction *= ilength;

					Ray light_ray(hit_point, light_direction);

					float cos_mtl = light_direction.dot(rec.n);

					//length *= 1.11f;

					if (cos_mtl > 0.0f && !hit_anything_range_alpha(n, light_ray, length))
					{
						
						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);


						
						float length2 = length * length;
						L += T * Ke * bsdf_eval / length2;

					}


					Sampling_Struct sampling;
					bool isReflect;


					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;

					prev_pdf = sampling.pdf;

					new_ray = Ray(new_hit_point, sampling.direction);

					specular = sampling.isSpecular;

					T *= sampling.weight;
				}
				else
				{

					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					
					rec.n = isReflect ? rec.n : -rec.n;

					new_ray = Ray(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f, sampling.direction);
					prev_pdf = sampling.pdf;
					specular = true;


					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						//L += T;
						//return L;
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;

				}
			}
			else
			{
				//L += T;// *vec3(4.575004577636719, 3.5907576084136963, 1.5497708320617676);

				return L;
			}
		}
		//cheat because back face cause many dark place
		//L += T;
		return L;
	}


	vec3 Volumetric_Rendering(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);

		float w_mats = 1.0f;

		Ray new_ray = r;

		HitRecord rec;

		MediumInteraction mi;


		for (int i = 0; i < 60; ++i)
		{
			float t_max;

			vec3 color;
			float alpha = 1.0f;

			bool intersect = hit_color_alpha(n, new_ray, rec, &color, alpha);

			if (intersect)
				t_max = rec.t;
			else
				t_max = 200.0f;//repalce with distance hit between ray and bounding volume

			mi.tMax = t_max;
			
			vec3 medium_color = medium.Heterogeneous_Sample(new_ray, mi);

			if (mi.isValid)
			{
				vec3 direction_out;
				float pdf_medium = HenyeyGreenStein(new_ray.d, direction_out);

				int li = Ls.sample();
				vec3 light_point(Ls.sample_light(li));

				vec3 medium_hit_point = mi.p;

				vec3 light_direction(light_point - medium_hit_point);

				float length = light_direction.length();

				float inv_length = 1.0f / length;

				light_direction *= inv_length;

				Ray medium_shadow_ray(medium_hit_point, light_direction);

				if (!hit_anything_range_alpha(n, medium_shadow_ray, length))
				{
					T *= medium_color;
					mi.tMax = length;
					L += T * medium.Heterogeneous_Transimission(medium_shadow_ray, mi) * Ls.Ke[li] * pdf_medium;
				}
				else
					T *= medium_color;

				float prob = minf(T.maxc(), 0.999f);

				if (randf() >= prob)
					return L;
				
				float inv_prob = 1.0f / prob;

				T *= inv_prob;

				direction_out = direction_out.norm();

				new_ray = Ray(medium_hit_point, direction_out);

				vec3 new_color;
				float new_alpha;

				if (hit_color_alpha(n, new_ray, rec, &new_color, new_alpha))
				{
					int mtl = trs[rec.ti].mtl;

					if (mats[mtl].isLight)
					{
						int li = light_map[rec.ti];
						//vec3 light_point(Ls.sample_light(li));

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();

						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];
						
						w_mats = pdf_medium + pdf_light > 0.0f ? pdf_medium / (pdf_medium + pdf_light) : pdf_medium;

					}
				}
			}
			else if (!intersect)
			{
				return L;
			}
			else
			{
				int mtl = trs[rec.ti].mtl;			

				if (mats[mtl].isLight)
				{
					int li = light_map[rec.ti];
					
					L += T * w_mats * mats[mtl].Ke * medium.Heterogeneous_Transimission(new_ray, mi);
				}

				vec3 hit_point(new_ray.o + rec.t * new_ray.d + rec.n * 0.0002f);

				//sample light
				int li = light_map[rec.ti];
				
				vec3 light_point(Ls.sample_light(li));

				vec3 light_direction(light_point - hit_point);

				float length2 = light_direction.length2();

				light_direction.normalize();

				Ray shadow_ray(hit_point, light_direction);

				float length = sqrt14(length2);

				if (!hit_anything_range_alpha(n, shadow_ray, length))
				{
					float cos_light = -light_direction.dot(rec.n);

					float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

					//vec3 bsdf = bsdfs[mtl]->eval()

					vec3 original_normal = rec.n;

					//Sampling_Struct sampling;
					//bool isReflect;

					//bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);
					//bsdfs[mtl]->pdf(new_ray.d, )

					vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
					float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

					float w_ems = pdf_light + bsdf_pdf > 0.0f ? pdf_light / (pdf_light + bsdf_pdf) : pdf_light;

					mi.tMax = length;

					L += T * bsdf_eval * w_ems * cos_light * medium.Heterogeneous_Transimission(shadow_ray, mi);
				}

				float prob = minf(T.x, 0.99f);

				if (randf() > prob)
					return L;

				float inv_prob = 1.0f / prob;

				T *= inv_prob;

				vec3 original_normal = rec.n;

				Sampling_Struct sampling;

				bool isReflect = false;

				bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

				float pdf_mat = sampling.pdf;

				T *= sampling.weight;

				new_ray = Ray(hit_point, sampling.direction);

				bool intersect = hit_anything_alpha(n, new_ray);

				if (intersect)
				{
					int mtl = trs[rec.ti].mtl;

					if (mats[mtl].isLight)
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();


						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float w_mats = pdf_mat + pdf_light > 0.0f ? pdf_mat / (pdf_mat + pdf_light) : pdf_mat;

						L += T * w_mats * mats[mtl].Ke;
						break;
					}
				}
			}
		}

		return L;
	}

	vec3 Volumetric_Rendering_2(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);

		float w_mats = 1.0f;

		Ray new_ray = r;

		HitRecord rec;

		MediumInteraction mi;


		for (int i = 0; i < 60; ++i)
		{
			float t_max;

			vec3 color;
			float alpha = 1.0f;

			/*bool intersect = hit_color_alpha(n, new_ray, rec, &color, alpha);

			if (intersect)
				t_max = rec.t;
			else
				medium.bound.hit_shadow_th(new_ray, t_max);
				//t_max = 200.0f;//repalce with distance hit between ray and bounding volume
			*/

			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight)
				{
					L += T * mats[mtl].Ke;
					return L;
				}

				mi.tMax = rec.t;

				vec3 medium_color = medium.Heterogeneous_Sample(new_ray, mi);

				if (mi.isValid)
				{
					vec3 direction_out;
					float pdf_medium = HenyeyGreenStein(new_ray.d, direction_out);

					int li = Ls.sample();
					vec3 light_point(Ls.sample_light(li));

					vec3 medium_hit_point = mi.p;

					vec3 light_direction(light_point - medium_hit_point);

					float length = light_direction.length();

					float inv_length = 1.0f / length;

					light_direction *= inv_length;

					Ray medium_shadow_ray(medium_hit_point, light_direction);

					if (!hit_anything_range_alpha(n, medium_shadow_ray, length))
					{
						T *= medium_color;
						mi.tMax = length;
						L += T * medium.Heterogeneous_Transimission(medium_shadow_ray, mi) * Ls.Ke[li] * pdf_medium;
					}
					else
						T *= medium_color;

					float prob = minf(T.maxc(), 0.999f);

					if (randf() >= prob)
						return L;

					float inv_prob = 1.0f / prob;

					T *= inv_prob;

					direction_out = direction_out.norm();

					new_ray = Ray(medium_hit_point, direction_out);

					/*vec3 new_color;
					float new_alpha;

					if (hit_color_alpha(n, new_ray, rec, &new_color, new_alpha))
					{
						int mtl = trs[rec.ti].mtl;

						if (mats[mtl].isLight)
						{
							int li = light_map[rec.ti];
							//vec3 light_point(Ls.sample_light(li));

							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();

							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							w_mats = pdf_medium + pdf_light > 0.0f ? pdf_medium / (pdf_medium + pdf_light) : pdf_medium;

						}
					}*/


				}
				else
				{
					int mtl = trs[rec.ti].mtl;

					if (mats[mtl].isLight)
					{
						int li = light_map[rec.ti];

						L += T * w_mats * mats[mtl].Ke * medium.Heterogeneous_Transimission(new_ray, mi);

						return L;
					}

					vec3 hit_point(new_ray.o + rec.t * new_ray.d + rec.n * 0.0002f);

					//sample light
					int li = light_map[rec.ti];

					vec3 light_point(Ls.sample_light(li));

					vec3 light_direction(light_point - hit_point);

					float length2 = light_direction.length2();

					light_direction.normalize();

					Ray shadow_ray(hit_point, light_direction);

					float length = sqrt14(length2);

					if (!hit_anything_range_alpha(n, shadow_ray, length))
					{
						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//vec3 bsdf = bsdfs[mtl]->eval()

						vec3 original_normal = rec.n;

						//Sampling_Struct sampling;
						//bool isReflect;

						//bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);
						//bsdfs[mtl]->pdf(new_ray.d, )

						vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						float w_ems = pdf_light + bsdf_pdf > 0.0f ? pdf_light / (pdf_light + bsdf_pdf) : pdf_light;

						mi.tMax = length;

						L += T * bsdf_eval * w_ems * cos_light * medium.Heterogeneous_Transimission(shadow_ray, mi);
					}

					float prob = minf(T.x, 0.99f);

					if (randf() > prob)
						return L;

					float inv_prob = 1.0f / prob;

					T *= inv_prob;

					vec3 original_normal = rec.n;

					Sampling_Struct sampling;

					bool isReflect = false;

					bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					float pdf_mat = sampling.pdf;

					T *= sampling.weight;

					new_ray = Ray(hit_point, sampling.direction);

					/*bool intersect = hit_anything_alpha(n, new_ray);

					if (intersect)
					{
						int mtl = trs[rec.ti].mtl;

						if (mats[mtl].isLight)
						{
							int li = light_map[rec.ti];

							vec3 light_direction(new_ray.d * rec.t);

							float length2 = light_direction.length2();

							light_direction.normalize();


							float cos_light = -light_direction.dot(rec.n);

							float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

							float w_mats = pdf_mat + pdf_light > 0.0f ? pdf_mat / (pdf_mat + pdf_light) : pdf_mat;

							L += T * w_mats * mats[mtl].Ke;
							break;
						}
					}*/
				}
			}
			else
				return L;
			

			

			
			
			
		}

		return L;
	}










	vec3 SphericalCoordinate(float& sin_theta, float& cos_theta, float& phi, vec3& x, vec3& y, vec3& z)
	{
		float sin_phi, cos_phi;

		FTA::sincos(phi, &sin_phi, &cos_phi);

		return sin_theta * cos_phi * x + sin_theta * sin_phi * y + cos_theta * z;
	}


	bool isScatter(float& intersect_distance)
	{
		float sample_distance = -log(randf()) / sigma_t;

		if (sample_distance < intersect_distance)
		{
			//cout << "Yes\n";
			intersect_distance = sample_distance;
			return true;
		}
		//cout << "No\n";
		return false;
	}

	//float phase_HG(float& cos_theta)
	//float phase_HG(float& cos_theta)
	float HG_pdf(vec3& wi, vec3& wo)
	//float HG_pdf(float& cos_theta)
	{
		//float denom = 1.0 + g * (g + 2.0 * cos_theta);

		float denom = 1.0 + g * (g + 2.0 * wi.dot(wo));

		return Inv4Pi * (1.0 - g * g) / (denom * sqrt14(denom));
	}
	
	//HenyeyGreenStein Compute the sample angle cos_theta
	//Phase_HG compute the probability cos_theta will be sample
	//similar to bsdf and pdf
	float HG_Sample(vec3& wi, vec3& wo)
	{
		float cos_theta;

		float r1 = randf();

		if (abs(g) < 1e-3)
			cos_theta = 1.0 - 2.0 * r1;
		else
		{
			float sqrt_term = (1.0 - g * g) / (1.0 - g + 2.0 * g * r1);

			cos_theta = (1.0 + g * g - sqrt_term * sqrt_term) / (2.0 * g);
		}

		float sin_theta = sqrt14(1.0 - cos_theta * cos_theta);

		float phi = 2.0 * pi * randf();

		onb onb(wi);

		wo = SphericalCoordinate(sin_theta, cos_theta, phi, onb.u, onb.v, onb.w);

		//return phase_HG(cos_theta);
		//return phase_HG(-wi, wo);

		return HG_pdf(-wi, wo);
		//return HG_pdf(cos_theta);
	}

	vec3 VolumeTric_Rendering_Homogeneous(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);

		float prev_pdf = 1.0f;

		Ray new_ray(r);

		HitRecord rec;
		vec3 color;
		float alpha = 1.0f;

		bool specular = true;

		for (int i = 0; i < 60; ++i)
		{
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;

				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

					//expf(-sigma_t * rec.t) is beer law for homogeneous medium
					//upgrade this one to heterogenous medium

					T *= expf(-sigma_t * rec.t);

					if (specular)
					{
						L += T * Ke;

						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();

						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						L += T * Ke * mis_weight;

						return L;
					}
				}

				float distance = rec.t;

				float cont_prob = 1.0f;

				if (isScatter(distance))
				{
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					vec3 Transmitance(-expf(sigma_t * distance));

					vec3 density = sigma_t * Transmitance;

					float atmos_pdf = density.x;//(x + y + z) / 3

					T *= Transmitance * sigma_s / atmos_pdf;

					//sample light
					int li = Ls.sample();

					vec3 light_point = Ls.sample_light(li);

					vec3 light_direction(light_point - hit_point);

					float length2 = light_direction.length2();

					float length = sqrt14(length2);

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray shadow_ray(hit_point, light_direction);

					//float cont_prob = min(T.maxc(), 1.0f);

					cont_prob = min(T.maxc(), 1.0f);

					if (!hit_anything_range(n, shadow_ray))
					{
						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						//float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						float scatter_pdf = HG_pdf(-new_ray.d, light_direction);

						float mis = mis2(pdf_light, scatter_pdf);

						float attenuation = expf(-sigma_t * length);

						L += T * attenuation * Ls.Ke[li] * mis / pdf_light;
					}

					vec3 direction_out;
					float p = HG_Sample(new_ray.d, direction_out);

					new_ray = Ray(hit_point, direction_out);

					prev_pdf = p;

					specular = false;
				}
				else
				{
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					vec3 Transmitance(-expf(sigma_t * distance));

					float atmos_pdf = Transmitance.x;

					T *= Transmitance / atmos_pdf;

					
					cont_prob = min(T.maxc(), 1.0f);

					if (!bsdfs[mtl]->isSpecular())
					{

						int li = Ls.sample();
						vec3 light_point(Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length();

						float ilength = 1.0f / length;
						light_direction *= ilength;

						Ray light_ray(hit_point, light_direction);


						float cos_mtl = light_direction.dot(rec.n);

						if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
						{

							float cos_light = abs(-light_direction.dot(Ls.normal[li]));

							float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

							vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
							float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);


							float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

							T *= expf(-sigma_t * length);

							L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;
						}
					}

					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;


					new_ray = Ray(new_hit_point, sampling.direction);


					specular = sampling.isSpecular;

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;
				}
			}
			else
				return L;
		}

		return L;
	}

	vec3 VolumeTric_Rendering_Homogeneous_2(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);

		float prev_pdf = 1.0f;

		Ray new_ray(r);

		HitRecord rec;
		vec3 color;
		float alpha = 1.0f;

		bool specular = true;

		for (int i = 0; i < 60; ++i)
		{
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;

				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

					//expf(-sigma_t * rec.t) is beer law for homogeneous medium
					//upgrade this one to heterogenous medium

					//T *= expf(-sigma_t * rec.t);

					if (specular)
					{
						L += T * Ke;

						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();

						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						L += T * Ke * mis_weight;

						return L;
					}
				}

				float distance = rec.t;

				float cont_prob = 1.0f;

				if (isScatter(distance))
				{
					//cout << "Yes\n";
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					vec3 Transmitance(expf(-sigma_t * distance));

					vec3 density = sigma_t * Transmitance;

					float atmos_pdf = density.x;//(x + y + z) / 3

					T *= Transmitance;// *sigma_h * sigma_s / atmos_pdf;

					/*//sample light
					int li = Ls.sample();

					vec3 light_point = Ls.sample_light(li);

					vec3 light_direction(light_point - hit_point);

					float length2 = light_direction.length2();

					float length = sqrt14(length2);

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray shadow_ray(hit_point, light_direction);

					//float cont_prob = min(T.maxc(), 1.0f);

					cont_prob = min(T.maxc(), 1.0f);

					if (!hit_anything_range(n, shadow_ray))
					{
						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						//float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						float scatter_pdf = HG_pdf(-new_ray.d, light_direction);

						float mis = mis2(pdf_light, scatter_pdf * cont_prob);

						//float attenuation = expf(-sigma_t * length);

						//L += T * attenuation * Ls.Ke[li] * mis / pdf_light;

						L += T * Ls.Ke[li] * mis / pdf_light;
					}*/

					vec3 direction_out;
					float p = HG_Sample(-new_ray.d, direction_out);

					new_ray = Ray(hit_point, direction_out);

					//prev_pdf = p;

					specular = false;
					continue;
				}
				else
				{
					//cout << "No\n";
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					//vec3 Transmitance(-expf(sigma_t * distance));

					//float atmos_pdf = Transmitance.x;

					//T *= Transmitance / atmos_pdf;

					//float inv_atmos = 1.0f / atmos_pdf;

					//T *= inv_atmos;

					cont_prob = min(T.maxc(), 1.0f);

					if (!bsdfs[mtl]->isSpecular())
					{

						int li = Ls.sample();
						vec3 light_point(Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length();

						float ilength = 1.0f / length;
						light_direction *= ilength;

						Ray light_ray(hit_point, light_direction);


						float cos_mtl = light_direction.dot(rec.n);

						if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
						{

							float cos_light = abs(-light_direction.dot(Ls.normal[li]));

							float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

							vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
							float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);


							float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

							//T *= expf(-sigma_t * length);

							//L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;

							L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;
						}
					}

					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;


					new_ray = Ray(new_hit_point, sampling.direction);


					specular = sampling.isSpecular;

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;
				}
			}
			else
				return L;
		}

		return L;
	}

	vec3 VolumeTric_Rendering_Homogeneous_All_Transmitance(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);

		float prev_pdf = 1.0f;

		Ray new_ray(r);

		HitRecord rec;
		vec3 color;
		float alpha = 1.0f;

		bool specular = true;

		for (int i = 0; i < 60; ++i)
		{
			if (hit_color_alpha(n, new_ray, rec, &color, alpha))
			{
				float cos_incident = new_ray.d.dot(rec.n);

				vec3 original_normal = rec.n;

				if (cos_incident > 0)
					rec.n = -rec.n;

				int mtl = trs[rec.ti].mtl;

				if (mats[mtl].isLight || mats[mtl].type == Light_Diffuse_type)
				{
					vec3 Ke = mats[mtl].isLight ? mats[mtl].Ke : mats[mtl].Kd;

					//expf(-sigma_t * rec.t) is beer law for homogeneous medium
					//upgrade this one to heterogenous medium

					T *= expf(-sigma_t * rec.t);

					if (specular)
					{
						L += T * Ke;

						return L;
					}
					else
					{
						int li = light_map[rec.ti];

						vec3 light_direction(new_ray.d * rec.t);

						float length2 = light_direction.length2();

						light_direction.normalize();

						float cos_light = -light_direction.dot(rec.n);

						float pdf_light = length2 / (Ls.area[li] * cos_light) * Ls.pdf[li];

						float mis_weight = mis2(prev_pdf, pdf_light);

						L += T * Ke * mis_weight;

						return L;
					}
				}

				float distance = rec.t;

				float cont_prob = 1.0f;

				if (isScatter(distance))
				{
					//cout << "Yes\n";
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					vec3 Transmitance(expf(-sigma_t * distance));

					vec3 density = sigma_t * Transmitance;

					float atmos_pdf = density.x;//(x + y + z) / 3

					T *= Transmitance * sigma_s / atmos_pdf;

					//sample light
					int li = Ls.sample();

					vec3 light_point = Ls.sample_light(li);

					vec3 light_direction(light_point - hit_point);

					float length2 = light_direction.length2();

					float length = sqrt14(length2);

					float ilength = 1.0f / length;

					light_direction *= ilength;

					Ray shadow_ray(hit_point, light_direction);

					//float cont_prob = min(T.maxc(), 1.0f);

					cont_prob = min(T.maxc(), 1.0f);

					if (!hit_anything_range(n, shadow_ray))
					{
						float cos_light = abs(-light_direction.dot(Ls.normal[li]));

						float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

						//vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
						//float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);

						float scatter_pdf = HG_pdf(-new_ray.d, light_direction);

						float mis = mis2(pdf_light, scatter_pdf * cont_prob);

						float attenuation = expf(-sigma_t * length);

						L += T * attenuation * Ls.Ke[li] * mis / pdf_light;

						//L += T * Ls.Ke[li] * mis / pdf_light;
					}

					vec3 direction_out;
					float p = HG_Sample(-new_ray.d, direction_out);

					new_ray = Ray(hit_point, direction_out);

					prev_pdf = p;

					specular = false;
				}
				else
				{
					//cout << "No\n";
					vec3 hit_point(new_ray.o + new_ray.d * distance);

					vec3 Transmitance(expf(-sigma_t * distance));

					float atmos_pdf = Transmitance.x;

					T *= Transmitance / atmos_pdf;

					//float inv_atmos = 1.0f / atmos_pdf;

					//T *= inv_atmos;

					cont_prob = min(T.maxc(), 1.0f);

					if (!bsdfs[mtl]->isSpecular())
					{

						int li = Ls.sample();
						vec3 light_point(Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length();

						float ilength = 1.0f / length;
						light_direction *= ilength;

						Ray light_ray(hit_point, light_direction);


						float cos_mtl = light_direction.dot(rec.n);

						if (cos_mtl > 0.0f && !hit_anything_range(n, light_ray))
						{

							float cos_light = abs(-light_direction.dot(Ls.normal[li]));

							float pdf_light = (length * length) / (Ls.area[li] * cos_light) * Ls.pdf[li];

							vec3 bsdf_eval = bsdfs[mtl]->eval(new_ray.d, light_direction, rec.n, original_normal, color);
							float bsdf_pdf = bsdfs[mtl]->pdf(new_ray.d, light_direction, rec.n, original_normal);


							float mis = mis2(pdf_light, bsdf_pdf * cont_prob);

							T *= expf(-sigma_t * length);

							//L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;

							L += T * Ls.Ke[li] * bsdf_eval * mis / pdf_light;
						}
					}

					Sampling_Struct sampling;
					bool isReflect;

					bool b = bsdfs[mtl]->sample(new_ray.d, rec.n, original_normal, sampling, isReflect, color);

					if (!b)
						return vec3(0.0f);

					rec.n = isReflect ? rec.n : -rec.n;

					vec3 new_hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;


					prev_pdf = sampling.pdf;


					new_ray = Ray(new_hit_point, sampling.direction);


					specular = sampling.isSpecular;

					T *= sampling.weight;
				}
				if (i > 2)
				{
					prev_pdf *= cont_prob;
					if (cont_prob < randf())
					{
						break;
					}
					float inv_cont_prob = 1.0f / cont_prob;
					T *= inv_cont_prob;
				}
			}
			else
				return L;
		}

		return L;
	}

	vec3 Uniform_Hemisphere_Sampling()
	{
		float cos_theta = randf();

		float sin_theta = sqrt14(1.0f - cos_theta * cos_theta);

		float phi = 2.0f * pi * randf();

		float x = cosf(phi) * sin_theta;

		float y = sinf(phi) * sin_theta;

		float z = cos_theta;

		return vec3(x, y, z);
	}

	
	bool __fastcall hit_anything_range_test(node*& n, Ray& r, const float& d, const int& first)
	{
		node* stack[26];



		stack[0] = n->nodes[0];
		stack[1] = n->nodes[1];

		int si = 1;

		while (si >= 0)
		{
			auto top(stack[si--]);

			float th;

			if (top->box.hit_shadow_th(r, th) && th > 0.0f)
			{
				if (!top->leaf)
				{
					stack[si + 1] = top->nodes[1 - first];
					stack[si + 2] = top->nodes[first];

					si += 2;
				}
				else
				{

					auto start = top->start, end = start + top->range;

					for (auto i = start; i < end; ++i)
					{
						int mtl = trs[i].mtl;
						//if (trs[i].hit_anything(r, d) && !mats[mtl].isLight)
						if (trs[i].hit_anything(r) && !mats[mtl].isLight)
							return true;
					}
				}
			}
			else
				continue;
		}
		return false;
	}

	vec3 __fastcall ray_casting_color(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		//float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		float alpha;

		if (hit_color_alpha(n, new_ray, rec, &color, alpha))
		{
			//return rec.n;
			//vec3 v = 0.5f * rec.n + vec3(0.5f);
			//return v;
			//int mtl = trs[rec.ti].mtl;
			//if (mats[mtl].Matname == "metal_gold")
			
			int mtl = trs[rec.ti].mtl;

			if (!mats[mtl].isLight)
				return color;
			else
				return vec3(1.0f);//mats[mtl].Ke;
			//return vec3(0.0f);

		}
		return vec3(1.0f);
	}

	vec3 __fastcall ray_casting_normal(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		//float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		float alpha;

		if (hit_color_alpha(n, new_ray, rec, &color, alpha))
		{
			return rec.n;
			//return color;
			//return 0.5f * (rec.n + vec3(1.0f));
		}
		return vec3(0.0f);
	}

	void __fastcall ray_casting_color_normal(node*& n, Ray& r, vec3& col, vec3& norm)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		//float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		float alpha = 1.0f;

		if (hit_color_alpha(n, new_ray, rec, &color, alpha))
		{
			col = color;
			norm = rec.n;
		}
		else
		{
			col = vec3(1.0f);
			norm = vec3(0.0f);
		}
	}


	vec3 __fastcall ray_casting_color_denoise(node*& n, Ray& r)
	{
		vec3 L(0.0f);
		vec3 T(1.0f);
		HitRecord rec;
		vec3 color;

		//float prev_pdf = 1.0f;

		Ray new_ray(r);
		bool specular = true;

		float alpha;

		if(hit_color_alpha(n, new_ray, rec, &color, alpha))
		{
			//return rec.n;
			//vec3 v = 0.5f * rec.n + vec3(0.5f);
			//return v;
			//int mtl = trs[rec.ti].mtl;
			//if (mats[mtl].Matname == "metal_gold")

			int mtl = trs[rec.ti].mtl;

			if (!mats[mtl].isLight)
			{

				if (mats[mtl].type == Mirror_type)
				{
					vec3 direction_out = Reflect(new_ray.d, rec.n);
					vec3 hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;
					new_ray = Ray(hit_point, direction_out);
					//continue;
				}
				else if (mats[mtl].type == Glass_type)
				{
					//return vec3(1.0f);
					float cos_i = new_ray.d.dot(rec.n);

					float eta = cos_i >= 0.0f ? mats[mtl].ior : 1.0f / mats[mtl].ior;
					
	
					float cos_t;
					float abs_cos_i = abs(cos_i);

					float F = Dielectric_Reflectance(eta, abs_cos_i, cos_t);
					
					if (randf() < F)
					{
						vec3 direction_out = Reflect(new_ray.d, rec.n);
						vec3 hit_point = new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f;
						new_ray = Ray(hit_point, direction_out);
					}
					else
					{
						vec3 direction_out = (eta * new_ray.d + (eta * abs_cos_i - cos_t) * rec.n).norm();
						vec3 hit_point = new_ray.o + new_ray.d * rec.t - rec.n * 0.0002f;
						new_ray = Ray(hit_point, direction_out);
					}
					//continue;
				}
				else if (mats[mtl].alpha_material)
				{
					float alpha = mats[mtl].opaque;
					int ind = rec.ti;
					if (mats[mtl].use_alpha_texture)
					{
						int t0 = trs[ind].vt0,
							t1 = trs[ind].vt1,
							t2 = trs[ind].vt2;

						vec2 t(intp(vt[t0], vt[t1], vt[t2], rec.u, rec.v));

						int tex_ind = mats[mtl].TexInd;
						alpha = texs[tex_ind].ev_alpha(t);
					}

					if (randf() > alpha)
					{
						vec3 hit_point = new_ray.o + new_ray.d * (rec.t + 1.0002f);
						new_ray = Ray(hit_point, new_ray.d);
						//continue;
					}
					else
						return color;
				}
				else
				{
					return color;
				}
			}
			else
				return mats[mtl].Ke;
			//return vec3(0.0f);

		}
		
		return vec3(1.0f);
	}
};



#endif // !_VEC3_H_
