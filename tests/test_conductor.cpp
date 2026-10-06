#include "check.h"
#include "conductor.h"

void TestConductor()
{
    // 120 BPM -> 0.5 s per beat
    CHECK_NEAR(SecondsPerBeat(120.0f), 0.5, 1e-12);
    CHECK_NEAR(SongTimeToBeats(1.0, 120.0f, 0.0f), 2.0, 1e-12);
    CHECK_NEAR(SongTimeToBeats(1.0, 120.0f, 0.25f), 1.5, 1e-12);

    // Beat index + phase
    BeatInfo b = BeatAt(1.25, 120.0f, 0.0f);
    CHECK(b.index == 2);
    CHECK_NEAR(b.phase, 0.5, 1e-6);

    b = BeatAt(0.0, 120.0f, 0.0f);
    CHECK(b.index == 0);
    CHECK_NEAR(b.phase, 0.0, 1e-6);

    // Before beat 0 (offset in the future): negative index, phase still in [0,1)
    b = BeatAt(0.1, 120.0f, 0.35f);   // -0.5 beats
    CHECK(b.index == -1);
    CHECK_NEAR(b.phase, 0.5, 1e-6);

    // Signed offset to the nearest beat: + late, - early
    CHECK_NEAR(NearestBeatOffset(1.03, 120.0f, 0.0f), 0.03, 1e-9);
    CHECK_NEAR(NearestBeatOffset(0.97, 120.0f, 0.0f), -0.03, 1e-9);
    CHECK_NEAR(NearestBeatOffset(2.0, 100.0f, 0.2f), 0.0, 1e-6);   // beat 3 at 0.2 + 3*0.6 = 2.0

    // Judgement windows
    const TimingWindows w;   // perfect 50 ms, good 120 ms
    CHECK(JudgeHit(1.00, 120.0f, 0.0f, w) == Judgement::Perfect);
    CHECK(JudgeHit(1.04, 120.0f, 0.0f, w) == Judgement::Perfect);
    CHECK(JudgeHit(0.92, 120.0f, 0.0f, w) == Judgement::Good);
    CHECK(JudgeHit(1.10, 120.0f, 0.0f, w) == Judgement::Good);
    CHECK(JudgeHit(1.25, 120.0f, 0.0f, w) == Judgement::Miss);   // exactly between beats
    CHECK(JudgeHit(1.15, 120.0f, 0.0f, w) == Judgement::Miss);

    // Internal clock (no music, no audio device needed)
    Conductor c;
    ConductorInit(c, 120.0f, 0.0f, nullptr);
    CHECK(!c.hasMusic);
    for (int i = 0; i < 60; ++i) ConductorUpdate(c, 1.0f / 60.0f);
    CHECK_NEAR(c.songTime, 1.0, 1e-5);
    CHECK(ConductorBeat(c).index == 2 || ConductorBeat(c).index == 1);   // float accumulation at the edge
    CHECK(ConductorJudge(c, c.songTime) == Judgement::Perfect);
}
