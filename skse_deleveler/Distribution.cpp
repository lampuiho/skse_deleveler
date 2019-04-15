#include "Distribution.h"

#include <random>
#include <chrono>

static std::default_random_engine generator((unsigned)(std::chrono::system_clock::now().time_since_epoch().count() % (long long)UINT32_MAX));

float GenGaussRandFloat(float mean, float stdd) {
	std::normal_distribution<float> distribution(mean, stdd);
	return distribution(generator);
}

float ComputeNormMean(float max, float min) {
	return (min + max) / 2.f;
}

unsigned short ComputePoisson(unsigned short lamb) {
	std::poisson_distribution<unsigned short> distribution(lamb);
	return distribution(generator);
}

void ShiftedLogLogistic::SetParamByPercentile(float p1, float p2, float p3){
	//(c1^shape-1)=(p1-loc)*shape/scale
	//(c2^shape-1) = (p2-loc)*shape/scale
	//(c3^shape-1) = (p3-loc)*shape/scale
	shape = p3 - p1 + 1;
	loc = p1;
}

float ShiftedLogLogistic::operator()() {
	return loc + (generator() % (unsigned int)shape);
}
