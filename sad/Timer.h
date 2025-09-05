#pragma once
#include <chrono>

class Timer {
private:
	std::chrono::system_clock::time_point start;
	const char* iden;

public:
	Timer(const char* iden);
	~Timer();
};

#ifdef _DEBUG

#define TIMEIT() Timer __macro_timer(__FUNCTION__)
#define ITIMEIT(iden) Timer __macro_itimer(iden)

#else // _DEBUG

#define TIMEIT() {}
#define TIMEIT(iden) {}

#endif // _DEBUG