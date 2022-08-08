#ifndef _RND_H_
#define _RND_H_
#include <random>

using namespace std;

thread_local uint32_t s_RndState = 1;
static const float imax = 1.0f / UINT32_MAX;
static const float irand_max = 1.0f / RAND_MAX;
float randf()
{
	uint32_t x = s_RndState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 15;
	s_RndState = x;
	return x * imax;
	//return rand() * irand_max;
}


/*
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<>dis(0, 1);

float randf()
{
	return dis(gen);
}
*/
#endif // !_RND_H_

