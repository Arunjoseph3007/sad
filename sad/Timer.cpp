#include "Timer.h"

Timer::Timer(const char* iden) {
	this->start = std::chrono::system_clock::now();
	this->iden = iden;
}

Timer::~Timer() {
	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

	long long durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - this->start).count();

	printf("[TIMER]: %s exited in %lld ms", this->iden, durationMs);
}