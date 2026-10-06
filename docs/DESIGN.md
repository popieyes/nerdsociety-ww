# DESIGN — "Dando la Nota"

> **Status: IDEA BANK v0.2, for discussion.** Story and gameplay are **not decided yet**. This doc
> collects options so the team can pick. The scope table and milestone plan are **provisional**
> until an idea is chosen.
> Owner: game-designer. Last update: 2026-10-05.

## 0. What's decided

| Topic | Decision |
|---|---|
| Genre | Rhythm game: you command a Viking crew on a longship through changing weather; a short, funny story told through mechanics, music and scenery |
| Camera | **Side view** (3D scene, camera from the side; readable like Patapon) |
| Platform | Windows desktop = MUST. **Web build = SHOULD** |
| Team | **No composer.** Music must come from existing tracks or be generated in the game (§3) |
| Difficulty | **Deferred** until an idea is chosen |
| Duration | ~1 month jam |

The theme **"Dar la nota"** has three meanings we can use:
1. **To hit the note**, i.e. play in tune and on time.
2. **To make a scene**: to stand out and embarrass yourself in public.
3. **To deliver a note**, read literally ("la nota" = a written message).

---

## 1. Story / theme directions

Each direction is a different answer to "why is this crew playing music at sea?"
They can be mixed, but the strongest jam games usually commit to one.

### D1. The band on tour
*Viking metal band sails to the Great Thing (the big festival) to play the gig of their lives.*
- **Minimal:** 3 legs of the voyage = 3 "rehearsals"; the end card shows the concert.
- **Why fun:** an obvious, warm comedic frame. Bad playing = the band makes a scene ("da la nota").
- **Cost:** low. The story is told through crew barks and an ending card.
- **Risk:** generic, since "band on tour" is a common joke. The music doesn't affect the world, so the weather is just backdrop.

### D2. Charm the sea (diegetic music, the sea listens)
*Rán, the sea goddess, can't stand bad music. Your crew's drumming is literally what keeps the sea calm.*
- **Minimal:** wave height and storm intensity follow your accuracy. On-beat = smooth swells, off-beat
  = choppy chaos. Each weather is Rán's mood (bored → curious → angry → won over).
- **Why fun:** the world visibly reacts to *your* rhythm. Playing well feels like taming a storm.
  Uses the water shader and buoyancy we already have.
- **Cost:** medium. Water/weather parameters must be driven by a "harmony" value (scene-artist).
- **Risk:** feedback loop. Rough sea → harder to read → more misses → rougher sea. Needs damping/caps.

### D3. The silent sea (music = progress)
*A curse stole the music of the North. Each stretch of sea gives back one instrument.*
- **Minimal:** Act 1 = only your drum. Act 2 adds a horn layer, Act 3 a chant, finale = full song.
  Weather brightens as the music returns (fog/grey → sunset → aurora).
- **Why fun:** the soundtrack builds up as you progress, so you can hear your progress. Starts
  minimalist, which is cheap.
- **Cost:** low/medium. Needs layered music (stems or sequenced, §3), which suits our music situation well.
- **Risk:** the early acts may feel empty. They need strong visuals and crew humour.

### D4. Deliver the note (triple meaning)
*A shy Viking wrote a love note ("la nota") to someone on the far side of the sea. The crew must
deliver it, and he has to read/sing it out loud at the end. Will he "dar la nota" (make a scene)?*
- **Minimal:** a bottle/scroll on deck is the "cargo". Lose Dignity → the note gets wet/torn
  (visual). Ending varies with how intact the note is (2–3 ending cards).
- **Why fun:** uses all 3 meanings. Personal, sweet-and-silly stakes. Endings make you want to replay.
- **Cost:** low. A few extra ending cards and a prop.
- **Risk:** romance + Vikings needs a careful comic tone. The music isn't diegetic by default
  (can be combined with D2 or D3).

### D5. Crew drama (the crew are the instruments)
*Every sailor is a musician with a bad habit: one rushes, one drags, one falls asleep. You're the
drummer who has to keep the crew together, or they mutiny.*
- **Minimal:** 3–4 named sailors, each with a quirk that changes their timing. Weathers bring
  out a quirk (seasick in the storm, sleepy in the fog).
- **Why fun:** personality-driven failure is funny ("¡Bjorn, otra vez no!"). Strong characters make a jam game memorable.
- **Cost:** medium. Per-character animations/barks.
- **Risk:** reading several characters' timing at once can be confusing. Must keep it to 1 quirk active at a time.

**Initial take:** **D2 or D3 give the most unique mechanics** (the music changes the world).
**D4 gives the best theme hook** (the triple meaning). D2 + D4 or D3 + D4 combine well: the
world-reaction from one, the story frame from the other. D1 is the safe fallback.

---

## 2. Mechanic menu

Pick one **core** (C1 or C2) and 2–3 **modifiers** (M*). Each entry gives: minimal version /
why it's fun / cost / risk.

### Core input (choose one)

**C1. Patapon 4-beat commands.** 4 drums (BUM / TAK / HUU horn / ¡HEY!); a 4-beat pattern = a
command (ROW, PORT, STARBOARD, BRACE…). The player inputs in one bar, the crew executes in the next.
- Min: 4 commands. / Fun: learning a "language" and the call-and-reply feeling. / Cost: medium. / Risk: memorising patterns adds a learning curve, and a 10-min game needs a good tutorial.

**C2. Row the stroke (port/starboard drums).** Two drums: left = port oars, right = starboard
oars. Every on-beat stroke pushes the boat. Hitting both = straight, hitting one = turn. Steering
*comes out of* the rhythm.
- Min: 2 buttons, 1 hit per beat. / Fun: immediate and physical, like Rhythm Heaven; the boat feels alive. / Cost: low (input) + medium (boat physics tuning). / Risk: may feel shallow without modifiers, and needs more than one input per beat to go deep.

### Modifiers

| # | Idea | Minimal version | Why fun | Cost | Risk |
|---|---|---|---|---|---|
| M1 | **Call & response** | Something plays a 4-beat phrase (sea serpent, ghost ship, storm god, crew); you repeat it | Turns music into a dialogue; natural "boss" | Low | Phrases must be clearly audible, not covered by the music |
| M2 | **Weather changes the rhythm** | Fog: visual beat cues disappear (play by ear). Wind: pushes the beat into swing. Storm: double-time drums | Each act *plays* differently, not just looks different | Medium | Swing/odd timing is hard; tune windows per act |
| M3 | **Weather changes the input** | Waves: a big wave arrives on a beat and you must BRACE exactly on it. Whirlpool: port/starboard swap. Ice: notes must be held | Weather becomes an antagonist with "attacks" | Low–Med | Control swaps can feel unfair; telegraph them for a bar |
| M4 | **Tempo shifts** | The storm accelerates 100 → 130 BPM over a section; calm after the storm slows down | Tension that you can feel physically | Low if the music is sequenced, **high with pre-recorded tracks** | Needs the sequenced-music approach (§3) |
| M5 | **Boat physics on the beat** | Each good stroke = impulse; on-beat strokes build speed, off-beat ones make the boat yaw/wobble. Buoyancy bob in sync | The boat itself is the feedback, so you *see* your timing | Medium (we have buoyancy) | Physics noise can hide the beat; keep it stylised |
| M6 | **Sea reacts to accuracy** (D2) | A "harmony" value (0..1) drives wave amplitude, sky darkness and rain | The world mirrors your skill | Medium | Feedback spiral; clamp it |
| M7 | **Crew personalities** (D5) | 1 quirky sailor per act: Rusher (plays early), Sleeper (needs ¡HEY! to wake), Seasick (misses in waves) | Characters and comedy from systems | Medium | Readability; one quirk at a time |
| M8 | **Music layers as reward** (D3/fever) | Streak of good hits adds a layer (chant, horn); misses remove it | Hearing yourself get better | Low with stems/sequencer | Hard with single-file tracks |
| M9 | **Embarrassment / "make a scene" meter** | Misses trigger a short comic fail (oars clash, someone falls in); meter = Dignity, replaces HP | The theme as a fail state; failing is fun to watch | Low–Med (1 gag per action) | Gags must be short so they don't break the flow |
| M10 | **Fever: "¡SKÁL!"** | 4+ clean bars → crew sings along, faster boat, glowing wake | Classic payoff | Low | None big; keep it score-only |

**Initial take:** C2 (or a slim C1 with 3 commands) + M5 + M1 + one weather modifier per act (M2/M3)
is a small set that is still clearly "ours". M4 and M8 are only cheap if we use the sequenced-music
approach below.

---

## 3. Music without a composer

The game needs music with a **steady, known tempo**. The options:

### Option A: Licensed tracks (royalty-free / CC0 / CC-BY)
Where to look (Viking / Nordic folk / epic drums / sea shanty):
- **OpenGameArt.org**: filter by licence (CC0, CC-BY, CC-BY-SA). Many game loops.
- **incompetech.com** (Kevin MacLeod): CC-BY 4.0. Often lists BPM. Celtic/folk/epic categories.
- **Free Music Archive** (freemusicarchive.org): filter by CC licence. Check each track individually.
- **Pixabay Music**: Pixabay licence (free, no attribution); check its terms against the jam rules.
- **Freesound.org**: drum hits, horn blasts, crowd shouts. Mostly CC0/CC-BY. Best for **samples**, not songs.
- **Kenney.nl** audio packs: CC0 SFX/UI.

**Licence check (every file):** we must be allowed to use it commercially/in a jam game.
**Avoid NC (non-commercial) and ND (no derivatives)** if we cut or loop the track. CC-BY needs
credits in-game. Record author, URL, licence and changes in `resources/CREDITS.md`.

- **Pros:** real, rich music with zero composing.
- **Cons:** tempo may drift (live recordings), intros vary, we must **beat-map each track by
  hand** (BPM + offset). Tempo changes or adaptive layers (M4, M8) are basically impossible. Hard to find 3–5 tracks
  that fit together.

### Option B: Loop/stem packs
Same sources, but searching for **loop packs** (drum loop + melody loops at one BPM).
- **Pros:** fixed BPM by construction; layering (M8, fever) works; easy to loop per act.
- **Cons:** few Viking-flavoured CC packs exist; may sound repetitive.

### Option C: Procedural / sequenced in code
The game **plays its own music**: a small step sequencer triggers samples (drums, horn, chant
syllables, a drone or simple melody) on a beat grid. Samples come from a CC0 pack (Freesound /
OpenGameArt / Kenney), or we synthesize simple drums and drones ourselves.
- **Pros:** **perfect sync by construction**: the sequencer *is* the conductor. Tempo shifts
  (M4), layers (M8), call-and-response phrases (M1) and weather-driven changes (M2) become data, not
  audio editing. Fully diegetic: the crew's drums *are* the music. No licence chasing for whole songs.
- **Cons:** someone has to write patterns (a simple text/array format), and it can sound thin or
  "chiptune-y" if the samples are poor. Mixing many samples needs care (clipping).
  Precise timing requires scheduling on the audio thread (see §4).

### Recommendation: C as the backbone + optional B/A layer
- **The beat (drums, horn, crew chant) is sequenced in code from CC0 samples.** This is the clock
  and the part the player interacts with, so it must be perfect.
- **Optionally** add one melodic/ambient loop per act from a CC-BY/CC0 pack, **re-tempo'd to our
  BPM** in Audacity and started on a bar boundary. If it drifts, it's only atmosphere and doesn't break gameplay.
- Default **one global BPM (~100–110)** and vary intensity by pattern density (double-time
  drums in the storm) rather than BPM. This keeps M4 possible later.
- **Week 1 risk check:** if in-code sequencing sounds bad or proves unreliable (especially on
  web), fall back to Option B with a single-BPM loop pack.

---

## 4. What the code needs from the rhythm system (for `architect`)

These needs hold for every idea above. Keep it a **pure-logic module** (testable without raylib) plus a thin audio layer.

**Conductor / clock**
- `bpm`, `beatsPerBar = 4`, `offsetSec`, `latencyOffsetSec` (user calibration).
- Song position:
  - **Option C:** a sample counter in an `AudioStream` callback (raylib `SetAudioStreamCallback`).
    This is the master clock. The sequencer mixes samples into the buffer at exact sample positions.
  - **Options A/B:** `GetMusicTimePlayed()` smoothed with frame `dt`, handling loop wrap and pause.
- Exposes `songPos`, `beat` (float), `beatIndex`, `barIndex`, `beatInBar`, `beatPhase` (0..1),
  and per-frame `onBeat` / `onBar` edges for visuals. BPM must be changeable at runtime (M4).

**Sequencer (Option C)**
- Pattern data: tracks × steps (e.g. 16 steps/bar) → sample id + volume, per act, in a simple data
  struct. Layers can be switched on/off per bar (M8, fever, weather).
- Simple mixing (sum + soft clip). Thread-safe hand-off from the game thread (e.g. pending-changes
  struct read at bar boundaries).
- Web note: confirm audio callbacks work in the Emscripten build early (web is SHOULD).

**Input judgement**
- Timestamp each press in song time (minus latency). Snap it to the nearest beat (or 8th note if a mechanic
  needs it). Result: `{button, beatIndex, deltaSec, grade}`.
- Windows in a tunables struct (start: Perfect ±50 ms, Good ±100 ms).

**Pattern matching** (C1 commands, M1 call-and-response): a data table of `pattern → id`, an
input-bar/execute-bar state machine, and combo/streak counting.

**Level data:** per act `{bpm, patterns/track, hazards scheduled in beats, weather preset, modifiers}`.
Hazards are scheduled in **beats, not seconds**, so they stay in sync if the tempo changes.

**Tests:** beat ↔ time maths (including BPM change), judgement window boundaries, loop wrap, pattern
matching, sequencer step timing.

---

## 5. Scope (PROVISIONAL, pending idea choice)

| Priority | Gameplay | Art | Audio |
|---|---|---|---|
| **MUST** | Conductor + input + judgement; chosen core (C1 or C2); boat reacts to commands; 1 fail state; 3 acts with distinct weather; title + end screen | Existing longship + water shader; side-view camera; simple crew (capsules + helmets OK); per-act sky/fog/light preset; beat-pulse HUD | Sequenced drum/horn beat from CC0 samples (or a fallback loop pack); hit SFX; credits file |
| **SHOULD** | 2–3 modifiers from §2; call-and-response boss; fever; latency calibration; **web build** | Rain/lightning/fog VFX, transitions between weathers; crew row/bob animation; comic fail gags | Melodic/ambient layer per act; weather ambience; crew shouts |
| **COULD** | Tempo shifts; crew personalities; multiple endings; scoring/ranks; 4th–5th act | Serpent / ghost ship / props; ending cards | Extra layers per streak |
| **WON'T** | Level editor, multiplayer, branching story, online leaderboards, complex physics | Rigged character animation, cinematic cutscenes | Original composed score, full orchestral music |

---

## 6. Milestones (PROVISIONAL, 4 weeks)

| Week | Goal | Exit criteria |
|---|---|---|
| **1. Choose & find the fun** | Day 1–2: pick a direction + core. Clock + sequenced placeholder beat + input + judgement; boat moves on the beat; greybox side view. Check web audio callback | "Playing on the beat feels good." **Go/no-go on core + music approach** |
| **2. Core complete** | Chosen core + fail state + hazards in beats; Act 1 content; first real samples/loops | Act 1 playable start to end by a non-developer |
| **3. Content** | Acts 2–3, weather presets & transitions, 1–2 modifiers, boss/finale, crew art | Full run playable; **content freeze at end of week** |
| **4. Polish & ship** | Juice, menus, calibration, balancing, credits, bug fixes, release build (+ web if it works) | Submitted **2 days before the deadline** |

---

## 7. Open questions

1. **Story direction:** which 1–2 of D1–D5 do we prototype? (Suggestion: D2 or D3, plus D4 as the frame.)
2. **Core input:** Patapon commands (C1) or port/starboard rowing (C2)?
3. **Music approach:** OK with in-code sequencing (§3, option C) as the backbone?
4. **Team & art:** how many people, and who does art (crew models/animations)?
5. **Language:** Spanish, English, or both? (The wordplay works best in Spanish.)
6. **Length:** 3 acts (~6 min) or 5 acts (~10 min)?

*Deferred:* difficulty/forgiveness. We'll decide it once a direction is chosen.
