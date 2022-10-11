//#include "Scene.h"
//#include "math.h"
//#include "Scene_Prestigious_Path_Tracing.h"
//#include "Smooth_Normal_Scene.h"
//#include "Scene.h"
//#include "Scene_Save_Space.h"
#include "Scene_16.h"
//#include "Scene_16_No_Compress_Vertex.h"
#include "Tone_Mapping.h"
#include "Filter.h"
#include <time.h>
#include <omp.h>

//http://www.raytracerchallenge.com/bonus/texture-mapping.html

using namespace std;
#define Width 200
#define Height 200

#define iWidth 1 / Width
#define iHeight 1 / Height
#define aspect_ratio Width / Height
#define golden_ratio 1.61803398875


int ns = 16;
const int step = 16;
int sqrt_ns = sqrt14(ns);
float isqrt_ns = 1 / sqrt_ns;

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
//vec3 look_from(5.101118f, 1.083746f, -2.756308f); const vec3 view_direction = vec3(-0.93355, -0.004821, 0.358416).norm();
//float fov = 86.001194f;

//test mirror
//vec3 look_from(2.896949, 1.304485, -2.128901); vec3 view_direction(-0.550573, 0.019999, 0.834548);

//vec3 look_from(2.760269, 1.764775, -0.826542); vec3 view_direction(-0.758540, -0.109779, 0.642313);

//benedik position
//look_at = loom_at haha
//vec3 look_from(5.105184555053711f, 0.7310651540756226f, -2.3178906440734863f); vec3 look_at(1.452592134475708f, 1.0136401653289795f, -1.3172874450683594f); vec3 view_direction = (look_at - look_from).norm();

//vec3 look_from(5.105184555053711f, 1.0310651540756226f, -2.3178906440734863f); vec3 look_at(1.452592134475708f, 1.0136401653289795f, -1.3172874450683594f); vec3 view_direction = (look_at - look_from).norm();


//vec3 look_from(3.577393, 0.706135, -0.603293); vec3 view_direction(-0.651559, - 0.333487, 0.681269);

//sibenik
//const vec3 look_from(-16.567905, -12.021462, -0.521549);const vec3 view_direction = vec3(0.984883, 0.164252, 0.055007).norm();

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

//final good
//vec3 look_from(0.182988f, 0.196638f, 0.226799f);
//vec3 view_direction(-0.554256f, -0.086899f, -0.827798f);
//float fov = 46.0f;

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
//float fov = 35.0f;

//FirePlace Room
//vec3 look_from(5.105184f, 0.731065f, -2.317890f);
//vec3 look_at(1.452592f, 1.013640f, -1.317287f);

//vec3 view_direction = (look_at - look_from).norm();

//float fov = 70.0f;

//Living Room Benedik

//vec3 look_from(2.250089f, 1.311707f, 6.178715f);
//vec3 look_at(0.022981f, 1.123810f, 1.561731f);

//vec3 view_direction = (look_at - look_from).norm();

//float fov = 45.0f;

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
//float fov = 60.0f;

//San Miguel
//vec3 look_from(12.697965f, 2.348103f, 11.928419f);
//vec3 view_direction(0.3555520f, 0.155959f, -0.921565f);
//float fov = 77.0f;

vec3 look_from(6.533221, 2.459555, 7.022907);
vec3 view_direction(0.886697, 0.056137, -0.458931);
float fov = 77.0f;

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

void __fastcall determine_visibility(node*& n, Scene& scn, vector<bool>& visible, vector<bool>& in_light, int num_sample, int num_shadow, int& count, int& count_light)
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
			if (light_block == 0)
			{
				in_light[j * Width + i] = true;
				++count_light;
			}
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
*/
//Ghi chu
//su dung prev_normal lam cho nguon sang bi xam nhin ko that
//prev_normal su dung de tinh pdf light
//khong nen su dung
/*
vec3 fix_camera_scale(Scene& scn, vec3& look_from)
{
	vec3 v_min = scn.v_min;
	vec3 v_max = scn.v_max;

	vec3 box_extend = v_max - v_min;

	vec3 inv_dimension(1.0f / box_extend.x, 1.0f / box_extend.y, 1.0f / box_extend.z);

	vec3 scale = vec3(65535.0f) * inv_dimension;

	vec3 vertex_scale = look_from * scale; //(look_from - v_min) * scale;

	//vec3 look_from_scale = 65535.0f * (look_from - v_min) / (v_max - v_min);

	//return look_from_scale;

	return vertex_scale;
}
*/
void main()
{
	//cout << "How many Samples would you like: ";
	int num_sample_render = ns;

	//cin >> num_sample_render;

	ns = num_sample_render;
	sqrt_ns = sqrt14(ns);
	isqrt_ns = 1 / sqrt_ns;

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
	//look_from.x += 0.020f;

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

	//---------------------------SanMiguel-----------------------------------

	//Scene scn("san-miguel-low-poly.obj", true, true, vec3(-0.4f, 1.0f, -0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f, look_from, false);
	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.2f, 1.0f, 0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f, look_from, false);
	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(-0.1f, 0.7f, 0.2f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);
	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(-0.1f, 0.7f, 0.3f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.6f, 0.3f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	//Good
	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.6f, 0.3f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);


	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.6f, 0.1f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.9f, 0.3f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.95f, 0.27f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(0.1f, 0.95f, 0.27f), 0.06, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	//Scene scn("E:\\Models_For_Rendering\\textures\\san-miguel-low-poly.obj", true, true, vec3(-0.4f, 1.0f, -0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f, look_from, false);

	
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

	//Scene scn("E:\\Models\\dabrovic_sponza\\sponza.obj");

	//Sibenik
	
	//(6.440733, 0.532747, 0.109297);
	//vec3(7.198114, 0.964133, 0.264984)

	//look_from -= view_direction * 2.0f;

	//float a = 90.0f;
	//Scene scn("E:\\Models\\sibenik\\sibenik.obj", vec3(6.440733, 0.532747, 0.109297), vec3(a, a, a));

	//Material Test Ball
	//Scene scn("E:\\Models\\Material_Test_Ball\\Painter\\painter_with_light_And_light_screw.obj");

	//kitchen
    //E:\Models\kitchen\blender\Country-Kitchen 
    //E:\Models\country_kitchen
	//Scene scn("E:\\Models\\kitchen\\blender\\Country_Kitchen.obj");// , true, true, vec3(-0.4f, 1.0f, -0.2f), 0.04, true, false, "Sky 19.hdr", 26.0f);
  
	//Salle De Bain
	//Scene scn("E:\\Models\\salle_de_bin_bath_room\\salle_de_bin\\textures\\salle_de_bain_stainless_extended_wall.obj");

	//Lamp
	//Scene scn("E:\\Models\\Lamp\\blender\\Little_Lamp.obj");

	//dining Room
	//goc (0.5, 0.5, 0.1)
	//Scene scn("E:\\Models\\dining_room\\blender\\The_Breakfast_Room.obj", true, true, vec3(0.5f, 0.5f, 0.1f), 0.02f, true, true, "Sky 19.hdr", 0.0f);
	
	//Scene scn("E:\\Models\\dining_room\\blender\\The_Breakfast_Room.obj");


	//Victorian House
	
	//best version
	//Scene scn("E:\\Models\\Victorian_House\\blender\\Victorian_House.obj", false, true, vec3(-0.5f, 0.5f, -0.9f), true, true, "", 0.0f);
	
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

	//bathRoom
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
	//Scene scn("E:\\Models\\Living_Room\\living_room\\textures\\living_room.obj");
	//Scene scn("E:\\Models\\Living_Room\\living_room\\textures\\living_room.obj");
	//Scene scn("E:\\Models\\Living_Room\\living_room\\textures\\living_room.obj");
	//Scene scn("E:\\Models\\Living_Room\\living_room\\textures\\living_room.obj");
	
	//Scene scn("E:\\Models\\Living_Room\\blender\\The_White_Room.obj");

	//FirePlaceRoom

	//Scene scn("E:\\a_a_Sang_Ray_Tracing\\Advance_Features\\40_Sponza_Palace\\Sponza\\Models\\fireplace_room\\fireplace_room.obj");

	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj", "Sky 19_pregamma_1_pattanaik00_mul_1_autolum.jpg");// "Sky 19.hdr");

	//Scene scn("E:\\Models\\fireplace_room\\FireRoom\\fireplace_room.obj");

	//stair case

	//Scene scn("E:\\Models\\stair_case\\final_model\\StairCase_no_Carpet2.obj");

	//Scene scn("E:\\Models\\stair_case\\final_model\\StairCase_with_mtl_Light.obj");

	//Scene scn("E:\\Models\\blender_to_obj\\wooden_staircase\\StairCase_with_mtl.obj");

	//Scene scn("E:\Models\blender_to_obj\wooden_staircase");


	//coffer
	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\coffe_maker_modify_no_hole.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\coffe_maker_modify_final_copy.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\pbrt_coffe.obj");

	//Scene scn("E:\\Models\\coffe_maker\\original_blend\\Braun_KS_20_Finished_Finale.obj");

	//Scene scn("E:\\Models\\coffe_maker\\coffer\\copy_coffe_maker\\final_good\\coffe_maker_modify_final_good_braun.obj");
              
	//Scene scn("E:\\Models\\country_kitchen\\Country-Kitchen.obj");

	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10.obj");

	//Scene scn("E:\\Models\\Modern_Hall_Fix_Floor\\fix2\\Hall10.obj");

	
	//Scene scn("E:\\Models\\Modern_Hall_Fix_Floor\\Hall10.obj");

	
	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10_fix_normal2.obj");

	//Scene scn("E:\\a_a_Final_Model_Rendering\\bathroom_one_tube\\contemporary_bathroom.obj");
	

	//Modern Hall

	//Scene scn("E:\\Models\\Modern_Hall\\Modern_Hall_Obj\\Hall10.obj");


	//SanMiguel

	//vec3 look_from_scale = fix_camera_scale(scn, look_from);


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
	cout << "v  size: " << scn.v.size()  << "\n";
	cout << "vt size: " << scn.vt.size() << "\n";
	cout << "vn size: " << scn.vn.size() << "\n";
	node* n;

	
	scn.build_bvh(n, 6);

	//scn.delete_before_render();
	//scn.n = n;


	t_build = clock() - t_build;
	cout << "Building time: " << t_build / 1000.0f << "s" << "\n";

	int size = Width * Height;

	vector<vec3> c;
	c.resize(size);
	
	vector<int> sample_count;
	sample_count.resize(size);

	vector<vec3> raw_color;
	raw_color.resize(size);

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


	
	//vector<bool> visible(Width * Height, true);
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
		determine_visibility(n, scn, visible, in_light, num_sample, num_shadow, count, count_light);
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

		cout << "Compute Shadow node order:" << t_shadow / 1000.0f << "s" << "\n";
	}
	*/
	
	clock_t t_render = clock();
	for (int j = 0; j < Height; ++j)
	{
		fprintf(stderr, "\rRendering (%d spp) %5.2f%%", ns, 100.0f * j / (Height - 1));
		#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < Width; ++i)
		{

			//vector<vec2> sample;
			//sample.resize(ns);

			//new_multi_jitter(sample);

			int num_sample_use = 0;

			//int index = j * Width + i + rand() % Width;
			//int sample_so_far = 0;
			bool converge = false;

			vec3 sum(0.0f);

			float sum_sqr = 0.0f;
			float sum_so_far = 0.0f;

			int pixel_index = j * Width + i;

			float convergence_rate;

			//if (visible[pixel_index])
			//	convergence_rate = 0.001f;
			//else
				convergence_rate = 0.0001f;

			
			for (int s = 0; s < ns; s += step)
			{

				//float color = 0.0f;

				//float p_index = i;
				//float q_index = j;
				for (int num = 0; num < step; ++num)
				{

					//float p = ((float)i + sample[s + num].x) * iWidth;
					//float q = ((float)j + sample[s + num].y) * iHeight;

					float p = ((float)i + randf()) * iWidth;
					float q = ((float)j + randf()) * iHeight;

					p = (2.0f * p - 1.0f) * aspect_ratio * tan_theta;
					q = (1.0f - 2.0f * q) * tan_theta;

					Ray new_ray(look_from, (u * p + v * q - w).norm());

					//good
					//vec3 current_color(denan(scn.god_ray_pbrt_test_blur(n, new_ray)));

					//vec3 current_color(denan(scn.god_ray_pbrt_test_blur_2(n, new_ray)));

					//vec3 current_color(denan(scn.god_ray_pbrt(n, new_ray)));
					
					//good
					vec3 current_color(denan(scn.sibenik_tracing_directional_lighting_sun_mirror(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_directional_lighting(n, new_ray)));

					//Material Roughness
					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke_alpha_roughness(n, new_ray)));


					//Sibenik Point Light
					//vec3 current_color(denan(scn.sibenik_tracing_point_light_only(n, new_ray)));

					//victorian house, Dining Room

					//vec3 current_color(denan(scn.sibenik_tracing_directional_lighting(n, new_ray)));


					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke_alpha(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Le(n, new_ray)));
					
					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Ke(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_enviroment(n, new_ray)));

					//vec3 current_color(denan(scn.sibenik_tracing_artificial_light_normal_Le(n, new_ray)));

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
					raw_color[j * Width + i] = sum;
					sample_count[j * Width + i] = num_sample_use;
					converge = true;
					break;
				}
			}

			if (!converge)
			{
				c[j * Width + i] = sum * ins;
				raw_color[j * Width + i] = sum;
				sample_count[j * Width + i] = num_sample_use;
			}

		}
	}
	
	//-----------------------San Miguel---------------------------------

	//string image_name = "SanMiguel_Cam_original_final_Test";

	//string image_name = "SanMiguel_Mirror_Sun_Color_No_Compress_Vertex_change_light_5_direction_RR_6_cam_2_480_270_64spp_3_x2";

	string image_name = "SanMiguel_Final_4096_Bump";// _test_blur_32_0.1_0.95_0.27_s_sun_0.036_a_sun_0.002_env_scale_2_s_0.01_a_0.002";

	ofstream ofs(image_name + ".ppm");

	//ofstream ofs("SanMiguel_test_leaf_Transparent_Shadow.ppm");

	//ofstream ofs("SanMiguel_sah_large.ppm");

	//ofstream ofs("SanMiguel_Transparent_Lighting_fix_alpha_leaf_water_0.9_0.9_0.95_Big.ppm");

	//ofstream ofs("SanMiguel_low_poly_triangles_Back_Face_Culling_Alpha_Leaf.ppm");

	//ofstream ofs("SanMiguel_Compress_Everything_Decompress_64_spp_optimize.ppm");

	//ofstream ofs("SanMiguel_Compress_vt_vn_compress_texture_64spp_sun_color_30_20_10.ppm");

	//ofstream ofs("Kitchen_No_Compress_Texture_hash_v_vt_vn_Compress_vt_vn_speed_test.ppm");

	//ofstream ofs("SanMiguel-Directional-Lighting_64spp_30_20_10_no_compress_texture.ppm");

	//ofstream ofs("Kitchen_Compress_Texture.ppm");

	//ofstream ofs("SanMiguel_Directional_Lighting.ppm");

	//ofstream ofs("Ball_Hash_64.ppm");

	//ofstream ofs("SanMiguel_Low_Poly_Triangle_16.ppm");

	//ofstream ofs("SanMiguel_Low_Poly_X_millions_triangles_texture.ppm");

	//The Final Material Testing
	//ofstream ofs("Rough_Plastic_Tungsten_Color_Pdf.ppm");

	//ofstream ofs("Test_Glass_After_Unify.ppm");

	//ofstream ofs("ThinSheet_compute_Refract_with_Direct_Computation.ppm");

	//Refract vs Refract2
	//ofstream ofs("Modenr_Hall_Refract_vs_Refract2_Glass_Refract2.ppm");

	//Dielectric_Reflectance vs Fresnel_Dielectric
	//ofstream ofs("Modern_Hall_Dielectric_Reflectance_vs_Fresnel_Dielectric_Glass_Dielectric.ppm");


	//ThinSheet

	//ofstream ofs("ThinSheet_original_Refract.ppm");

	//Tungsten_Plastic
	//ofstream ofs("Ball_Plastic_Complex_BSDF_Roughness_no_more_abs_cos.ppm");

	//beckmann space ship
	//ofstream ofs("GGX_Space_Ship.ppm");

	//Material Ball Roughness
	//ofstream ofs("Material_Ball_Rough_Conductor_All_Tungsten_BSDF_H_beckmann_new.ppm");
	//ofstream ofs("Material_Ball_Resume_Final_Rough_Conductor_with_roughness_map.ppm");
	//ofstream ofs("Material_Ball_Roughness_Map_Al_Cu_Cr_256_Simple_Modify_Version.ppm");

	//Mordern Hall
	//ofstream ofs("Modern_Hall_No_More_Crash_128.ppm");
	//ofstream ofs("Xem_thu_cac_mo_hinh_co_crash_hay_khong_kitchen_khong_crash.ppm");

	//Dabrovic Sponza
	//ofstream ofs("Dabrovic_Sponza.ppm");

	//ofstream ofs("a_a_a_a_a_a_Dabrovic_Sponza_No_Bump_Ke_Alpha.ppm");

	//ofstream ofs("test_ke_alpha_Dabrovic_sponza.ppm");

	//Sibenik
	//ofstream ofs("Sibenik_fall_off_Lighting_test_auto_bump_strength_6.6f_final_good.ppm");
	//ofstream ofs("Sibenik_Exponential_Lighting_0.5f.ppm");

	//Dabrovic_Sponza
	//ofstream ofs("Dabrovic_Sponza.ppm");

	//Fix Enviroment Lighting with area light
	//ofstream ofs("Dining_Room_Fix_Directional_Lighting_And_Area_Light_use_area_light_Ke_0.1f.ppm");

	//Good
	//ofstream ofs("True_bread_bump_Phan_Sang_Original_View_1024x1024.ppm");

	//Sibenik With Point Light

	//ofstream ofs("Sibenik_Ke_90_fall_off_Wall_Bump_Phan_Sang_3.6.ppm");

	//ofstream ofs("Sibenik_Ke_90_fall_off_Bump_kamen_algorithm_online.ppm");

	//ofstream ofs("Sibenik_Ke_70_fall_off_move_back_2.0f_bump.ppm");

	//ofstream ofs("Sibenik_bump_Ke_16_use_2_1_final_good_3_32spp.ppm");


	//test Rough Plastic Tungsten version with thickness
	//ofstream ofs("Smooth_Coat_Ball_Rough_Plastic_Diffuse_Thickness_Base_1024.ppm");


	//Plastic Tungsten version
	//ofstream ofs("Smooth_Coat_Ball_Test_Plastic_Diffuse_thickness_version_1_1024.ppm");

	//Smooth Coat Hexa Ball


	//ofstream ofs("Smooth_Coat_Ball_Fix_Black_Area_new_setting_ior_1.1_Add_Enviroment.ppm");

	//ofstream ofs("Smooth_Coat_Ball_Add_Stopping_Conditions_No_Add_Enviroment_1024_True.ppm");

	//ofstream ofs("Smooth_Coat_Hexa_Ball_Light_Screw_Light_Orange_Base_more_blue_and_roughness.ppm");


	//Smooth Coat Bread Bin
	//ofstream ofs("Smooth_Coat_Bread_Fix.ppm");


	//bin build
	//ofstream ofs("Country_Kitchen_Build_Bin_All.ppm");
	//ofstream ofs("Country_Kitchen_sah_full_swept.ppm");
	

	//build bvh no extra triangle space while build
	//ofstream ofs("Country_Kitchen_No_Extra_Space.ppm");

	//Kitchen
	

	//ofstream ofs("Country-Fix_Bread_Bump_final_good_brick_2_128.ppm");

	//ofstream ofs("Country-Fix_Bread_Bump_direct_view.ppm");
	//ofstream ofs("Country-Fix_Bread_Bump_direct_view_bump_plus_0.1_multiply_2_minus_1_normalize_bump.ppm");
	//ofstream ofs("Country-Kitchen_Small_More_Metal_And_Rough_Plastic_oven_more_specular_cooker_black.ppm");

	//blur directional lighting
	//ofstream ofs("Dining_Room_blur_Directional_Lighting_0.02f_Big_Flip_Texture.ppm");

	//salle_de_bain
	//ofstream ofs("salle_de_bain_bright_extended_wall_Final_1024.ppm");

	//Retest Space Ship
	//ofstream ofs("Space_Ship_Light_Diffuse_ReTest_MIS.ppm");

	//Littler Lamp
	//ofstream ofs("Little_Lamp_Rough_Plastic_Diffuse_roughness_0.01_with_specular_0.4f_Metal_0.01.ppm");
	//ofstream ofs("Littler_Lamp_Plastic_Original_roughness_0.15_spec_0.1_Light_2_floor_1.0_Light_Core_no_Big_Light.ppm");


	//Dining Room
	
	//unblur version
	//ofstream ofs("Dining_Room_blur_Directional_Lighting_0.02f_Big_Flip_Texture.ppm");

	//Absolute
	//ofstream ofs("DinningRoom_Tungsten_Rough_Conductor_Absolute.ppm");

	//Victoriran House

	//ofstream ofs("Victorian_House_Remove_blue_line_2_Big_fov_20.6.ppm");

	//ofstream ofs("a_a_a_a_a_a_Space_Ship_Light_Diffuse.ppm");

	//ofstream ofs("a_a_a_a_a_a_Space_Ship_Final_1280_720_64_fov_46.0f.ppm");

	//ofstream ofs("a_a_a_a_a_a_Space_Ship_Red_Cover_Big.ppm");

	//ofstream ofs("a_a_a_a_a_a_Space_Ship_Red_Light_Control_Big.ppm");

	//ofstream ofs("a_a_a_a_a_a_Smooth_Space_Ship_Small.ppm");

	//ofstream ofs("original_space_ship.ppm");

	//ofstream ofs("a_a_a_a_a_a_SpaceShip_Full_Material_Light_less_intensive_Rough_Plastic_Diffuse_Big.ppm");

	//ofstream ofs("a_a_a_a_Bed_Room_Rough_Glass_Full_Fledge_Version_false_false_Fix_roughness_0_pdf_1.0f_64.ppm");

	//ofstream ofs("a_a_a_a_modern_hall_rough_glass_full_fleged_version.ppm");

	//ofstream ofs("a_a_a_a_modern_hall_rough_glass_test_good_version.ppm");

	//ofstream ofs("a_a_Bed_Room_With_Fresnel_Rough_Glass.ppm");

	//ofstream ofs("a_a_modern_Hall_Fresnel_Glass_64.ppm");

	//ofstream ofs("a_a_modern_Hall_Fresnel_Rough_Glass_roughness_0.0_64.ppm");

	//ofstream ofs("a_a_Bed_Room_Rough_Dielectric_version_V.ppm");

	//ofstream ofs("a_a_Rough_Dielectric_Good_Final4_16spp_remap_roughness4_version_2.ppm");

	//ofstream ofs("BathRoom_rough_plastic_original_roughness_0.04_spec_0.4.ppm");

	//ofstream ofs("RoughPlastic_orignal.ppm");

	//ofstream ofs("Glass_BedRoom_true_true_rough_plastic_transmitance_0.8_ior_1.5_roughness_0.0.ppm");

	//ofstream ofs("implement_thinDieletric.ppm");

	//ofstream ofs("Stair_Case_Final_3.0_Ke.ppm");

	//ofstream ofs("Rough_Dielectric.ppm");

	//ofstream ofs("sibenik.ppm");

	//ofstream ofs("BedRoom_With_Mirror.ppm");

	//ofstream ofs("a_a_a_a_Living_Room_Original_Look_Full_Material_Big_forward_0.2f.ppm");

	//ofstream ofs("Living_Room_Full_Material_Close_Up_Look_Big_Size_fov_45.ppm");

	//ofstream ofs("a_a_a_sibenik_tracing_Small_original_look_back_light_2_final_push_root_in_stack.ppm");

	//ofstream ofs("a_living_room_sibenik_tracing_Le_2_old_Fimlmic_only_Diffuse_original_position.ppm");

	//ofstream ofs("a_living_room_original_textures_Gammar.ppm");

	//ofstream ofs("Stair_Case_Big_standard_fov_70.ppm");

	//ofstream ofs("coffe_final_800_1000_1024.ppm");

	//ofstream ofs("coffe_no_hole_Le_4.0f.ppm");

	//ofstream ofs("coffe_pbrt.ppm");

	//ofstream ofs("coffateria_looking_position.ppm");

	//ofstream ofs("Country_Kitchen.ppm");

	//ofstream ofs("Yblien_Rough_Conductor_Tungsten_new_Fresnel.ppm");

	//ofstream ofs("bathroom_Rough_Conductor_Yoan_Blien_Specular_True_Multiply_F.ppm");

	//ofstream ofs("Modern_Hall_2_Rough_Plastic_spec_2.0f.ppm");

	//ofstream ofs("Modern_Hall_2_Rough_Plastic_Fix_Replace_Yoan_Blien_Suck_Rough_Plastic.ppm");

	//ofstream ofs("Modern_Hall_2_Rough_Plastic_Yoan_Blien.ppm");

	//ofstream ofs("Modern_Hall_2.ppm");

	//ofstream ofs("Modern_Hall_Full_New_Material_600_600_1024.ppm");

	//ofstream ofs("Glass_Green_Tint_real_color_6_Test.ppm");

	//ofstream ofs("Glass_Green_Tint_real_color_6_1024.ppm");

	//ofstream ofs("Glass_Beer.ppm");

	//ofstream ofs("Glass_Green_Tint.ppm");

	//ofstream ofs("Modern_Hall_Glass_schlick_new_cos_t_dot_product_version.ppm");

	//ofstream ofs("Modern_Hall_Glass_Schlick.ppm");

	//ofstream ofs("Fire_Room_Test_Glass_Schlick.ppm");

	//ofstream ofs("Fire_Room.ppm");

	//ofstream ofs("original_n.ppm");

	ofs << "P3\n" << Width << " " << Height << "\n255\n";

	string raw_name = image_name + "_" + to_string(ns) + "_raw_.txt";

	//ofstream ofs_raw(raw_name);
	//ofs_raw << "P3\n" << Width << " " << Height << "\n255\n";

	ofstream ofs_log(image_name + "_log.txt");

	clock_t t_write = clock();

	for (int i = 0; i < size; ++i)
	{
		//vec3 color = Filmmic_ToneMapping(c[i]);
		//vec3 raw = raw_color[i];
		//ofs_raw << raw.x << " " << raw.y << " " << raw.z << " " << sample_count[i] << "\n";

		vec3 color = Filmic_unchart2(c[i]);
		//vec3 color = c[i];

		//color = clamp_vector(color, 0.0f, 1.0f);
		color *= 255.99f;

		ofs << color.x << " " << color.y << " " << color.z << "\n";
	}
	
	
	ofs.close();
	
	vector<vec3>().swap(c);
	vector<vec3>().swap(raw_color);
	vector<int>().swap(sample_count);

	delete_bvh(&n);
	scn.delete_struct();

	t_render = clock() - t_render;

	float render_time = ((double)t_render) / CLOCKS_PER_SEC;

	std::cout << "\nRendering time: " << ((double)t_render) / CLOCKS_PER_SEC << "\n";

	t_write = clock() - t_write;

	float write_time = ((double)t_write) / CLOCKS_PER_SEC;
	ofs_log << "Render Time: " << render_time << "\n";
	ofs_log << "Write Time: " << write_time << "\n";

	scn.delete_struct();

	cout << "Delete used data\n";

	//getchar();
	
}

