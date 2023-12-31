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
  Profile::Draw(); // output profiling text from last frame
}

int main() {
  bool exitGame = false;
  InitTime();
  Profile::Init();
  while (!exitGame) {
    Profile::Begin("Main Game Loop");
        input();
        update();
        Profile::Begin("Graphics Draw Routine");
            render();
        Profile::End("Graphics Draw Routine");
    Profile::End("Main Game Loop");
    
    Profile::DumpOutputToBuffer(); // buffer will be drawn next frame
  }
  return 0;
}