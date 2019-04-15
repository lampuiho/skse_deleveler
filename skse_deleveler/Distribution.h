#pragma once

float GenGaussRandFloat(float mean, float stdd);

float ComputeNormMean(float max, float min);

unsigned short ComputePoisson(unsigned short lamb);

class ShiftedLogLogistic {
	float loc, scale, shape;

	void SetParamByPercentile(float p1, float p2, float p3);

public:
	ShiftedLogLogistic(float p1, float p2, float p3) { SetParamByPercentile(p1, p2, p3); }

	float operator()();
};
