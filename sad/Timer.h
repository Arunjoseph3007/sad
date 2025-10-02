#pragma once
#include <chrono>
#include <unordered_map>
#include <string>

class Timer {
private:
	std::chrono::high_resolution_clock::time_point start;
	const char* iden;

public:
	Timer(const char* iden);
	~Timer();
};

struct TimerStats {
	size_t invocations;
	std::chrono::high_resolution_clock::duration totalTime;

	// return average execution time in nanos
	float averageExecutionTime() const;
};

// TODO convert to singleton
class TimerManager {
private:
	std::unordered_map<std::string, TimerStats> timerStatsTable;

public:
	TimerManager();
	~TimerManager();

	void registerInvocation(std::string iden, std::chrono::high_resolution_clock::duration dur);
};

#ifdef _DEBUG

#define TIMEIT() Timer __macro_timer(__FUNCTION__)
#define ITIMEIT(iden) Timer __macro_itimer(iden)

#else // _DEBUG

#define TIMEIT() {}
#define TIMEIT(iden) {}

#endif // _DEBUG