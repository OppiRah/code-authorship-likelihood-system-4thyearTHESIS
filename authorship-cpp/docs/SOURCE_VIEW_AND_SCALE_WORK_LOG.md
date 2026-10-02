> **Archived 2026-10-02.** Working brief and progress log for the View source
> feature and the Classes/Flagged scale work (Sept 2026), kept as the record
> of those design decisions and test results. Status since: item 3's open
> questions 2 and 3 were resolved by commits 04b761b (student detail exact
> matching) and 2e94431 (countCFiles one subfolder level). Fixes D (drawCard
> cost, which also makes the scroll glide choppy) and E remain deferred. The
> LCS 500-token cap (question 1) remains a documented thesis limitation.

# CALSS — Match Highlighting, Smooth Scroll, and Scale Performance

Copy everything below the line into your coding assistant (Claude Code, pointed at the actual CALSS repo). This covers three separate asks that came up after the "View Source" feature (Flagged Pairs) shipped. They're independent — do them as three separate checkpoints, not one big change, and report back after each before moving to the next.

---

## 1. Highlight matching/copied lines when viewing source from a Flagged Pair

**Decision (already made, don't re-litigate):** highlight the lines that **match or are near-identical between the two compared files**, not a classic differences-style diff. The point is to draw the eye straight to the evidence that caused the flag — "these lines are why this pair got flagged" — not to show unrelated edits.

**Investigate first, then implement:**

- This only applies when "View source" is opened **from a flagged pair** (i.e. we have both `pathA` and `pathB` in hand) — a standalone single-file view (if one exists elsewhere) has nothing to compare against and stays plain.
- Don't reuse or extend the token-similarity pipeline's scoring output (`tokenScore`/`styleScore` are aggregate numbers, not line-level matches) — this needs its own lightweight, display-only line comparison, computed when the modal opens, not during analysis.
- Recommended approach (confirm/refine, don't just assume): reuse the **existing tokenizer normalization** (`tokenizer.cpp` — identifiers/numbers/strings already get normalized to `VAR_0`, `NUM_0`, etc. for the Jaccard/LCS scoring) at the **line level** for this feature: normalize each line of both files the same way, then run a line-based LCS (or similar) over the two normalized line sequences to find which lines correspond, then map matched normalized-line indices back to the real (raw, unnormalized) source lines for display. This means a copied-but-renamed-variables line still highlights as a match, which is the actually-useful case (verbatim matches are the easy case; this catches the "renamed variables to look different" case too). Confirm this reuse is actually straightforward before committing to it — if the existing tokenizer's line/position tracking makes this awkward, say so and propose the next-best approach instead.
- **Layout:** the current source-view modal (`MODAL_SOURCE_VIEW` in `gui.cpp`/`gui_common.h`) is a single fixed-size card showing one file. Compare-mode needs to show two files' worth of content with highlights. Investigate and recommend one of: (a) a wider modal with both files side-by-side, each independently scrollable, matched lines highlighted in both; (b) the current single-file modal with a toggle/tab between File A and File B, each showing its own matches highlighted, plus a way to jump to the next/previous highlighted line. Pick based on what's actually feasible in this GDI/Win32 codebase without a disproportionate rebuild — don't over-engineer a side-by-side view if it's a lot more code for marginal benefit over the toggle approach.
- Highlight color: red background or red text on the matched lines (instructor's own words: "red highlight") — keep it readable against whatever the modal's current background/monospace styling is.
- Same edge cases as the original feature apply here too: a missing/moved file on either side should degrade to the existing fallback message, not crash the diff computation.

### RESOLVED — Checkpoint 1 committed (5cc2448)

Compiles clean (full compile.bat link, zero errors/warnings).

**Approach:** new normalizeSourceLines() added to tokenizer.cpp — runs the existing comment/string/#-stripping (which already preserves line breaks) plus the existing normalizeTokens() per line, restarting VAR_n numbering fresh each line so e.g. `sum = sum + nums[i];` and `total = total + values[k];` match. A line-level LCS pairs up matching lines between the two normalized sequences, in order. No existing tokenizer function changed; scoring untouched.

**Layout: option (b), toggle** — File A / File B tabs in a new header row (click, A/B keys, or Tab). Switching files keeps a visible matched line at the same on-screen height in the other file. Prev / Next (or N/P) jump between matched *blocks*, not individual lines. Scrollbar shows red tick marks at match locations in long files. Highlight style: dark red background, bright red left bar, light text.

**Decisions made, accepted as-is:**
1. Lines under 4 tokens (}, i++;, return 0;) are ignored as match anchors, but still fill in red if they fall inside an already-matched block. Constant: SOURCE_MATCH_MIN_TOKENS. Accepted tradeoff: `int main() {` will highlight in most pairs — treated as acceptable noise rather than raising the cutoff and risking missed real matches.
2. String contents are placeholder-matched (same as scoring) — `printf("Average…", x)` matches `printf("Mean…", y)`. Rare false-positive risk since LCS ordering constrains it, but a message-text change alone won't break the match.
3. Exact-match-only after normalization — a copied line with one token added/removed won't highlight. Accepted for v1; revisit only if instructors report obvious copies not lighting up because of a single inserted line.
4. tokenizer.cpp was touched (new function only, nothing existing changed) — not on the boundary list (which named similarity.cpp, features.cpp, knn_model.cpp, and main.cpp's scoring logic specifically), flagged proactively since it sits next to scoring code. Confirmed fine.

### CLICK-TESTED (see item 3's A+B+C report) — passed, one bug found and fixed

Tested in the real window against a 150-line flagged pair, keys sent directly to the app. N/P jump between matched blocks, B switches to File B lined up with the matching line, three wheel notches move exactly 180px, End/Home work. Red highlights, scrollbar ticks, and the missing-file fallback all display correctly.

**Bug found and fixed (a6cbcf1):** in compare mode, the file path (or the missing-file message) was drawn over the File A/B tabs. Single-file mode was unaffected.

**Still not tested:** mouse clicks on the File A/B tabs and the Prev/Next buttons (only their keyboard shortcuts were exercised), the Tab key itself (the test script's keypresses skip the main message loop where Tab is handled), hover states, and what the glide actually feels like by hand.

## 2. Smooth scrolling in the source-view modal

Currently (per the shipped implementation) WM_MOUSEWHEEL moves the modal's scroll offset directly, line-by-line or in fixed jumps. Make it smooth for long files — the codebase already has an easing pattern used elsewhere for animation (easeOutCubic() in gui_common.h, used for the pair expand/collapse animation over ~240ms). Reuse that same pattern here rather than inventing a new easing curve: animate the scroll offset toward its target over a short duration instead of snapping. Keep keyboard scrolling (Up/Down/PageUp/PageDown/Home/End, already wired) consistent with the same smoothing.

Small, contained change — implement directly, no investigation phase needed.

### RESOLVED — Checkpoint 2 committed (52f3f2d)

Compiles clean (full build links).

**What changed:** mouse wheel, Up/Down, PageUp/PageDown, Home/End, and the checkpoint-1 Prev/Next match jumps all ease to target instead of snapping — 200ms, reusing easeOutCubic() on the same 15ms timer as the pair expand animation (shortened from the expand animation's 240ms deliberately, since that length felt sluggish for scrolling; exposed as one constant, SOURCE_SCROLL_ANIM_MS, if 240ms is ever wanted instead). Rapid input retargets from wherever the glide is currently heading, not from its current position, so fast wheel spinning accumulates smoothly instead of stalling. Step sizes unchanged (60px/notch, 3 lines/arrow press, one page for PageUp/PageDown). Switching File A/B lets any in-progress glide finish first, then swaps instantly — animating across two different files' content was judged to look wrong. Closing the modal mid-glide cancels cleanly since scroll state lives on the modal itself.

### CLICK-TESTED (see item 3's A+B+C report) — works, but choppy — cause identified, not yet fixed

The glide follows the intended easing curve (4 → 12 → 16 → 18px steps observed). But it only gets ~4 frames per 200ms glide in practice: each frame repaints the whole page behind the modal plus the modal's own cards, at 43–69ms/frame. **Root cause is the same drawCard() cost as fix D below** — the glide can't run smoothly until that's addressed, or until the background is cached while the modal is open (an alternative not yet tried). Not changed as part of A+B+C.

**Not tested:** what the glide actually feels like by hand (script can measure frame timing but not "feel").

## 3. Performance at realistic scale — Classes tab, Flagged Pairs tab, general scrolling

**Target scale:** 4 blocks/sections, ~40 students each (160 students total), 600-800+ .c files loaded in one run.

### RESOLVED — Diagnosis complete (see prior revision for full method/table) — DECIDED 2026-09-29: A + B + C approved as one batch, D and E deferred.

### RESOLVED — A+B+C committed (d5778c2)

**Result at target scale:** Classes now paints in **~58ms/frame** (was ~830ms) and switching to it takes **~1.0s** (was ~3.5s, and now the fade animation actually gets enough frames to play). This is short of the ~10-20ms originally projected — the remainder is drawCard() (deferred fix D), which was underestimated in the original diagnosis.

**What changed:**
- **A:** each student's flagged count and highest score are now computed once per analysis run instead of on every paint. Matching switched from substring to **exact name matching** — "LuisDizon" no longer counts toward "Luis".
- **B:** each student's block and the sync manifest are also computed once per run. The student detail page reuses that manifest instead of re-reading it from disk every paint.
- **C:** off-screen cards are no longer drawn, but still get click areas and keyboard focus stops (so tabbing/clicking behavior is unchanged).
- **One addition flagged proactively:** main.cpp gained a one-line studentNameForFile wrapper around extractStudentName, because the GUI's existing name helper doesn't handle legacy filenames like finalsexamCarlos — matching on the raw helper would have broken. Scoring code itself is untouched.

**How it was checked:**

|                               | Before              | After                                          |
|-------------------------------|---------------------|-------------------------------------------------|
| Classes scroll                | ~830ms              | ~58ms                                           |
| Switch to Classes             | ~3.5s (4-5 frames)  | ~1.0s (20 frames — fade plays properly)         |
| One-time cache build per run  | –                   | 16ms                                            |

Same-output check: previous and new builds driven by the same script, 22 screenshots each (Classes at 5 scroll positions, a block tab, "sort by flagged", student detail page, Flagged Pairs, View Source) — pixel-identical except the timestamp in the context strip. Block, flag count, and highest score match for all 160 students across both builds.

**What's left in every frame:** ~20 on-screen cards at ~2ms each in drawCard(). That function is now the dominant cost on every tab — i.e., fix D matters more than originally estimated.

**Checkpoints 1 and 2 click-test results:** folded into their own sections above.

**What was tested:** a script driving the real app window and sending real keypresses — not a human clicking through it by hand.

### Three things flagged for the user to decide (raised 2026-09-29)

1. **Pre-existing scoring bug (tokenizer.cpp's LCS), found incidentally, out of scope to fix here:** the LCS comparison only looks at the first 500 tokens of each file, but the score is computed by dividing by the *full* file's token count. Net effect: identical/near-identical files score lower the longer they are, so long copied submissions can dodge detection entirely.

   | Identical copy, file size | Score                              |
   |----------------------------|-------------------------------------|
   | 40-80 lines                | 100%                                |
   | 120 lines                  | 89% (not labelled Exact Duplicate) |
   | 200 lines                  | 79%                                 |
   | 314 lines                  | 74%, not flagged                    |

   **DECIDED 2026-09-29: no fix.** Left as a known limitation, not scheduled for any checkpoint.

2. **Student detail page still substring-matches.** Fix A switched the Classes-tab card counts to exact name matching, but the student detail page's "Flagged Appearances" and file lists still use the old substring match — so the detail page can now show *more* pairs than the card's (now-correct) count implies. Small, contained fix if wanted (just needs the same swap applied there too). **Not yet decided.**

3. **The countCFiles() "0 files" block-subfolder bug is now confirmed a third time**, and is visibly reproducible in the actual app (context strip shows "0 files" for a block-subfolder dataset) — not just a code-read inference or a profiling-script workaround anymore. **No fix scheduled yet.**

**Remaining deferred fixes, unchanged status:**
- **D (deferred, needs visual sign-off)** — cheaper drawCard() (single shadow pass or cached shadow bitmap). Now confirmed as the dominant cost on every tab *and* the cause of item 2's choppy glide, so it matters more than the original diagnosis suggested. Would need a before/after screenshot for sign-off since it's a visible styling change.
- **E (deferred, low priority)** — cache Flagged Pairs block heights (~3.7ms/frame). Not urgent.

**Next:** waiting on the user for (1) yes/no on fixing the student-detail substring match, (2) whether to schedule the countCFiles() fix, and (3) go-ahead on fix D now that it's shown to matter more than expected.

## Boundaries (all three items)

- No changes to similarity.cpp, features.cpp, knn_model.cpp, or scoring/comparison logic in main.cpp. Item 1's line-matching is a new display-only computation, not a change to how pairs get scored or flagged.
- Keep the three items as separate checkpoints/commits — don't tangle the diff-highlighting change with the performance work, they're unrelated.
- Report compiles-clean status and what was and wasn't actually click-tested for each item, same honesty standard as the last pass — don't claim something was verified interactively if it was only reasoned through statically.
