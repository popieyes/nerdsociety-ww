#include "conductor.h"

#include <cmath>

double SecondsPerBeat(float bpm)
{
    return 60.0 / (double)bpm;
}

double SongTimeToBeats(double songTime, float bpm, float offset)
{
    return (songTime - (double)offset) / SecondsPerBeat(bpm);
}

BeatInfo BeatAt(double songTime, float bpm, float offset)
{
    const double beats = SongTimeToBeats(songTime, bpm, offset);
    const double whole = std::floor(beats);
    return { (long)whole, (float)(beats - whole) };
}

double NearestBeatOffset(double inputTime, float bpm, float offset)
{
    const double beats = SongTimeToBeats(inputTime, bpm, offset);
    return (beats - std::round(beats)) * SecondsPerBeat(bpm);
}

Judgement JudgeHit(double inputTime, float bpm, float offset, const TimingWindows& windows)
{
    const double err = std::fabs(NearestBeatOffset(inputTime, bpm, offset));
    if (err <= windows.perfect) return Judgement::Perfect;
    if (err <= windows.good) return Judgement::Good;
    return Judgement::Miss;
}

const char* JudgementName(Judgement j)
{
    switch (j) {
        case Judgement::Perfect: return "PERFECT";
        case Judgement::Good: return "GOOD";
        case Judgement::Miss: return "MISS";
    }
    return "?";
}

void ConductorInit(Conductor& c, float bpm, float offset, const char* musicPath)
{
    c = Conductor{};
    c.bpm = bpm;
    c.offset = offset;
    if (musicPath && IsAudioDeviceReady() && FileExists(musicPath)) {
        c.music = LoadMusicStream(musicPath);
        c.hasMusic = IsMusicValid(c.music);
        if (c.hasMusic) PlayMusicStream(c.music);
    }
}

void ConductorUpdate(Conductor& c, float dt)
{
    if (c.hasMusic) {
        UpdateMusicStream(c.music);
        c.songTime = GetMusicTimePlayed(c.music);
    } else {
        c.songTime += dt;
    }
}

BeatInfo ConductorBeat(const Conductor& c)
{
    return BeatAt(c.songTime, c.bpm, c.offset);
}

Judgement ConductorJudge(const Conductor& c, double inputTime)
{
    return JudgeHit(inputTime, c.bpm, c.offset, c.windows);
}

void ConductorUnload(Conductor& c)
{
    if (c.hasMusic) UnloadMusicStream(c.music);
    c = Conductor{};
}
