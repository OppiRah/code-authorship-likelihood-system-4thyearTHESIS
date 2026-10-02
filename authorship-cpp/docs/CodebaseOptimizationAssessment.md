# CALSS Codebase Optimization Assessment

You are acting as a senior C++17 software engineer and codebase reviewer.

You are working on **CALSS**, our C++17 Win32 thesis application for detecting code-authorship anomalies in student C submissions.

Your task in this phase is **NOT to modify the source code**.

Your job is to thoroughly inspect the current repository and determine whether the codebase actually needs optimization, refactoring, restructuring, or cleanup — and, most importantly, determine **what can safely be changed without accidentally changing the application's behavior or thesis results**.

---

## 1. Important Context

CALSS is a thesis tool, so correctness and behavioral stability are more important than making the code look theoretically perfect.

The application performs code-authorship analysis on student C submissions and includes functionality such as:

* submission/student management
* code analysis
* similarity/scoring
* class/submission organization
* flagged-pair results
* GUI presentation
* optional AI-generated summaries using Gemini
* file/configuration handling
* Windows/Win32 GUI functionality

The application is written in **C++17** and uses a Win32-style GUI architecture.

The codebase has already undergone some restructuring.

In particular, the GUI was intentionally split across multiple source files instead of putting the entire GUI implementation into one enormous `.cpp` file.

For example, the repository contains or may contain files conceptually similar to:

* `gui.cpp`
* `gui_overview.cpp`
* `gui_students.cpp`
* other GUI-specific `.cpp`/`.h` files

This separation was intended to make the GUI more manageable and maintainable.

Therefore:

**Do NOT assume that multiple GUI files are unnecessary fragmentation.**

Likewise:

**Do NOT assume that a large `.cpp` file is automatically bad simply because it contains thousands of lines.**

Evaluate the actual responsibilities and dependencies before recommending structural changes.

---

# 2. Primary Goal

Determine:

> **Does CALSS currently need optimization, refactoring, cleanup, architectural restructuring, or performance work — and if so, what should actually be changed?**

I specifically want to distinguish between:

### A. Performance optimization

Changes that make the program execute faster or use fewer resources.

Examples:

* unnecessary repeated computation
* inefficient algorithms
* excessive copying
* unnecessary allocations
* inefficient containers
* repeated file I/O
* redundant parsing
* avoidable string operations
* unnecessarily expensive GUI operations
* expensive operations occurring more often than necessary

### B. Architectural refactoring

Changes that improve how the code is organized without intentionally changing behavior.

Examples:

* separating responsibilities
* reducing coupling
* extracting coherent modules
* improving dependency direction
* removing inappropriate cross-module dependencies
* improving ownership boundaries

### C. Maintainability/readability cleanup

Examples:

* clearer names
* smaller functions where appropriate
* removing duplication
* simplifying complicated logic
* consistent patterns
* improving comments/documentation
* removing obsolete code

### D. Correctness fixes

Actual bugs or unsafe behavior that should be fixed.

These must be clearly separated from ordinary refactoring.

### E. Cosmetic changes

Changes that make the code look cleaner but provide little meaningful engineering benefit.

Do not recommend these merely for the sake of making the code "prettier."

---

# 3. Thesis Safety Is the Highest Priority

Before recommending any modification, consider whether the change could alter the application's behavior.

The following are especially sensitive:

* similarity calculations
* tokenization
* scoring formulas
* pair comparison
* classification/flagging behavior
* confidence calculations
* student/submission association
* result generation
* persisted data
* AI summary integration
* anything affecting thesis experiment results

Do NOT recommend rewriting algorithms simply because another implementation is more elegant.

If an algorithm is correct and sufficiently performant, prefer leaving it alone.

If you find something suspicious, identify it and explain the concern rather than silently changing it.

---

# 4. Inspect the Entire Repository

Before forming conclusions, inspect the actual codebase.

Do not rely on filenames alone.

Determine:

* project structure
* source files
* headers
* build scripts
* resources
* configuration files
* tests
* documentation
* dependencies
* generated files
* entry points
* major subsystems
* important data structures
* major control/data flows

Understand how the application actually works.

Trace important relationships between modules rather than reviewing files in isolation.

---

# 5. Analyze the Architecture

Produce an architectural overview of the current application.

Identify:

* major subsystems
* responsibilities of each subsystem
* dependencies between subsystems
* data flow
* control flow
* GUI/application boundaries
* analysis/scoring boundaries
* file/data boundaries
* external API boundaries
* global/shared state
* ownership relationships

Look for:

* circular dependencies
* inappropriate dependencies
* tight coupling
* duplicated responsibilities
* mixed concerns
* hidden dependencies
* global-state problems
* unclear ownership
* modules that know too much about each other

However, do not label something as bad merely because it does not follow a textbook architecture.

Judge it according to the actual project.

---

# 6. Thoroughly Review the GUI Architecture

This is especially important.

The GUI has already been intentionally divided into multiple source files such as:

* `gui.cpp`
* `gui_overview.cpp`
* `gui_students.cpp`
* etc.

Determine whether this separation is actually beneficial.

For each major GUI module, identify:

* its responsibility
* what state it owns
* what functions it exposes
* what other modules it depends on
* whether it duplicates functionality
* whether it contains unrelated responsibilities
* whether it mixes presentation with application/business logic
* whether its dependencies are reasonable

Specifically investigate whether the current structure represents:

### Good modularization

or

### Arbitrary file splitting

or

### A mixture of both.

Do NOT recommend merging files simply to reduce the number of source files.

Do NOT recommend splitting files simply to make individual files shorter.

The important question is whether the boundaries represent meaningful responsibilities.

---

# 7. Investigate Very Large Files

Some source files may contain approximately 2,000–5,000+ lines.

Do NOT use line count alone as the reason to refactor.

For every unusually large file, determine:

1. Why is it large?
2. What responsibilities does it contain?
3. Are those responsibilities cohesive?
4. Are there obvious logical boundaries?
5. Are functions strongly related?
6. Is there significant duplication?
7. Does the file have excessive coupling?
8. Would splitting it make the code easier to understand?
9. Would splitting it introduce unnecessary complexity?
10. Would splitting it risk breaking behavior?

If splitting is recommended, explain the proposed logical boundaries.

For example:

```text
Current:
large_file.cpp

Potential structure:
large_file.cpp
feature_a.cpp
feature_b.cpp
feature_c.cpp
```

But only recommend this when there is a concrete architectural reason.

---

# 8. Search for Code Duplication

Identify:

* duplicated functions
* duplicated Win32 GUI logic
* repeated validation
* repeated file handling
* repeated string processing
* repeated error handling
* repeated data transformations
* repeated constants
* repeated UI construction patterns

For each meaningful duplication, explain:

* where it occurs
* why it appears duplicated
* whether abstraction would actually improve the code
* whether abstraction could make the code harder to understand

Avoid creating abstractions for tiny pieces of code where the abstraction would be worse than the duplication.

---

# 9. Review C++17 Practices

Review the code for meaningful C++ issues.

Investigate:

* ownership
* raw pointers
* smart pointers
* references
* const correctness
* object lifetime
* RAII
* resource management
* memory management
* copying vs moving
* unnecessary allocations
* string handling
* containers
* exception/error handling
* initialization
* global/static state
* header dependencies
* forward declarations
* include hygiene
* macro usage
* unsafe casts
* Win32 resource handling

Do not recommend modern C++ features merely because they exist.

Only recommend them when they improve correctness, maintainability, safety, or meaningful performance.

---

# 10. Look for Actual Performance Problems

Do not optimize based on intuition alone.

Identify actual potentially expensive operations such as:

* O(n²) or worse algorithms
* repeated full-file processing
* repeated tokenization
* repeated parsing
* unnecessary recomputation
* excessive disk access
* excessive allocations
* unnecessary copying
* inefficient data structures
* redundant API calls
* unnecessary GUI redraw/update operations

For each suspected performance issue, explain:

* what happens
* why it may be expensive
* where it occurs
* approximate complexity where possible
* whether it is likely to matter for CALSS's realistic workload

Distinguish:

**theoretical inefficiency**

from

**a practical performance problem for this application.**

Do not optimize theoretical problems that have no meaningful effect on the actual thesis workload unless the change is exceptionally low-risk.

---

# 11. Identify Dead or Obsolete Code

Search for:

* unused functions
* unused variables
* obsolete implementations
* legacy code paths
* duplicate implementations
* commented-out code
* stale TODOs
* old compatibility paths
* abandoned experiments
* unused includes
* unused resources

Do not delete anything merely because it looks unused.

Verify whether it is actually reachable or required by the build/runtime.

If uncertain, mark it as requiring verification.

---

# 12. Review Error Handling and Resource Safety

Look for:

* ignored return values
* unchecked failures
* file-opening failures
* API failures
* invalid handles
* resource leaks
* memory leaks
* use-after-free risks
* invalid pointers
* incorrect ownership assumptions
* missing cleanup
* silent failures
* inconsistent error handling

Separate confirmed problems from possible concerns.

---

# 13. Review Build and Project Organization

Inspect:

* build scripts
* compiler configuration
* compiler warnings
* warning suppression
* debug/release differences
* dependencies
* generated files
* resource compilation
* output directories
* configuration files

Determine whether the project organization itself creates maintainability problems.

Do not rewrite the build system unless there is a concrete reason.

---

# 14. Review Existing Tests and Verification

Determine what automated or manual verification currently exists.

Identify:

* unit tests
* integration tests
* GUI tests
* analysis/scoring tests
* regression tests
* build verification
* manual testing procedures

Determine which parts of the application are currently protected against regressions.

This is important because future refactoring should not happen blindly.

If important behavior lacks tests, recommend what should be tested **before refactoring**.

---

# 15. Establish a "Do Not Touch" Area

Identify code that should probably remain unchanged unless a real defect is discovered.

Examples may include:

* verified thesis scoring formulas
* algorithms whose behavior is part of experimental validity
* stable low-risk code
* code where the cost/risk of refactoring exceeds its benefit
* code that is ugly but functionally correct and isolated

Explain why each area should remain stable.

---

# 16. Classify Every Recommendation

Every meaningful recommendation should receive one of these classifications:

### SAFE

Low-risk cleanup/refactoring with little likelihood of behavioral change.

### LOW RISK

Reasonably safe, but should still have regression verification.

### MODERATE RISK

Potentially changes internal behavior or has meaningful dependency implications.

### HIGH RISK

Could affect thesis results, application behavior, or major architecture.

### DO NOT RECOMMEND

Changes that would provide little benefit, introduce unnecessary complexity, or create unacceptable risk.

---

# 17. Do NOT Modify the Source Code

This is extremely important.

For this assessment phase:

**DO NOT:**

* rewrite source files
* refactor source files
* rename files
* move files
* delete code
* change algorithms
* change formulas
* change GUI behavior
* change project configuration

You may inspect everything necessary.

The repository must remain functionally and structurally unchanged after this assessment.

If you need to create documentation, that is allowed.

---

# 18. Create the Assessment Document

After thoroughly inspecting the repository, create:

```text
docs/CODEBASE_OPTIMIZATION_ASSESSMENT.md
```

If the `docs` directory or an appropriate documentation structure already exists, follow the existing convention rather than creating unnecessary parallel documentation.

The document should contain:

# CALSS Codebase Optimization Assessment

## 1. Executive Summary

Answer:

* Does CALSS actually need optimization?
* Is the main issue performance, architecture, maintainability, readability, correctness, or some combination?
* What should NOT be optimized?
* What are the highest-value improvements?

Do not simply say "yes, it needs optimization."

Explain what kind.

---

## 2. Current Architecture

Explain the actual architecture discovered in the repository.

Include major modules and their responsibilities.

---

## 3. GUI Architecture Assessment

Specifically analyze the current multi-file GUI structure.

Explain whether the separation into files such as:

```text
gui.cpp
gui_overview.cpp
gui_students.cpp
...
```

is:

* justified
* partially justified
* unnecessarily fragmented
* or in need of further restructuring

Explain why.

---

## 4. Large File Analysis

Create a table similar to:

| File | Approx. Size | Main Responsibilities | Cohesion | Refactor Needed? | Risk |
| ---- | -----------: | --------------------- | -------- | ---------------- | ---- |

Do not recommend splitting files purely based on line count.

---

## 5. Performance Findings

List actual or potential performance problems.

For each:

* location
* issue
* why it matters
* estimated impact
* recommended action
* risk

---

## 6. Code Quality Findings

Include:

* duplication
* complexity
* naming
* ownership
* resource management
* error handling
* C++ practices
* dependency issues
* dead code
* maintainability concerns

---

## 7. Thesis-Critical Areas

Explicitly identify code that should be treated as sensitive because changing it could affect thesis results.

---

## 8. Testing / Regression Protection

Explain what is currently tested and where additional protection is needed before refactoring.

---

## 9. Recommended Improvements

Organize recommendations into:

### Phase 0 — Baseline / Verification

What should be established before changing anything.

### Phase 1 — Safe Cleanup

Low-risk improvements.

### Phase 2 — Controlled Refactoring

Changes that improve architecture or maintainability.

### Phase 3 — Performance Optimization

Only changes that have a meaningful performance justification.

### Phase 4 — Optional Future Improvements

Things that would be nice but are not currently worth the risk/scope.

---

## 10. Do Not Touch

List areas where changing the code would provide little benefit or create unnecessary risk.

---

## 11. Proposed Refactoring Order

If refactoring is recommended, explain the safest order.

For example:

```text
baseline
  ↓
tests/regression protection
  ↓
safe cleanup
  ↓
low-risk modular refactoring
  ↓
performance changes
  ↓
verification
```

Do not assume this exact sequence is correct; determine the appropriate sequence from the actual repository.

---

## 12. Final Recommendation

Give a concise engineering assessment answering:

> "If this were your codebase, what would you actually change before considering CALSS finished?"

This is NOT asking for a score or a subjective ranking.

Give concrete engineering reasoning.

---

# 19. Important Mindset

Do not optimize for:

* fewer lines
* fewer files
* newer C++ features
* more abstractions
* design-pattern usage
* "clean code" aesthetics
* making the architecture look impressive

Optimize for:

* correctness
* maintainability
* understandability
* stability
* appropriate performance
* reasonable architecture
* safe future modification
* thesis reproducibility
* ease of debugging
* ability for the developers to understand the code

Remember that this is a thesis application, not a commercial enterprise system with an unlimited engineering budget.

Avoid overengineering.

---

# 20. Final Constraint

At the end of the task, report:

1. What you inspected.
2. Whether you found actual optimization opportunities.
3. Whether you found architectural/refactoring opportunities.
4. Whether you found correctness/security/resource-management concerns.
5. Whether you found areas that should remain untouched.
6. Where the assessment document was created.
7. Confirm that **no source code was modified** during this assessment.

Do not begin implementing the recommendations yet.

The next phase will be a separate, controlled implementation/refactoring task based on the assessment document.
