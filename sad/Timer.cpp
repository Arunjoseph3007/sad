#include "Timer.h"
#include "Common.h"
#include <algorithm>

using namespace std::chrono;

TimerManager GlobalTimerManager;

Timer::Timer(const char* iden) {
	this->start = high_resolution_clock::now();
	this->iden = iden;
}

Timer::~Timer() {
	high_resolution_clock::time_point end = high_resolution_clock::now();
	high_resolution_clock::duration dur = end - this->start;
	long long durationMicro = duration_cast<microseconds>(dur).count();

	printf("[TIMER]: %s finished in %f ms\n", this->iden, (float)durationMicro / 1000);
	GlobalTimerManager.registerInvocation(this->iden, dur);
}


float TimerStats::averageExecutionTime() const {
	return (float)this->totalTime.count() / this->invocations;
}

TimerManager::TimerManager() {
	printf("Timer manager constructor called\n");
}

typedef std::pair<std::string, TimerStats> TimerStatPair;

// used for sorting timer stats in descending order of average execution time
static bool compAverageTime(const TimerStatPair& a, const TimerStatPair& b) {
	return a.second.averageExecutionTime() > b.second.averageExecutionTime();
}

TimerManager::~TimerManager() {
	printf("Timer manager destructor called, printing stats\n");

	std::vector<TimerStatPair> statPairs(this->timerStatsTable.begin(), this->timerStatsTable.end());
	std::sort(statPairs.begin(), statPairs.end(), compAverageTime);

	size_t tableLen = 77;
	repeat(tableLen) { printf("-"); } printf("\n");
	printf("| %30s | %8s | %13s | %13s |\n", "Name", "nTimes", "Total Time", "Average Time");
	repeat(tableLen) { printf("-"); } printf("\n");
	for (const auto& it : statPairs) {
		long long totalTimeMicros = duration_cast<microseconds>(it.second.totalTime).count();
		float avergaeTime = (float)totalTimeMicros / it.second.invocations;
		printf("| %30s | %8zu | %10.3f ms | %10.3f ms |\n",
			it.first.c_str(),
			it.second.invocations,
			(float)totalTimeMicros / 1000,
			(float)avergaeTime / 1000);
	}
	repeat(tableLen) { printf("-"); } printf("\n");
}

void TimerManager::registerInvocation(std::string iden, std::chrono::high_resolution_clock::duration dur) {
	auto it = this->timerStatsTable.find(iden);

	if (it == this->timerStatsTable.end()) {
		this->timerStatsTable[iden] = { .invocations = 1,.totalTime = dur };
	}
	else {
		this->timerStatsTable[iden].invocations++;
		this->timerStatsTable[iden].totalTime += dur;
	}
}