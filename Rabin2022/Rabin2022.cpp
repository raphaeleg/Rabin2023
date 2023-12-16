#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "winmm")
#include "custom_time.h"
#include "profile.h"
#include <chrono>
#include <thread>
using namespace std::literals;

void input() {  }
void update() { std::this_thread::sleep_for(200ms); }
void render() {
  std::this_thread::sleep_for(400ms);
  ProfileDraw(); // output profiling text from last frame
}

int main() {
  bool exitGame = false;
  InitTime();
  ProfileInit();
  while (!exitGame) {
    ProfileBegin("Main Game Loop");
        input();
        update();
        ProfileBegin("Graphics Draw Routine");
            render();
        ProfileEnd("Graphics Draw Routine");
    ProfileEnd("Main Game Loop");
    
    ProfileDumpOutputToBuffer(); // buffer will be drawn next frame
  }
  return 0;
}