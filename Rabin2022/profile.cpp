#define _CRT_SECURE_NO_WARNINGS

#include "profile.h"
#include "custom_time.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
struct ProfileSample { // would consider moving structs to another file
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

static float measurementFormula(float x, float oldRatio, float newRatio, float percent) noexcept {
	return (x * oldRatio) + (percent * newRatio);
}
static Measurements StoreInHistory(std::string_view name, float percent) noexcept {
	float newRatio = std::clamp(0.8f * GetElapsedTime(), 0.0f, 1.0f);
	float oldRatio = 1.0f - newRatio;

	auto his = std::ranges::find_if(history,
		[name](const ProfileSampleHistory& h) { return h.name == name; });

	if (his == std::end(history)) {
		history.push_back(ProfileSampleHistory(name, percent, percent, percent));
		return Measurements(percent, percent, percent);
	}

	his->ave = measurementFormula(his->ave, oldRatio, newRatio, percent);
	his->min = std::clamp(measurementFormula(his->min, oldRatio, newRatio, percent), 0.0f, percent);
	his->max = std::clamp(measurementFormula(his->max, oldRatio, newRatio, percent), 0.0f, percent);
	return Measurements(his->ave, his->min, his->max);
}
static std::string addIndentations(std::string_view name, size_t level) noexcept {
	std::string indents("\t", level);
	return indents.append(name);
}
void Profile::Init() noexcept {
	startProfile = GetExactTime();
}
void Profile::Begin(std::string_view name) noexcept {
	auto sample = std::ranges::find_if(samples,
		[name](const ProfileSample& s) { return s.name == name; });

	if (sample == std::end(samples)) {
		samples.push_back(ProfileSample(1, 1, name, GetExactTime(), 0.0f, 0.0f, 0));
		return;
	}
	sample->openProfiles++;// tempted to delete this as vector is cleared before calling Begin()
	sample->instances++;
	sample->startTime = GetExactTime();
	assert(sample->openProfiles == 1);
}
static auto& GetParentIndex() noexcept {
	int parent = -1;
	for (unsigned int inner = 0; inner < samples.size(); inner++) {
		if (samples[inner].openProfiles <= 0) { continue; }
		if (parent < 0) { parent = inner; }
		else if (samples[inner].startTime >= samples[parent].startTime) { parent = inner; }
	}
	return parent;
}
void Profile::End(std::string_view name) noexcept {
	auto sample = std::ranges::find_if(samples,
		[name](const ProfileSample& s) { return s.name == name; });
	if (sample == std::end(samples)) { return; }

	float duration = GetExactTime() - sample->startTime;
	sample->openProfiles--;
	sample->numParents = std::count_if(samples.begin(), samples.end(), [](auto& s) noexcept { return s.openProfiles > 0; });
	sample->accumulator += duration;

	auto p = GetParentIndex();
	if (p >= 0) { // Record this time in fChildrenSampleTime (add it in)
		auto& parent = samples[p];
		parent.childrenSampleTime += duration;
	}
}
static std::string GetSampleDataText(ProfileSample sample) noexcept {
	float sampleTime = sample.accumulator - sample.childrenSampleTime;
	float percentTime = (sampleTime / (endProfile - startProfile)) * 100.0f;
	Measurements measure = StoreInHistory(sample.name, percentTime);
	std::string measureText = std::format("{:3.1f}  : {:3.1f}  : {:3.1f}", measure.ave, measure.min, measure.max);
	std::string indentedName = addIndentations(sample.name, sample.numParents);

	return std::format("{} : {:>3} : {}\n", measureText, sample.instances, indentedName);
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
		textBox.append(GetSampleDataText(sample));
	}
	samples.clear();
	startProfile = GetExactTime();
}
void Profile::Draw() noexcept {
	if (!textBox.empty()) { std::cout << textBox << "\n"; }
}