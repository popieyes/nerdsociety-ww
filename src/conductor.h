#pragma once
#include "raylib.h"

// Rhythm clock. The song position comes from the audio stream (GetMusicTimePlayed) when a music
// track is loaded; otherwise an internal clock advanced by dt stands in, so everything works
// before the soundtrack exists. All beat math below is pure and unit-tested (tests/test_conductor.cpp).

enum class Judgement { Perfect, Good, Miss };

struct TimingWindows {
    float perfect = 0.050f;   // |offset| <= perfect seconds
    float good = 0.120f;      // |offset| <= good seconds
};

struct BeatInfo {
    long index;   // beat number (floor), negative before the first beat
    float phase;  // 0..1 progress through the current beat
};

// --- Pure beat math -------------------------------------------------------------------------
double SecondsPerBeat(float bpm);
double SongTimeToBeats(double songTime, float bpm, float offset);   // offset = time of beat 0
BeatInfo BeatAt(double songTime, float bpm, float offset);
double NearestBeatOffset(double inputTime, float bpm, float offset); // signed seconds, + = late
Judgement JudgeHit(double inputTime, float bpm, float offset, const TimingWindows& windows);
const char* JudgementName(Judgement j);

// --- Clock ------------------------------------------------------------------------------------
struct Conductor {
    float bpm = 120.0f;
    float offset = 0.0f;          // seconds from song start to beat 0
    TimingWindows windows;
    double songTime = 0.0;        // current song position in seconds
    Music music = {};
    bool hasMusic = false;
};

// musicPath may be null or point to a missing file: the internal clock is used then.
void ConductorInit(Conductor& c, float bpm, float offset, const char* musicPath);
void ConductorUpdate(Conductor& c, float dt);
BeatInfo ConductorBeat(const Conductor& c);
Judgement ConductorJudge(const Conductor& c, double inputTime);
void ConductorUnload(Conductor& c);
