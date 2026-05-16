# Shadow Mapping — Review Findings

## Finding: T-2 — 2026-05-16

- **Source**: code-reviewer
- **Defect class**: weak-acceptance-criteria
- **Suspected owner**: planner/groomer
- **Evidence**: Pipeline creation function existed but was never wired into the init sequence; acceptance criterion ("pipeline logged as created") did not specify the integration call site, so a correct implementation of "create pipeline" did not satisfy "logged as created".
- **Retrospected**: false

## Finding: T-1 — 2026-05-16

- **Source**: code-reviewer
- **Defect class**: ownership-boundary-violation
- **Suspected owner**: implementor-skill
- **Evidence**: Bumped frozen interface version banner (v1.3 → v1.4) without updating the frozen spec document, violating the freeze rule that any post-freeze contract change requires explicit human approval and synchronized spec update.
- **Retrospected**: false
