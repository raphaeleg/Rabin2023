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
	unsigned int iProfileInstances; // # of times ProfileBegin called
	int iOpenProfiles;              // # of times ProfileBegin w/o ProfileEnd
	std::string szName{};               // Name of sample
	float fStartTime;               // The current open profile start time
	float fAccumulator;             // All samples this frame added together
	float fChildrenSampleTime;      // Time taken by all children
	unsigned int iNumParents;       // Number of profile parents
};

struct ProfileSampleHistory {
	std::string szName{}; // Name of the sample
	float fAve;       // Average time per frame (percentage)
	float fMin;       // Minimum time per frame (percentage)
	float fMax;       // Maximum time per frame (percentage)
};
std::vector<ProfileSample> samples;
std::vector <ProfileSampleHistory> history;
float g_startProfile = 0.0f;
float g_endProfile = 0.0f;
std::string textBox = "";
void Profile::Init() noexcept {
	g_startProfile = GetExactTime();
}
void Profile::Begin(std::string_view name) noexcept {
	for (auto& sample : samples) {
		if (name == sample.szName) {
			// Found the sample
			sample.iOpenProfiles++;
			sample.iProfileInstances++;
			sample.fStartTime = GetExactTime();
			assert(sample.iOpenProfiles == 1); // max 1 open at once
			return;
		}
	}

	ProfileSample sample;
	sample.szName = name;
	sample.iOpenProfiles = 1;
	sample.iProfileInstances = 1;
	sample.fAccumulator = 0.0f;
	sample.fStartTime = GetExactTime();
	sample.fChildrenSampleTime = 0.0f;
	samples.push_back(sample);
}
void Profile::End(std::string_view name) noexcept {
	unsigned int numParents = 0;

	for (auto& sample : samples) {
		if (name == sample.szName) { // Found the sample
			unsigned int inner = 0;
			int parent = -1;
			float fEndTime = GetExactTime();
			sample.iOpenProfiles--;

			// Count all parents and find the immediate parent
			for (int inner = 0; inner < samples.size(); inner++) {
				if (samples[inner].iOpenProfiles > 0) { // Found a parent (any open profiles are parents)
					numParents++;
					if (parent < 0) { // Replace invalid parent (index)
						parent = inner;
					}
					else if (samples[inner].fStartTime >= samples[parent].fStartTime) { // Replace with more immediate parent
						parent = inner;
					}
				}
			}

			// Remember the current number of parents of the sample
			sample.iNumParents = numParents;

			if (parent >= 0) { // Record this time in fChildrenSampleTime (add it in)
				samples[parent].fChildrenSampleTime += fEndTime - sample.fStartTime;
			}

			// Save sample time in accumulator
			sample.fAccumulator += fEndTime - sample.fStartTime;
			return;
		}
	}
}
void Profile::DumpOutputToBuffer() noexcept {

	g_endProfile = GetExactTime();

	textBox.clear();

	textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
	textBox.append("--------------------------------------------\n");

	for (auto& sample : samples) {
		unsigned int indent = 0;
		float sampleTime, percentTime, aveTime, minTime, maxTime;
		std::string line, name, indentedName;

		if (sample.iOpenProfiles < 0) {
			assert(!"ProfileEnd() called without a ProfileBegin()");
		}
		else if (sample.iOpenProfiles > 0) {
			assert(!"ProfileBegin() called without a ProfileEnd()");
		}

		sampleTime = sample.fAccumulator - sample.fChildrenSampleTime;
		percentTime = (sampleTime / (g_endProfile - g_startProfile)) * 100.0f;

		aveTime = minTime = maxTime = percentTime;

		// Add new measurement into the history and get the ave, min, and max
		StoreInHistory(sample.szName, percentTime);
		GetFromHistory(sample.szName, &aveTime, &minTime, &maxTime);


		indentedName = sample.szName;
		for (indent = 0; indent < sample.iNumParents; indent++) {
			name = std::format("   {}", indentedName);
			indentedName = name;
		}

		line = std::format("{:3.1f} : {:3.1f} : {:3.1f} : {:>3} : {}\n", aveTime, minTime, maxTime, sample.iProfileInstances,
			indentedName);
		textBox.append(line); // Send the line to text buffer
	}

	{ // Reset samples for next frame
		samples.clear();
		g_startProfile = GetExactTime();
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
		if (name == his.szName) { // Found the sample
			his.fAve = (his.fAve * oldRatio) + (percent * newRatio);
			if (percent < his.fMin) {
				his.fMin = percent;
			}
			else {
				his.fMin = (his.fMin * oldRatio) + (percent * newRatio);
			}

			if (his.fMin < 0.0f) {
				his.fMin = 0.0f;
			}

			if (percent > his.fMax) {
				his.fMax = percent;
			}
			else {
				his.fMax = (his.fMax * oldRatio) + (percent * newRatio);
			}
			return;
		}
	}
	ProfileSampleHistory his;
	his.szName = name;
	his.fAve = his.fMin = his.fMax = percent;
	history.push_back(his);
	
}
void Profile::GetFromHistory(std::string_view name, float* ave, float* min, float* max) noexcept {
	for (auto& his : history) {
		if (name == his.szName) { // Found the sample
			*ave = his.fAve;
			*min = his.fMin;
			*max = his.fMax;
			return;
		}
	}
	*ave = *min = *max = 0.0f;
}
void Profile::Draw() noexcept {
	if (!textBox.empty()) {
		std::cout << textBox << "\n";
	}
}