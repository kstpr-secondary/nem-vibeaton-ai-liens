---
name: skill-creator
description: Create, refine, and organize Agent Skills for early-stage projects that have strong documentation but limited or no implementation history. Use when the task involves turning blueprints, concept documents, MVP plans, milestones, constraints, domain assumptions, architecture notes, or project terminology into reusable skills for planning, decomposition, architecture, subsystem design, domain workflows, or future implementation support, even when there is not yet any code, bug history, or code review data.
compatibility: Portable across heterogeneous agents. Do not assume model-specific behaviors unless explicitly requested.
metadata:
  author: your-org
  version: "1.0"
  project-stage: pre-implementation
  purpose: create-skills-from-project-documents
---

# Early-Project Skill Authoring

Create skills from the project's documented intent, not from invented implementation detail.

This skill exists for projects where the concept is clear but the codebase is immature or absent. The source of truth is the documentation set: blueprint, concept documents, MVP definitions, milestone plans, architecture intentions, constraints, terminology, and expected workflows.

A good early-project skill:
- Extracts reusable procedures and decision rules from actual project documents.
- Preserves distinctions between confirmed facts, intended design, and open assumptions.
- Helps future agents make consistent planning and implementation choices.
- Stays narrow enough to activate precisely.
- Avoids pretending that undocumented implementation details are already settled.

A bad early-project skill:
- Hallucinates concrete code patterns that the project has not chosen yet.
- Writes fake “best practices” with no project grounding.
- Treats assumptions as facts.
- Uses generic role labels without operational guidance.
- Creates implementation-review or bug-fix skills before implementation exists.

## Objective

When asked to create or improve a skill for an early-stage project, produce:
1. A valid, activation-friendly skill.
2. Instructions grounded in project documentation.
3. Clear separation between confirmed decisions and provisional guidance.
4. A skill that remains useful as implementation begins.
5. A path for later evolution once real code, reviews, and failure cases appear.

## Best source material

Prefer source material in this order:
1. Blueprint and architecture documents.
2. Concept documents.
3. MVP definitions and milestone plans.
4. Glossaries, naming conventions, subsystem descriptions, API sketches, data-flow notes, UX flows, and technical constraints.
5. Explicit user clarifications made during the current conversation.

Use only project-specific material when deriving project rules. If a needed detail is missing:
- state it as an assumption,
- make the assumption conservative,
- avoid turning it into a rigid rule unless the user confirms it.

## What skills are appropriate at this stage

Best candidates:
- systems-architecture-review
- milestone-decomposition
- mvp-planning
- subsystem-boundary-design
- rendering-architecture-planning
- gameplay-loop-concept-analysis
- data-model-planning
- ecs-adoption-strategy
- task-breakdown-from-concepts
- technical-risk-review
- library-evaluation-framework
- domain-glossary-enforcement

Usually premature:
- bug-fixing workflows
- code review skills tied to repository conventions
- migration/patch procedures
- debugging runbooks
- performance regression triage
- incident response skills
- narrow implementation-validator skills with no real executable target

## Core principles

### 1. Ground every instruction in documented intent
Every meaningful instruction should come from one of:
- stated architecture,
- stated constraints,
- domain vocabulary,
- milestone/MVP goals,
- explicit non-goals,
- documented subsystem responsibilities,
- explicit future plans in the project documents.

Do not invent detailed internal APIs, file layouts, or coding sequences unless the documents actually define them.

### 2. Distinguish fact, decision, and assumption
When synthesizing a skill, classify important guidance as:
- Confirmed: explicitly stated in project documents.
- Derived: a reasonable synthesis from multiple documented facts.
- Assumed: a conservative placeholder used because the documentation is incomplete.

When assumptions matter, mark them clearly in the skill.

Good:
- “Assume rendering backend abstraction is required because the blueprint targets multiple runtime environments, but backend selection remains open.”

Bad:
- “Use Vulkan with a custom render graph,” unless the documents actually say so.

### 3. Prefer planning and decision support over implementation prescription
At this stage, the most valuable skills help agents:
- decompose concepts into tasks,
- preserve architecture intent,
- map domain language consistently,
- identify risks and unknowns,
- choose among candidate approaches using project criteria,
- define interfaces and responsibilities at a high level.

Be cautious with low-level implementation rules until the project has real examples.

### 4. Encode non-obvious project knowledge
Focus on what a base model would not know:
- the project's terms of art,
- milestone sequencing logic,
- subsystem boundaries,
- integration constraints,
- domain priorities,
- target platforms,
- explicit tradeoffs,
- what counts as MVP vs future scope.

Cut generic advice that is not shaped by the documents.

### 5. Create coherent, narrow skills
A skill should cover one reusable unit of work.

Good:
- `mvp-task-decomposition`
- `rendering-subsystem-planning`
- `entt-adoption-evaluation`
- `sokol-fit-analysis`
- `systems-architecture-review`

Too broad:
- `game-project-expert`
- `engine-and-rendering-and-architecture`
- `all-project-planning`

Too narrow:
- `make-milestone-2-task-7`
- `rename-blueprint-header`

### 6. Use progressive disclosure
Keep `SKILL.md` focused on the guidance needed on most runs.

Move conditional detail into:
- `references/` for document-derived terminology, subsystem maps, milestone matrices, decision records, unresolved questions, library comparisons,
- `assets/` for checklists, templates, matrices, structured output shells,
- `scripts/` only if repeated automation becomes genuinely useful later.

When referencing a file, say exactly when to read it.

Good:
- “Read `references/milestone-map.md` when the user asks for sequencing, dependency ordering, or MVP scoping.”
- “Read `references/domain-glossary.md` when terminology must stay consistent across architecture or task definitions.”

## Workflow

### Step 1: Identify the real job of the requested skill
Classify the requested skill as one of:
- Role skill: architecture reviewer, rendering planner, engine specialist, domain reviewer.
- Domain skill: EnTT, Sokol, ECS design, rendering model, asset workflow, platform abstraction.
- Planning skill: milestone decomposition, MVP scoping, subsystem planning, dependency analysis.
- Translation skill: convert concept docs into task structures, acceptance criteria, or implementation guidance.

Then define:
- primary outcome,
- inputs it expects,
- outputs it should produce,
- what it must not pretend to know yet.

### Step 2: Extract project anchors
From the available documents, extract:
- product goals,
- milestone structure,
- MVP boundaries,
- explicit non-goals,
- technical constraints,
- subsystem candidates,
- terminology and glossary,
- platform targets,
- architecture preferences,
- unresolved questions.

These anchors should shape the skill.

### Step 3: Separate stable guidance from volatile guidance
Stable guidance is safe to encode now:
- domain language,
- project priorities,
- system boundaries,
- milestone logic,
- user-facing goals,
- platform constraints,
- architecture principles,
- evaluation criteria.

Volatile guidance should be weaker or conditional:
- exact APIs,
- exact class structure,
- repository layout,
- build pipelines,
- coding conventions,
- debugging processes,
- review expectations based on real diffs.

### Step 4: Draft the description first
The description is the activation surface. It must say:
- what the skill does,
- when it should be used,
- nearby task wording,
- indirect cues,
- why it matters in a pre-implementation project.

Use imperative phrasing: “Use when…”

Include indirect triggers such as:
- “turn this concept into tasks,”
- “review this architecture direction,”
- “compare libraries for this subsystem,”
- “define MVP boundaries,”
- “plan milestones,”
- “structure a future implementation.”

Avoid writing descriptions that imply real code familiarity when none exists.

### Step 5: Draft `SKILL.md`
Default section order:
1. Objective
2. Scope
3. Project grounding
4. Confirmed facts
5. Assumptions and open questions
6. Core workflow
7. Defaults and decision rules
8. Gotchas
9. Validation
10. File-loading rules
11. Output structure
12. Evolution notes

### Step 6: Add early-stage validation
Because no code may exist yet, validation should focus on document fidelity:
- Did the output stay within the blueprint and concept documents?
- Are assumptions labeled?
- Are MVP and post-MVP work separated?
- Are milestone dependencies consistent?
- Are terms used consistently?
- Did the skill avoid inventing implementation specifics?

### Step 7: Add an evolution path
Every early-project skill should include a short section like:
- “Revisit after first implementation milestone.”
- “Upgrade once code patterns exist.”
- “Add bug-derived gotchas after first defect cycle.”
- “Add review heuristics after first 10 PRs.”
- “Split into implementation and review variants once the codebase matures.”

## Writing rules

### Style
- Be direct.
- Prefer procedures over essays.
- Use numbered steps for workflows.
- Use bullets for constraints and gotchas.
- Avoid generic filler.

### Evidence discipline
Do not blur the line between:
- what the documents explicitly say,
- what you infer,
- what you assume temporarily.

When in doubt, prefer:
- “based on the blueprint, …”
- “the concept docs imply …”
- “assume X unless the user provides a concrete alternative.”

### Defaults
Provide defaults only when they can be justified by:
- project documents,
- explicit constraints,
- platform targets,
- coherent architectural reasoning that stays within the documented concept.

If several approaches are viable:
- pick one provisional default,
- explain why briefly,
- state what future evidence would justify changing it.

### Portability
Assume the resulting skill may be used by Opus, Sonnet, GPT, Gemini, Qwen, and others.
Therefore:
- avoid provider-specific hidden assumptions,
- use plain Markdown,
- write explicit triggers,
- avoid references to private tool semantics unless required,
- keep structure simple and reusable.

## Recommended patterns

### Pattern: document-grounded gotchas
Include only gotchas that arise from the documents, such as:
- The same subsystem has different names across concept docs; normalize them.
- MVP excludes feature X even though milestone 3 discusses it.
- Platform abstraction is required early because future targets are heterogeneous.
- A library choice is still open; do not write implementation as if already decided.

### Pattern: decision frameworks
When a domain choice is still open, encode the evaluation process instead of locking in an answer.

Example:
1. Check project constraints.
2. Check target platforms.
3. Check whether the library supports the intended subsystem boundaries.
4. Check whether it introduces future migration cost.
5. Recommend a default with explicit caveats.

### Pattern: concept-to-task translation
A planning skill can use this workflow:
1. Extract concrete outcomes from the concept document.
2. Separate MVP work from enabling infrastructure and future work.
3. Define tasks as deliverable-producing units.
4. Add dependencies.
5. Add risks and assumptions.
6. Verify every task maps back to documented intent.

### Pattern: architecture review without code
A role skill for systems architecture should review:
- boundary clarity,
- dependency direction,
- future portability,
- milestone feasibility,
- coupling risk,
- fit to concept documents,
- what remains undecided and should stay undecided.

## Anti-patterns

Do not create skills that:
- fake implementation certainty,
- prescribe exact code structures without evidence,
- describe debugging workflows before there is anything to debug,
- turn vague concepts into rigid fake requirements,
- overfit to one model vendor,
- duplicate general software advice with no project grounding,
- collapse MVP and long-term vision into one scope,
- fail to mark assumptions.

## Template: compact early-project skill skeleton

Use this when producing a project-stage skill:

```markdown
***
name: <skill-name>
description: <what it does, when to use it, and indirect cues>
metadata:
  project-stage: pre-implementation
***

# <Skill Title>

## Objective
<What the skill helps the agent accomplish>

## Scope
In scope:
- ...
- ...

Out of scope:
- ...
- ...

## Project grounding
Use these sources of truth:
- blueprint
- concept documents
- MVP definitions
- milestone plans
- explicit constraints

## Confirmed facts
- ...
- ...

## Assumptions and open questions
- Assume ...
- Open question: ...
- Do not lock in ...

## Default workflow
1. ...
2. ...
3. ...

## Decision rules
- Prefer ...
- Avoid ...
- Escalate uncertainty when ...

## Gotchas
- ...
- ...
- ...

## Validation
1. Check fidelity to documents.
2. Check assumption labeling.
3. Check MVP vs future separation.
4. Check terminology consistency.

## File loading
- Read `references/...` when ...
- Read `assets/...` when ...

## Output
Use this structure:
- ...
- ...

## Evolution
After implementation starts, revisit:
- ...
- ...
```

## Output contract for this meta-skill

When the user asks for a new skill, return:
1. Skill type.
2. Why the skill is appropriate for an early-stage project.
3. Proposed directory structure.
4. Full `SKILL.md`.
5. Optional reference or asset files only if justified.
6. A trigger test set:
   - 5 should-trigger prompts,
   - 5 should-not-trigger prompts.
7. A short evolution note describing how the skill should change once implementation begins.

When the user asks to improve an existing early-project skill, return:
1. Scope problems.
2. Grounding problems.
3. Description revisions.
4. Revised `SKILL.md`.
5. Suggested file splits.
6. Trigger tests.
7. Maturity upgrades to add later.

## Final check

Before finishing, verify:
- The skill is grounded in real project documents.
- The description is specific and activation-friendly.
- The scope is coherent.
- The skill does not pretend to know missing implementation details.
- Assumptions are clearly marked.
- MVP and future scope are separated.
- Validation checks fidelity to documentation.
- Extra files are justified.
- The skill remains portable across heterogeneous agents.