#define _CRT_SECURE_NO_WARNINGS

#include "profile.h"
#include "custom_time.h"
#include <iostream>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <vector>
struct ProfileSample {
	unsigned int instances; // # of times ProfileBegin called
	int openProfiles;              // # of times ProfileBegin w/o ProfileEnd
	std::string name{};               // Name of sample
	float startTime;               // The current open profile start time
	float accumulator;             // All samples this frame added together
	float childrenSampleTime;      // Time taken by all children
	unsigned int numParents;       // Number of profile parents
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
static Measurements GetFromHistory(std::string_view name) noexcept {
	for (auto& his : history) {
		if (name == his.name) { // Found the sample
			return Measurements(his.ave, his.min, his.max);
		}
	}
	return Measurements(0.0f, 0.0f, 0.0f);
}
void Profile::Init() noexcept {
	startProfile = GetExactTime();
}
void Profile::Begin(std::string_view name) noexcept {
	for (auto& sample : samples) {
		if (name == sample.name) {
			// Found the sample
			sample.openProfiles++;
			sample.instances++;
			sample.startTime = GetExactTime();
			assert(sample.openProfiles == 1); // max 1 open at once
			return;
		}
	}

	ProfileSample sample;
	sample.name = name;
	sample.openProfiles = 1;
	sample.instances = 1;
	sample.accumulator = 0.0f;
	sample.startTime = GetExactTime();
	sample.childrenSampleTime = 0.0f;
	samples.push_back(sample);
}
void Profile::End(std::string_view name) noexcept {
	unsigned int numParents = 0;

	for (auto& sample : samples) {
		if (name == sample.name) { // Found the sample
			unsigned int inner = 0;
			int parent = -1;
			float fEndTime = GetExactTime();
			sample.openProfiles--;

			// Count all parents and find the immediate parent
			for (int inner = 0; inner < samples.size(); inner++) {
				if (samples[inner].openProfiles > 0) { // Found a parent (any open profiles are parents)
					numParents++;
					if (parent < 0) { // Replace invalid parent (index)
						parent = inner;
					}
					else if (samples[inner].startTime >= samples[parent].startTime) { // Replace with more immediate parent
						parent = inner;
					}
				}
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
}
void Profile::DumpOutputToBuffer() noexcept {

	endProfile = GetExactTime();

	textBox.clear();

	textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
	textBox.append("--------------------------------------------\n");

	for (auto& sample : samples) {
		unsigned int indent = 0;
		float sampleTime, percentTime, aveTime, minTime, maxTime;
		std::string line, name, indentedName;

		if (sample.openProfiles < 0) {
			assert(!"ProfileEnd() called without a ProfileBegin()");
		}
		else if (sample.openProfiles > 0) {
			assert(!"ProfileBegin() called without a ProfileEnd()");
		}

		sampleTime = sample.accumulator - sample.childrenSampleTime;
		percentTime = (sampleTime / (endProfile - startProfile)) * 100.0f;

		aveTime = minTime = maxTime = percentTime;

		// Add new measurement into the history and get the ave, min, and max
		StoreInHistory(sample.name, percentTime);
		Measurements measure = GetFromHistory(sample.name);


		indentedName = sample.name;
		for (indent = 0; indent < sample.numParents; indent++) {
			name = std::format("   {}", indentedName);
			indentedName = name;
		}

		line = std::format("{:3.1f} : {:3.1f} : {:3.1f} : {:>3} : {}\n", measure.ave, measure.min, measure.max, sample.instances,
			indentedName);
		textBox.append(line); // Send the line to text buffer
	}

	{ // Reset samples for next frame
		samples.clear();
		startProfile = GetExactTime();
	}
}
void Profile::StoreInHistory(std::string_view name, float percent) noexcept {
	float oldRatio;
	float newRatio = 0.8f * GetElapsedTime();
	if (newRatio > 1.0f) {
		newRatio = 1.0f;
	}
	oldRatio = 1.0f - newRatio;

	for (auto& his : history) {
		if (name == his.name) { // Found the sample
			his.ave = (his.ave * oldRatio) + (percent * newRatio);
			if (percent < his.min) {
				his.min = percent;
			}
			else {
				his.min = (his.min * oldRatio) + (percent * newRatio);
			}

			if (his.min < 0.0f) {
				his.min = 0.0f;
			}

			if (percent > his.max) {
				his.max = percent;
			}
			else {
				his.max = (his.max * oldRatio) + (percent * newRatio);
			}
			return;
		}
	}
	history.push_back(ProfileSampleHistory(name, percent, percent, percent));

}
void Profile::Draw() noexcept {
	if (!textBox.empty()) {
		std::cout << textBox << "\n";
	}
}