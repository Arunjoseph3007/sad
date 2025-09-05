#include "Timer.h"

using namespace std::chrono;

Timer::Timer(const char* iden) {
	this->start = high_resolution_clock::now();
	this->iden = iden;
}

Timer::~Timer() {
	high_resolution_clock::time_point end = high_resolution_clock::now();

	long long durationMicro = duration_cast<microseconds>(end - this->start).count();

	printf("[TIMER]: %s finished in %f ms\n", this->iden, (float)durationMicro / 1000);
}