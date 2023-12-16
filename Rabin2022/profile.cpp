#define _CRT_SECURE_NO_WARNINGS

#include "profile.h"
#include "custom_time.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
struct ProfileSample {
	unsigned int instances; // # of times ProfileBegin called
	int openProfiles;              // # of times ProfileBegin w/o ProfileEnd
	std::string name{};               // Name of sample
	float startTime;               // The current open profile start time
	float accumulator;             // All samples this frame added together
	float childrenSampleTime;      // Time taken by all children
	unsigned int numParents;       // Number of profile parents
	ProfileSample(unsigned int i, int op, std::string_view n, float st, float a, float cst, unsigned int np) :
		instances(i), openProfiles(op), name(n), startTime(st), accumulator(a), childrenSampleTime(cst), numParents(np)
	{}
};
struct ProfileSampleHistory {
	std::string name{}; // Name of the sample
	float ave;       // Average time per frame (percentage)
	float min;       // Minimum time per frame (percentage)
	float max;       // Maximum time per frame (percentage)
	ProfileSampleHistory(std::string_view n, float a, float mn, float mx) : name(n), ave(a), min(mn), max(mx) {}
};
struct Measurements {
	float ave;       // Average time per frame (percentage)
	float min;       // Minimum time per frame (percentage)
	float max;       // Maximum time per frame (percentage)
	Measurements(float a, float mn, float mx) : ave(a), min(mn), max(mx) {}
};

std::vector<ProfileSample> samples;
std::vector <ProfileSampleHistory> history;
float startProfile = 0.0f;
float endProfile = 0.0f;
std::string textBox = "";

static Measurements StoreInHistory(std::string_view name, float percent) noexcept {
	float newRatio = std::clamp(0.8f * GetElapsedTime(), 0.0f, 1.0f);
	float oldRatio = 1.0f - newRatio;

	for (auto& his : history) {
		if (name != his.name) { continue; }
		his.ave = (his.ave * oldRatio) + (percent * newRatio);
		his.min = std::clamp((his.min * oldRatio) + (percent * newRatio), 0.0f, percent);
		his.max = std::clamp((his.max * oldRatio) + (percent * newRatio), 0.0f, percent);
		return Measurements(his.ave, his.min, his.max);
	}
	history.push_back(ProfileSampleHistory(name, percent, percent, percent));
	return Measurements(percent, percent, percent);
}
void Profile::Init() noexcept {
	startProfile = GetExactTime();
}
void Profile::Begin(std::string_view name) noexcept {
	for (auto& sample : samples) {
		if (name != sample.name) { continue; }
		sample.openProfiles++;
		sample.instances++;
		sample.startTime = GetExactTime();
		assert(sample.openProfiles == 1);
		return;
	}
	samples.push_back(ProfileSample(1,1,name,GetExactTime(),0.0f,0.0f,0));
}
void Profile::End(std::string_view name) noexcept {
	unsigned int numParents = 0;

	for (auto& sample : samples) {
		if (name != sample.name) { continue; }
		unsigned int inner = 0;
		int parent = -1;
		float fEndTime = GetExactTime();
		sample.openProfiles--;

		// Count all parents and find the immediate parent
		for (int inner = 0; inner < samples.size(); inner++) {
			if (samples[inner].openProfiles <= 0) { continue; }
			numParents++;
			if (parent < 0) { parent = inner; }
			else if (samples[inner].startTime >= samples[parent].startTime) { parent = inner; }
		}

		// Remember the current number of parents of the sample
		sample.numParents = numParents;

		if (parent >= 0) { // Record this time in childrenSampleTime (add it in)
			samples[parent].childrenSampleTime += fEndTime - sample.startTime;
		}

		// Save sample time in accumulator
		sample.accumulator += fEndTime - sample.startTime;
		return;
		
	}
}
void Profile::DumpOutputToBuffer() noexcept {
	endProfile = GetExactTime();

	textBox.clear();

	textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
	textBox.append("--------------------------------------------\n");

	for (auto& sample : samples) {
		if (sample.openProfiles < 0) {
			assert(!"ProfileEnd() called without a ProfileBegin()");
		}
		else if (sample.openProfiles > 0) {
			assert(!"ProfileBegin() called without a ProfileEnd()");
		}

		float sampleTime = sample.accumulator - sample.childrenSampleTime;
		float percentTime = (sampleTime / (endProfile - startProfile)) * 100.0f;
		Measurements measure = StoreInHistory(sample.name, percentTime);

		std::string line = std::format("{:3.1f} : {:3.1f} : {:3.1f} : {:>3} :    {}\n", measure.ave, measure.min, measure.max, sample.instances, sample.name);
		textBox.append(line);
	}

	samples.clear();
	startProfile = GetExactTime();
}
void Profile::Draw() noexcept {
	if (!textBox.empty()) { std::cout << textBox << "\n"; }
}