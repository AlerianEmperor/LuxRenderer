//#include "Scene.h"
//#include "math.h"
//#include "Scene_Prestigious_Path_Tracing.h"
//#include "Smooth_Normal_Scene.h"
#include "Scene.h"
//#include "Scene_Fast.h"
#include "Tone_Mapping.h"
#include "Filter.h"
#include <time.h>
#include <omp.h>

//http://www.raytracerchallenge.com/bonus/texture-mapping.html

using namespace std;
#define Width 400
#define Height 400

#define iWidth 1 / Width
#define iHeight 1 / Height
#define aspect_ratio Width / Height
#define golden_ratio 1.61803398875


const int ns = 16;
const int step = 16;
const int sqrt_ns = sqrt14(ns);
const float isqrt_ns = 1 / sqrt_ns;

const int num_shadow = 400;
#define ins  1 / ns
//NGUYEN NHAN CRASH CHUONG TRINH, loi heap crash
//do trong ham build bvh, ta swap cac tam giac ti[i] nam sat lai nhau
//dieu nay lam thay doi thu tu trong trs, nhung thu tu tam giac trong light map van khong doi
//va do do khi light map, voi index cu map den gia tri moi va xui xeo o day la spot holder 
//do do chuong trinh bi crash vi khi light map refer den mot gia tri khong ton tai se ra ket qua am
//tuy nhien trong chuong trinh li co the de ra gia tri 0 nen do do chuong trinh van chay binh thuong
//sua lai bang cach ta chi add light sau khi da build xong bvh

//bath room

//pots llok

//vec3 look_from(-1.352383, 0.923236, -1.987830);

//vec3 view_direction(-0.304377, -0.452886, -0.838003);

//original llok
//vec3 look_from(0.0072405338287353516, 0.9124049544334412, -0.2275838851928711);

//benedik look
//vec3 look_from(0.0072405338287353516, 0.9424049544334412, -0.2275838851928711);
//vec3 look_at(-2.787562608718872, 0.9699121117591858, -2.6775901317596436);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 55.0f;

//Test Conductor
//Test Smooth Coat
//Test Blinn
//vec3 look_from(-2.097548, 1.794362, -0.838512), view_direction(-0.071861, -0.183947, 0.980360);


//TEST ROUGH PLASTIC
//vec3 look_from(-1.642596, 1.023239, -1.160666); vec3 view_direction(-0.375579, -0.290740, -0.879998);

//vec3 look_from(-0.18993, 1.012759, -0.855806); const vec3 view_direction = vec3(-0.907783, -0.019999, -0.418963).norm();

//const vec3 look_from(-1.654151f, 2.002430f, -1.132994f); const vec3 view_direction(-0.496300f, -0.305059f, 0.812789f);

//const vec3 look_from(-0.642925, 0.581257, -1.244025); const vec3 view_direction(-0.591846, -0.501213, 0.631271);

//const vec3 look_from(0.008461, 1.045267, -0.645139);
//const vec3 view_direction = vec3(-0.825316, 0.059964, -0.561477).norm();

//const vec3 look_from(0.127673, 1.100172, -0.726294);
//const vec3 view_direction = vec3(-0.927660, 0.064954, -0.367732).norm();

//const vec3 look_from(0.307766, 1.144862, -0.763209);
//const vec3 view_direction = vec3(-0.943369, -0.023173, -0.330963).norm();

//fireplace room
//vec3 look_from(5.101118f, 1.083746f, -2.756308f); 
//const vec3 view_direction = vec3(-0.93355, -0.004821, 0.358416).norm();
//float fov = 46.0f;//86.001194f;

//test mirror
//vec3 look_from(2.896949, 1.304485, -2.128901); vec3 view_direction(-0.550573, 0.019999, 0.834548);

//vec3 look_from(2.760269, 1.764775, -0.826542); vec3 view_direction(-0.758540, -0.109779, 0.642313);

//BathRoom
//look_at = loom_at haha
//vec3 look_from(5.105184555053711f, 0.7310651540756226f, -2.3178906440734863f); 
//vec3 look_at(1.452592134475708f, 1.0136401653289795f, -1.3172874450683594f); 
//vec3 view_direction = (look_at - look_from).norm();
//const float fov = 55.0f;

//vec3 look_from(5.105184555053711f, 1.0310651540756226f, -2.3178906440734863f); vec3 look_at(1.452592134475708f, 1.0136401653289795f, -1.3172874450683594f); vec3 view_direction = (look_at - look_from).norm();


//vec3 look_from(3.577393, 0.706135, -0.603293); vec3 view_direction(-0.651559, - 0.333487, 0.681269);

//sibenik
//const vec3 look_from(-16.567905, -12.021462, -0.521549);
//const vec3 view_direction = vec3(0.984883, 0.164252, 0.055007).norm();

//original view
//vec3 look_from(-12.350879, -12.584566, 2.539952);
//const vec3 view_direction(0.980570, 0.187294, -0.058346);

//new view 1
//const vec3 look_from(-9.078943, -12.577990, 2.540149);
//const vec3 view_direction(0.971532, 0.236155, -0.018886);

//new view 2
//vec3 look_from(-11.236954, -11.983788, 2.243523);
//vec3 view_direction(0.955283, 0.206913, -0.211236);

//const float fov = 60.0f;


//BedRoom
//const vec3 look_from(-0.18078, -3.332532, 0.975756);
//const vec3 view_direction = vec3(-0.000336, 0.995084, 0.099038).norm();

//Living Room
//vec3 look_from(2.511603, 2.086595, 6.875006);
//vec3 view_direction = vec3(-0.426998, -0.223106, -0.876297).norm();
//float fov = 90.0f;

//bath room
//float fov = 55.0f;//55.0f

//Modern Hall
//vec3 look_from(6.9118194580078125f, 1.6516278982162476f, 2.5541365146636963f);
//vec3 look_at(2.328019380569458f, 1.6516276597976685f, 0.33640459179878235f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 70.0f;

//Country Kitchen
//vec3 look_from(1.211004614830017, 1.8047512769699097, 3.8523902893066406);
//vec3 look_at(-1.261659026145935, 1.5695619583129883, -1.215745449066162);

//vec3 view_direction = (look_at - look_from).norm();

//float fov = 60.0f;


//Coffee Maker
//vec3 look_from(-0.002960f, 0.198303f, 0.828150f);
//vec3 look_at(0.002896f, 0.166609f, 0.022717f);


//vec3 look_from(-0.016554f, -0.369325f, 4.241527f);
//vec3 view_direction(0.002120f, 0.989652f, -0.143472f);

//vec3 look_from(0.018517f, -0.377869f, 0.328335);
//vec3 view_direction(-0.004457f, 0.943093f, -0.332499f);

//vec3 view_direction = (look_at - look_from).norm();


//vec3 look_from(-0.020426, 0.305208, 0.352948);
//vec3 view_direction(0.015692, -0.324043, -0.945912);


//vec3 look_from(0.027780, 0.188531, 0.293707);

//vec3 view_direction(-0.056632, -0.073669, -0.995673);

//vec3 look_from(0.192458, 0.2303169, 0.258120);
//vec3 view_direction(-0.320231, -0.070173, -0.944737);

//good
//vec3 look_from(0.177848, 0.193122, 0.357804);
//vec3 view_direction(-0.239257, -0.043385, -0.969987);

//bad
//vec3 look_from(0, -1, 0.15);
//vec3 view_direction(0, 0, -1.0f);

//coffe final good
//vec3 look_from(0.182988f, 0.196638f, 0.226799f);
//vec3 view_direction(-0.554256f, -0.086899f, -0.827798f);
//float fov = 60.0f;

//tung sten
//vec3 look_from(-0.0029600381385535f, 0.19830328226089478f, 0.828150749206543f);
//vec3 look_at(0.002896534511819482f, 0.16660931706428528f, 0.022717338055372238f);

//vec3 view_direction = (look_at - look_from).norm();

//original bad
//vec3 look_from(0.36, 0.0, 0.2);
//vec3 view_direction(-1, 0, 0);


//braun final version
//vec3 look_from(0.190005f, 0.223990f, 0.244216f);
//vec3 view_direction(-0.599609f, -0.189056f, -0.777642f);
//float fov = 60.0f;

//vec3 look_from(0.0f, 1.0368f, 0.15051f);
//vec3 view_direction(0.0f, -1.0f, 0.0f);
//float fov = 65.0f;

//Stair Case
//vec3 look_from(0.032420f, 1.526729f, 4.881969f);
//vec3 look_at(-0.018809f, 1.828609f, -1.811280f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 70.0f;

//FirePlace Room
vec3 look_from(5.105184f, 0.731065f, -2.317890f);
vec3 look_at(1.452592f, 1.013640f, -1.317287f);
vec3 view_direction = (look_at - look_from).norm();
float fov = 70.0f;

//Living Room Benedik

//vec3 look_from(2.250089f, 1.311707f, 6.178715f);
//vec3 look_at(0.022981f, 1.123810f, 1.561731f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 60.0f;

//bed Room
//vec3 look_from(2.041891f, 1.429728f, 2.295251f);
//vec3 view_direction(-0.693090f, -0.046574f, -0.719345f);
//float fov = 90.0f;

//Space Ship
//original view
//vec3 look_from(-0.519663f, 0.817007f, 3.824389f);



//Bad View 90 degree rotata view
//vec3 look_from(-0.519663f, 3.824389f, 1.217007f);

//Good View
//vec3 look_from(-0.519663f, 1.217007f, 3.824389f);
//vec3 look_at(-0.066870f, 0.644895f, 0.529278f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 60.0f;
//float fov = 46.0f;

//Victorian House

//Sang position
//vec3 look_from(-22.736923f, 1.321379f, 14.055666f);
//vec3 view_direction(0.756694f, -0.143841f, -0.637749f);
//float fov = 70.0f;

//benedik position
//vec3 look_from(-37.466297f, -0.614253f, 32.122295f);
//vec3 look_at(-3.499759f, 7.133240f, -5.072649f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 20.6f;

//Dining Room
//vec3 look_from(-0.587317f, 2.7623f, 9.7142896f);
//vec3 look_at(-0.391763f, 1.805899f, -5.22966f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 46.0f;

//Lamp
//vec3 look_from(7.755991f, 5.067980f, -6.643479f);
//vec3 look_at(0.238440f, 2.568779, 1.390990f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 36.0f;

//Salle_De_Bain

//vec3 look_from(4.443147f, 16.934431f, 49.910232f);
//vec3 look_at(-2.573489f, 9.991769f, -10.588199f);		//vec3 look_at(-16.573489f, 9.991769f, -10.588199f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 50.0f;

//kitchen
//vec3 look_from(1.211004f, 1.804751f, 3.852390f);
//vec3 look_at(-1.261659f, 1.569561f, -1.215745f);
//vec3 view_direction = (look_at - look_from).norm();
//float fov = 60.0f;



//bread bin test
//vec3 look_from(-1.637141f, 1.453324f, 0.845030f);
//vec3 view_direction(-0.957332f, -0.287480f, -0.029497f);
//float fov = 60.0f;

//Hexa Ball
//vec3 look_from(-16.448154f, 69.273582f, 51.967617f);
//vec3 view_direction(0.242190f, -0.646819f, -0.723166f);
//float fov = 80.0f;

//Sibenik Point Light
//vec3 look_from(-11.236954, -11.983788, 2.243523);
//vec3 view_direction(0.955283, 0.206913, -0.211236);
//float fov = 60.0f;

//Dabrovic
//vec3 look_from(-11.392294, 2.706525, -0.121931);
//vec3 view_direction(0.994476f, 0.103216f, 0.019100f);
//float fov = 64.0f;

const vec3 w = -view_direction;
const vec3 up = vec3(0, 1, 0);
const vec3 u = up.cross(w).norm();
const vec3 v = w.cross(u);
const float tan_theta = tanf(fov * 0.5f * pi  / 180.0f);

const float asspect_tant = aspect_ratio * tan_theta;



static void new_multi_jitter(vector<vec2>& sample)
{
	int sqrt_sample = sqrt_ns;
	float sub_cell_width = 1.0f / float(ns);

	for (int i = 0; i < sqrt_sample; ++i)
	{
		for (int j = 0; j < sqrt_sample; ++j)
		{
			sample[i * sqrt_sample + j].x = i * sqrt_sample * sub_cell_width + j * sub_cell_width + randf() * sub_cell_width;
			sample[i * sqrt_sample + j].y = j * sqrt_sample * sub_cell_width + i * sub_cell_width + randf() * sub_cell_width;

		}
	}

	for (int i = 0; i < sqrt_sample; ++i)
	{
		for (int j = 0; j < sqrt_sample; ++j)
		{
			int k = j + int(randf() * (sqrt_sample - j - 1));
			swap(sample[i * sqrt_sample + j].x, sample[i * sqrt_sample + k].x);
		}
	}

	for (int i = 0; i < sqrt_sample; ++i)
	{
		for (int j = 0; j < sqrt_sample; ++j)
		{
			int k = j + int(randf() * (sqrt_sample - j - 1));
			swap(sample[j * sqrt_sample + i].x, sample[k * sqrt_sample + i].x);
		}
	}
}

//remove in light and count light
//void __fastcall determine_visibility(node*& n, Scene& scn, vector<bool>& visible, vector<bool>& in_light, int num_sample, int num_shadow, int& count, int& count_light)
void __fastcall determine_visibility(node*& n, Scene& scn, vector<bool>& visible, int num_sample, int num_shadow, int& count)
{
	for (int j = 0; j < Height; ++j)
	{
		#pragma omp for schedule(guided)
		for (int i = 0; i < Width; ++i)
		{
			int light_block = 0;

			float u_index = i;
			float v_index = j;

			for (int s = 0; s < num_sample; ++s)
			{
				HitRecord rec;

				float p = (u_index + randf()) * iWidth;
				float q = (v_index + randf()) * iHeight;

				p = (2.0f * p - 1.0f) * aspect_ratio * tan_theta;
				q = (1.0f - 2.0f * q) * tan_theta;
				Ray r(look_from, u * p + v * q - w);

				bool b = scn.all_hit(n, r, rec);
				int mtl = scn.trs[rec.ti].mtl;

				if (b && !scn.mats[mtl].isLight)//
				{
					//cout << "a";
					int ti = rec.ti;
					
					//int vn0 = scn.trs[ti].vn[0];
					//int vn1 = scn.trs[ti].vn[1];
					//int vn2 = scn.trs[ti].vn[2];

					vec3 rec_normal = rec.n;//vn0 < 0 ? scn.trs[ti].n : scn.intp(scn.vn[vn0], scn.vn[vn1], scn.vn[vn2], rec.u, rec.v);

					
					//vec3 rec_normal(scn.trs[ti].n);

					vec3 hit_point(r.o + r.d * rec.t + rec_normal * 0.0002f);
					for (int j = 0; j < num_shadow; ++j)
					{
						int li = scn.Ls.sample();
						vec3 light_point(scn.Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length() * 0.999f;
						Ray shadow_ray(hit_point, light_direction);

						//if (scn.hit_anything_range(n, shadow_ray, length))
						++light_block;
					}
				}
			}
			if (light_block == num_sample * num_shadow)
			{
				visible[j * Width + i] = false;
				++count;
			}
			/*if (light_block == 0)
			{
				in_light[j * Width + i] = true;
				++count_light;
			}*/
		}
	}
}

void compute_color_normal(node*& n, Scene& scn, vector<vec3>& color, vector<vec3>& normal)
{
	for (int j = 0; j < Height; ++j)
	{
		#pragma omp parallel for schedule(guided)
		for (int i = 0; i < Width; ++i)
		{
			vec3 sum_color(0.0f);
			vec3 sum_norm(0.0f);

			for (int s = 0; s < 20; ++s)
			{
				float p = ((float)i + randf()) * iWidth;
				float q = ((float)j + randf()) * iHeight;

				p = (2.0f * p - 1.0f) * aspect_ratio * tan_theta;
				q = (1.0f - 2.0f * q) * tan_theta;

				Ray new_ray(look_from, (u * p + v * q - w).norm());

				vec3 col(0.0f);
				vec3 norm(0.0f);

				scn.ray_casting_color_normal(n, new_ray, col, norm);

				sum_color += col;
				sum_norm += norm;
			}

			int ind = j * Width + i;
			normal[ind] = sum_norm * 0.05f;
			color[ind] = sum_color * 0.05f;
		}
	}
}

float compute_shadow_order(node*& n, Scene& scn, const int& ind)
{
	clock_t t = clock();

	for (int j = 0; j < Height; ++j)
	{
		#pragma omp parallel for schedule(guided)
		for (int i = 0; i < Width; ++i)
		{
			for (int s = 0; s < 10; ++s)
			{
				float p = ((float)i + randf()) * iWidth;
				float q = ((float)j + randf()) * iHeight;

				p = (2.0f * p - 1.0f) * aspect_ratio * tan_theta;
				q = (1.0f - 2.0f * q) * tan_theta;

				Ray new_ray(look_from, (u * p + v * q - w).norm());

				HitRecord rec;

				if (scn.all_hit(n, new_ray, rec))
				{
					vec3 hit_point(new_ray.o + new_ray.d * rec.t + rec.n * 0.0002f);
					for (int t = 0; t < 5; ++t)
					{
						int li = scn.Ls.sample();

						vec3 light_point(scn.Ls.sample_light(li));

						vec3 light_direction(light_point - hit_point);

						float length = light_direction.length();

						Ray light_ray(hit_point, light_direction);

						scn.hit_anything_range_test(n, light_ray, length * 0.999f, ind);
					}
				}
			}
		}
	}

	t = (clock() - t) * 0.001f;

	return t;
}



static vec3 denan(const vec3& v)
{
	vec3 temp(v);
	if (!(temp.x == temp.x))
		temp.x = 0.0f;
	if (!(temp.y = temp.y))
		temp.y = 0.0f;
	if (!(temp.z == temp.z))
		temp.z = 0.0f;

	return temp;
}

static float Luminance(const vec3& v)
{
	return 0.2126f * v.x + 0.7152f * v.y + 0.0722 * v.z;
}
/*
static vec3 clamp_vector(vec3& v, const float& low, const float& high)
{
	float x = min(max(low, v.x), high);
	float y = min(max(low, v.y), high);
	float z = min(max(low, v.z), high);

	return vec3(x, y, z);
}

//Remove The Mysterious Purple Dots Appear Across The Image.
//This Error sometime appear out of nowhere.
//The Pixels affected by this error have Green Channel = 0, The Remanning Red and Blue Channle make the pixel appear Purple.
//vec3 is perfectly.  
//What is the reason behind this phenomenal.
//Volume Image, Wooden Stair Case and Storm Trooper images was affect by this error.
void de_purple(string& file_name)
{
	ifstream ifs(file_name);

	char line[512];

	ifs.getline(line, 512);
	ifs.getline(line, 512);
	ifs.getline(line, 512);

	vector<vec3> color;

	while (ifs.getline(line, 512))
	{
		float x, y, z;

		ifs >> x;
		ifs >> y;
		ifs >> z;
		color.push_back(vec3(x, y, z));
	}

	int size = color.size();

	int dx[4] = { 1, 0, -1, 0 };
	int dy[4] = { 0, 1,  0, -1 };
	for (int i = 0; i < size; ++i)
	{
		if (color[i].y == 0 && color[i].x > 0 && color[i].z > 0)
		{
			int x_coord = i % Width;
			int y_coord = i / Width;

			int count_none_purple = 0;
			
			vec3 sum(0.0f);

			for (int j = 0; j < 4; ++j)
			{
				int new_x = x_coord + dx[j];
				int new_y = y_coord + dy[j];

				if (new_x >= 0 && new_x < Width && new_y >= 0 && new_y < Height)
				{
					int ind = new_y * Width + new_x;

					if (color[ind].y > 0.0f)
					{
						++count_none_purple;
						sum += color[ind];
					}
				}
			}

			if (count_none_purple >= 3)
				color[i] = sum / count_none_purple;
		}
	}

	ofstream ofs(file_name + "De_Purple.ppm");

	ofs << "P3\n" << Width << " " << Height << "\n255\n";

	for (int i = 0; i < size; ++i)
		ofs << color[i].x << " " << color[i].y << " " << color[i].z << "\n";
}

void main_2()
{
	string file_name = "Volume_10240_10240_9.ppm";

	de_purple(file_name);
}

*/
//Ghi chu
//su dung prev_normal lam cho nguon sang bi xam nhin ko that
//prev_normal su dung de tinh pdf light
//khong nen su dung
void main()
{
	cout << "Fire Room\n";
	//cout << "Daki Room\n";
	//cout << "Living Room\n";
	//cout << "Sexy Stair\n";
	//cout << "Dining Room\n";
	//cout << "Kitchen\n";

	//kitchen
	//look_from.x -= 0.4f;
	//look_from.z -= 0.4f;

	//salle de bain full wall
	//final
	//look_from += view_direction * 2.1f;

	//look_from.y += 3.0f;
	//look_from.x += 5.0f;
	//look_from.z -= 2.0f;
	//look_from -= view_direction * 14.1f;

	//Sang
	//victorian house
	//look_from += view_direction * 3.6f;
	//look_from.x -= 2.7f;

	//coffe maker
	//look_from -= view_direction * 0.12f;
	//look_from.x -= 0.021f;

	/*
	look_from -= view_direction * 0.12f;
	look_from.x += 0.069f;

	look_from += view_direction * 0.29f;

	look_from.y += 0.12f;
	*/

	//look_from += view_direction * 0.06f;

	//look_from -= view_direction * 0.05f;

	//look_from += view_direction * 0.1f;

	//Stair Case
	//look_from += view_direction * 0.12f;

	//look_from += view_direction * 0.2f;

	clock_t t1;
	t1 = clock();

	//test crash
	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj");

	//Crytek Sponza
	//Scene scn("E:\\Models\\crytek_sponza\\textures\\crytek_sponza.obj");

	//Dabrovic Sponza
	//(-0.9f, 4.8f, 0.5f)
	//doi voi mo hinh dabrovic
	//chieu duong x la chieu di vao man hinh
	//chieu duong y duong di thang len
	//chieu duong z tu trai qua phai

	//vec3(0.0f, 1.0f, -0.2f)

	//Scene scn("E:\\Models\\dabrovic_sponza\\sponza.obj", true, true, vec3(-0.4f, 1.0f, -0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f);

	//Scene scn("E:\\Models\\dabrovic_sponza\\sponza.obj", true, true, vec3(-0.6f, 1.0f, -0.4f), 0.04, true, false, "Sky 19.hdr", 26.0f);

	//Scene scn("E:\\Models\\dabrovic_sponza\\sponza.obj", true, true, vec3(-0.2f, 1.0f, -0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f);

	//Scene scn("E:\\Models\\dabrovic_sponza\\sponza.obj");

	//Sibenik
	
	//(6.440733, 0.532747, 0.109297);
	//vec3(7.198114, 0.964133, 0.264984)


	//look_from -= view_direction * 2.0f;
	//look_from.x -= 1.2f;
	//float a = 90.0f;
	
	//Scene scn("sibenik.obj", vec3(6.440733, 0.532747, 0.109297), vec3(a, a, a));
	//Scene scn("E:\\Models\\sibenik\\sibenik.obj", vec3(6.440733, 0.532747, 0.109297), vec3(a, a, a));
	
	//Material Test Ball
	//Scene scn("E:\\Models\\Material_Test_Ball\\Painter\\painter_with_light_And_light_screw.obj");

	//kitchen
    //E:\Models\kitchen\blender\Country-Kitchen 
    //E:\Models\country_kitchen

	//look_from += view_direction * 1.19f;
	//look_from.x -= 0.5f;

	//Scene scn("Country_Kitchen.obj");
	//Scene scn("E:\\Models\\kitchen\\blender\\Country_Kitchen.obj");
  

	//Salle De Bain
	//look_from += view_direction * 2.1f;

	//Scene scn("salle_de_bain_stainless_extended_wall.obj");

	//Scene scn("E:\\Models\\salle_de_bin_bath_room\\salle_de_bin\\textures\\salle_de_bain_stainless_extended_wall.obj");

	//Lamp
	//Scene scn("Little_Lamp2.obj");
	//Scene scn("E:\\Models\\Lamp\\blender\\Little_Lamp2.obj");


	//dining Room
	//goc (0.5, 0.5, 0.1)
	//look_from.z -= 1.9f;

	//look_from -= view_direction.x * 1.7f;
	//Scene scn("E:\\Models\\dining_room\\blender\\Dining_Room_Adjust_Texture.obj", true, true, vec3(0.5f, 0.5f, 0.1f), 0.02f, true, true, "Sky 19.hdr", 0.0f);
	//Scene scn("The_Breakfast_Room.obj", true, true, vec3(0.5f, 0.5f, 0.1f), 0.02f, true, true, "Sky 19.hdr", 0.0f);

	//cout << scn.compute_sky_color << "\n";

	//Scene scn("E:\\Models\\dining_room\\blender\\The_Breakfast_Room.obj");


	//Victorian House
	
	//best version
	//Scene scn("E:\\Models\\Victorian_House\\blender\\Victorian_House.obj", false, true, vec3(-0.5f, 0.5f, -0.9f), 0.26f, true, true, "", 0.0f);
	
	//Scene scn("E:\\Models\\Victorian_House\\blender\\Victorian_House.obj");

	//SpaceShip

	//Final2
	//Scene scn("E:\\Models\\spaceShip\\Blender\\SpaceShip_Final_2.obj");

	//Red Corver version
	//Scene scn("E:\\Models\\spaceShip\\Blender\\SpaceShip_Full_Mtl_Add_Red_Light_Low_Poly.obj");

	//Red Light Version
	//Scene scn("E:\\Models\\spaceShip\\Blender\\SpaceShip_Full_Mtl_Add_Red_Light_Control.obj");

	//Smooth Version
	//Scene scn("E:\\Models\\spaceShip\\Blender\\SpaceShip_Full_Mtl_Subdivision_lighter.obj");
	
	//Scene scn("E:\\Models\\spaceShip\\Blender\\SpaceShip_Full_Mtl_Fix_Light.obj");

	//Scene scn("E:\\Models\\spaceShip\\obj\\SpaceShip.obj");

	//BathRoom
	//Scene scn("E:\\a_a_Final_Model_Rendering\\bathroom_one_tube\\contemporary_bathroom.obj");

	//Bed Room
	
	//Final Scene With Mirror
	//Scene scn("E:\\Models\\blender_to_obj\\bedroom_slykdrako\\textures\\BedRoom_With_Mirror.obj");

	//Good
	//Scene scn("E:\\Models\\blender_to_obj\\bedroom_slykdrako\\textures\\bedroom.obj");

	//Scene scn("E:\\Models\\blender_to_obj\\bedroom_slykdrako\\BedRoom_With_Light.obj");

	//bad
	//Scene scn("E:\\Models\\BedRoom\\pbrt\\pbrt_to_obj\\BedRoom_Final.obj");

	//sibenik
	//Scene scn("E:\\Models\\sibenik\\sibenik.obj");

	//living Room
	//look_from += view_direction;
	//look_from.z += 0.4;
	//look_from.x -= 0.1f;

	//Scene scn("living_room.obj");
	//Scene scn("E:\\Models\\Living_Room\\living_room\\textures\\living_room.obj");
	
	//Scene scn("E:\\Models\\Living_Room\\blender\\The_White_Room.obj");

	//FirePlaceRoom

	//Scene scn("E:\\a_a_Sang_Ray_Tracing\\Advance_Features\\40_Sponza_Palace\\Sponza\\Models\\fireplace_room\\fireplace_room.obj");

	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj", "Sky 19.hdr");

	//int ind = 9;
	Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj", "env9_blur.jpg"); 
	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj", "env" + to_string(ind) + ".jpg");

	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj");

	//stair case
	//look_from += view_direction;
	//Scene scn("StairCase_no_Carpet2.obj");
	//Scene scn("E:\\Models\\stair_case\\final_model\\StairCase_no_Carpet2.obj");
	//cout << scn.Ls.Ke.size() << "\n";

	//Scene scn("E:\\Models\\stair_case\\final_model\\StairCase_with_mtl_Light.obj");

	//Scene scn("E:\\Models\\blender_to_obj\\wooden_staircase\\StairCase_with_mtl.obj");

	//Scene scn("E:\Models\blender_to_obj\wooden_staircase");


	//coffer

	//Scene scn("E:\\Models\\coffe_maker\\a_coffe_final_braun_fix\\coffe_and_baby_stand_on_ground_move_up_2.obj");
	
	//Scene scn("E:\\Models\\coffe_maker\\a_coffe_final_braun_fix\\coffe_final_fix_braun.obj");
	//Scene scn("E:\\Models\\coffe_maker\\coffe_final\\coffe_final_modify.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffe_final\\coffe_final.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\coffe_maker_modify_no_hole.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\coffe_maker_modify_final_copy.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\pbrt_coffe.obj");

	//Scene scn("E:\\Models\\coffe_maker\\original_blend\\Braun_KS_20_Finished_Finale.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\coffe_maker_modify_final_good_braun.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\a_final_coffe_braun.obj");
	
	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\a_b.obj");

	//Scene scn("E:\\Models\\country_kitchen\\Country-Kitchen.obj");

	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10.obj");

	//Scene scn("E:\\Models\\Modern_Hall_Fix_Floor\\fix2\\Hall10.obj");

	
	//Scene scn("E:\\Models\\Modern_Hall_Fix_Floor\\Hall10.obj");

	
	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10_fix_normal2.obj");

	//Scene scn("E:\\a_a_Final_Model_Rendering\\bathroom_one_tube\\contemporary_bathroom.obj");
	

	//Modern Hall

	//Scene scn("a_Hall10.obj");
	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10.obj");

	
	

	t1 = clock() - t1;
	cout << "Reading time: " << t1 / 1000.0f << "s" << "\n";

	
	//cout<<"Normal size: " << scn.vn.size() << "\n";

	/*for (int i = 0; i < scn.trs.size(); ++i)
	{
		cout << scn.trs[i].vn[0] << " " << scn.trs[i].vn[1] << " " << scn.trs[i].vn[2] << "\n";
	}*/

	clock_t t_build;
	t_build = clock();

	cout << sizeof(node) << "\n";
	cout << scn.trs.size() << "\n";
	node* n;

	//scn.build_bvh_bin(n, 6);

	//scn.build_bvh_no_extra_triangle_space(n, 6);

	scn.build_bvh(n, 6);

	scn.delete_before_render();
	//scn.n = n;


	t_build = clock() - t_build;
	cout << "Building time: " << t_build / 1000.0f << "s" << "\n";

	int size = Width * Height;

	vector<vec3> c;

	c.resize(size);
	

	clock_t t2 = clock();

	//test material//good
	/*
	for (int i = 0; i < scn.mats.size(); ++i)
	{
		cout << scn.mats[i].Matname << " " << scn.mats[i].Ke.x << " " << " " << scn.mats[i].Ke.y << " " << " " << scn.mats[i].Ke.z << "\n";
	}
	*/
	/*
	cout << "Light Size :" << scn.Ls.Ke.size() << "\n";
	cout << "Light List: \n";

	for (auto& v : scn.light_map)
	{
		int tr = v.first;

		int mtl = scn.trs[tr].mtl;
		cout << "Mat index: " << mtl << "\n";
		cout << v.first << " : " << v.second << " " << scn.mats[mtl].Matname << "\n";
	}
	cout << "Real Light List: \n";
	cout << scn.trs.size() << "\n";
	for (int k = 0; k < scn.trs.size(); k++)
	{
		int mtl = scn.trs[k].mtl;

		if (scn.mats[mtl].isLight)
		{
			cout << k <<" : " << scn.mats[mtl].Matname << "\n";
		}
	}
	*/


	
	vector<bool> visible(Width * Height, true);
	//vector<bool> in_light(Width* Height, false);

	int num_sample = 6;
	int num_shadow = 12;

	//vec3 prev_normal(view_direction);
	omp_set_num_threads(512);
	/*
	if (scn.use_area_light)
	{
		int count = 0;
		int count_light = 0;
		//determine_visibility(n, scn, visible, in_light, num_sample, num_shadow, count, count_light);
		determine_visibility(n, scn, visible, num_sample, num_shadow, count);
		//cout << "Num pixels in shadow: " << count << "\n";
		//cout << "Num pixels totally in light: " << count_light << "\n";

		t2 = clock() - t2;
		cout << "Visibility check: " << t2 / 1000.0f << "s" << "\n";

		clock_t t_shadow = clock();

		//int first_node = compute_shadow_order(n, scn,);

		//scn.first_shadow_node = first_node;

		float t_left = compute_shadow_order(n, scn, 0);
		float t_right = compute_shadow_order(n, scn, 1);

		scn.first_shadow_node = t_left < t_right ? 1 : 0;

		scn.second_shadow_node = 1 - scn.first_shadow_node;

		t_shadow = clock() - t_shadow;

		cout << "Compute Shadow node order: " << t_shadow / 1000.0f << "s" << "\n";
	}
	*/

	
	clock_t t_render = clock();
	/*for (int j = 0; j < Height; ++j)
	{
		fprintf(stderr, "\rRendering (%d spp) %5.2f%%", ns, 100.0f * j / (Height - 1));
		#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < Width; ++i)*/
	for (int i = 0; i < Width; ++i)
	{
		fprintf(stderr, "\rRendering (%d spp) %5.2f%%", ns, 100.0f * i / (Width - 1));
		#pragma omp parallel for schedule(dynamic)
		for (int j = 0; j < Height; ++j)
		{
			vector<vec2> sample;
			sample.resize(ns);

			new_multi_jitter(sample);

			int num_sample_use = 0;

			//int index = j * Width + i + rand() % Width;
			//int sample_so_far = 0;
			bool converge = false;

			vec3 sum(0.0f);

			float sum_sqr = 0.0f;
			float sum_so_far = 0.0f;

			int pixel_index = j * Width + i;

			float convergence_rate;

			if (visible[pixel_index])
				convergence_rate = 0.001f;
			else
				convergence_rate = 0.0001f;

			
			for (int s = 0; s < ns; s += step)
			{

				//float color = 0.0f;

				//float p_index = i;
				//float q_index = j;
				for (int num = 0; num < step; ++num)
				{

					float p = ((float)i + sample[s + num].x) * iWidth;
					float q = ((float)j + sample[s + num].y) * iHeight;

					p = (2.0f * p - 1.0f) * aspect_ratio * tan_theta;
					q = (1.0f - 2.0f * q) * tan_theta;

					Ray new_ray(look_from, (u * p + v * q - w).norm());

					//Material Roughness
					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke_alpha_roughness(n, new_ray)));


					//Sibenik Point Light
					//vec3 current_color(denan(scn.sibenik_tracing_point_light_only(n, new_ray)));

					//victorian house, Dining Room

					//vec3 current_color(denan(scn.sibenik_tracing_directional_lighting(n, new_ray)));


					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke_alpha(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Le(n, new_ray)));
					
					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke(n, new_ray)));

					vec3 current_color(denan(scn.sibenik_tracing_blur_enviroment(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_enviroment(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Le_clamp(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke(n, new_ray)));

					
					//vec3 current_color(denan(scn.ray_casting_normal(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal(n, new_ray)));

					//vec3 current_color(denan(scn.ray_casting(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_enviroment(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light(n, new_ray)));
					
					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal(n, new_ray)));
					
					//vec3 current_color(denan(scn.sibenik_tracing_volume(n, new_ray)));

					float lux = Luminance(current_color);

					sum_sqr += lux * lux;
					sum_so_far += lux;

					sum += current_color;
				}
				num_sample_use += step;

				float mean_sum = sum_so_far / num_sample_use;

				float variance = (sum_sqr / num_sample_use - mean_sum * mean_sum) / (num_sample_use - 1);

				vec3 value(sum / num_sample_use);


				if (variance < convergence_rate && value.minc() > 0.2f)
				{
					c[j * Width + i] = sum / num_sample_use;
					converge = true;
					break;
				}
			}

			if (!converge)
				c[j * Width + i] = sum * ins;
				

		}
	}
	ofstream ofs("Daki_Room_Final_Test_scale_20_blur_env_6.6_gaussian_Final_Original.ppm");
	//ofstream ofs("Wall_Half_Bright_Sky_19_hdr_raw_inv_gammar_2.2.ppm");
	//ofstream ofs("Daki_Room_Test_env_" + to_string(ind) + "_blur_0.06_Conductor_Al_black_0.9_env_jpg.ppm");
	//ofstream ofs("Daki_Room_Final_Check_Medium_512_blur_0.04.ppm");

	//ofstream ofs("Daki_Room_optimize_Rough_Plastic_Diffuse_Reflect.ppm");

	//ofstream ofs("Daki_Room_Test_speed_Remove_in_light_memory_remove_w_h_texture_variable.ppm");

	//ofstream ofs("Daki_Room_Golden_Ratio_env_scale_26_blur_radius_0.02.ppm");

	//ofstream ofs("Stair_Case_Throughput_Positive_All_Diffuse_pdf_0.ppm");

	//ofstream ofs("Stair_Case_Le_remove_negative_path_or_pdf_no_macro_512.ppm");

	//ofstream ofs("a_a_a_a_a_Daki_Room_Up_Right_Blur_Env_0.2f_scale_26_64spp.ppm");
	
	//ofstream ofs("Living_Room_2048.ppm");
	//ofstream ofs("Living_Room_Final_Test_Big_Plastic_Black_ior_1.2.ppm");
	//ofstream ofs("Living_Room_Transparent_Blind_Ke_5.ppm");
	//ofstream ofs("Test_Stair_Case_Ke_alpha.ppm");

	//ofstream ofs("Living_Room_move_camera.ppm");

	//ofstream ofs("Stair_Case_4096_Final.ppm");
	
	//ofstream ofs("Stair_Case_Final_Test_For_4096.ppm");

	//ofstream ofs("Stair_Case_Final_Rough_Conductor_new_GGX_Test_before_render_4096.ppm");

	//ofstream ofs("a_Stair_Case_check_cos_i_m_condition_rough_plastic_rough_conductor.ppm");

	//ofstream ofs("a_Stair_Case_cos_i_h_1e-6.ppm");

	//ofstream ofs("a_Stair_Case_Test_Final_No_More_Speckale_v4.ppm");

	//ofstream ofs("Dining_Room_Adjust_Texture.ppm");

	//ofstream ofs("Dining_Room_2048.ppm");

	//ofstream ofs("Dining_Room_Last_Test_Before_Rendering.ppm");
	//ofstream ofs("Dining_Room_Final_Test_Before_Render_no_frosted_glass.ppm");

	//ofstream ofs("Dining_Room_test_location_square_blur_fix_env_64.ppm");

	//ofstream ofs("Dining_Room_Test_Fast_Speed_512.ppm");

	//ofstream ofs("Dining_Room_Test_1e-6_condition_Conductor_chair.ppm");

	//ofstream ofs("Stair_Case_GGX_Test_1e-6_condition_512.ppm");

	//ofstream ofs("Bath_Room_GGX_alpha2_original_no_macro_square_no_macro_max_512_big.ppm");

	//ofstream ofs("a_Stair_Case_Beckmann_256_1e-6_microfacet_0_condition.ppm");
	//ofstream ofs("Bath_Room_Test_Plastic_Diffuse.ppm");

	//ofstream ofs("Stair_Case_Test_Plastic_Diffuse_real_GGX_all_set_false.ppm");

	//ofstream ofs("Stair_Case_512_Test_Plastic_Diffuse_2_remove_purple_speck_1e_6_include_GGX.ppm");

	//ofstream ofs("Stair_Case_Jean_Bikini.ppm");
	
	//ofstream ofs("Stair_Case_Lux_Bikini.ppm");

	//ofstream ofs("Stair_Case_before_render_2.6.ppm");

	//ofstream ofs("Stair_Case_Test_2.9_64_before_render.ppm");

	//ofstream ofs("Coffe_And_Baby_final_4096_4.0_final_version.ppm");

	//ofstream ofs("coffe_and_baby_final_stand_on_ground_move_up_1.ppm");

	//ofstream ofs("coffe_and_baby_braun_final_24.6.ppm");

	//ofstream ofs("Curve_Coffe_Braun_real_position_real_scale.ppm");

	//ofstream ofs("Stair_Case_With_Lux_And_Ezreal.ppm");

	//ofstream ofs("a_final_coffe_braun.ppm");

	//ofstream ofs("a_a_Retest_Country_Kitchen_Rough_Plastic.ppm");
	//ofstream ofs("a_a_Modern_Hall_test.ppm");
	//ofstream ofs("a_Modern_Hall_fix_rough_plastic_specular_speck_rough_plastic_diffuse_new_isspecular_sort.ppm");

	//ofstream ofs("a_Coffe_Maker.ppm");

	//ofstream ofs("a_Country_Kitchen.ppm");

	//ofstream ofs("a_Lamp_4096.ppm");

	//ofstream ofs("a_Lamp_before_render_Conductor_bentrong_ben_ngoai.ppm");
	
	//ofstream ofs("a_Contry_Kitchen.ppm");
	//ofstream ofs("a_Country_Kitchen_Test_Scene_bread_bump.ppm");
	//ofstream ofs("a_Final_Before_Render.ppm");
	//ofstream ofs("a_Test_Blind_Wood_Strip.ppm");
	//ofstream ofs("a_Kitchen_Big_14_2.ppm");

	//ofstream ofs("a_Lamp.ppm");

	//ofstream ofs("a_Space_Ship_2048.ppm");

	//ofstream ofs("a_Salle_De_Bain.ppm");

	//ofstream ofs("a_Dabrovic_Sponza_No_reljef_bump_1024.ppm");

	//ofstream ofs("a_Victorian_House_Blur_Light_0.26f_2048.ppm");
	
	/*
	float *cf = new float[Width * Height * 3];
	for (int i = 0; i < Width * Height; i++) 
	{
		//for (int j = 0; j < 3; j++) cf[i * 3 + j] = Filmic_unchart2(c[i][j]);

		vec3 filmic = Filmic_unchart2(c[i]);

		cf[i * 3 + 0] = filmic.x;
		cf[i * 3 + 1] = filmic.y;
		cf[i * 3 + 2] = filmic.z;
	}
	FILE *f = fopen("Stair_Case_256_filmic.pfm", "wb");
	fprintf(f, "PF\n%d %d\n%6.6f\n", Width, Height, -1.0);
	fwrite(cf, Width * Height * 3, sizeof(float), f);
	
	delete(cf);
	*/

	//ofstream ofs("Stair_Case_Clamp_threshhold_4_bikini_1024.ppm");
	
	//ofstream ofs("Stair_Case_microfacet_pdf_0_if_too_small_cos_i_h_eps_sample_function_only.ppm");
	//ofstream ofs("Stair_Case_Readmtl2_Diffuse_eps_sample_eval_pdf.ppm");
	//ofstream ofs("Stair_Case_No_Positive_Condition_fix_GGX_eps_condition_pdf_0_all_false.ppm");
	//ofstream ofs("Test_Stair_Case_Purple_Noise_positive_cos_original_texture_full_mtl.ppm");
	//ofstream ofs("Stair_Case_Ke_30_Fix_Fire_Fly.ppm");
	//ofstream ofs("Stair_Case_Test_Ray_Casting_fix_bvh.ppm");
	ofs << "P3\n" << Width << " " << Height << "\n255\n";
	

	for (int i = 0; i < size; ++i)
	{
		//vec3 color = Filmmic_Tone_mapping(c[i]);
		vec3 color = Filmic_unchart2(c[i]);
		//vec3 color = c[i];
		color *= 255.99f;

		ofs << color.x << " " << color.y << " " << color.z << "\n";
	}

	ofs.close();
	

	vector<vec3>().swap(c);
	
	delete_bvh(&n);
	scn.delete_struct();

	t_render = clock() - t_render;

	std::cout << "\nRendering time: " << ((double)t_render) / CLOCKS_PER_SEC << "\n";


	

	getchar();
	
}

