# Renderer SpecKit Plan

## Problem statement

Define a renderer MVP plan that matches the blueprint and produces a frozen renderer interface usable by the engine workstream.

## Required outputs

- Renderer architecture summary
- Frozen renderer interface spec
- Renderer task breakdown by milestone
- Quickstart/runbook

## Must-preserve decisions

- OpenGL 3.3 Core only
- `sokol_app` owned by renderer
- `sokol-shdc` build-time shader pipeline
- Magenta fallback pipeline on shader/pipeline creation failure
- Procedural mesh builders owned by renderer

## Open questions

- Final public header split
- Handle naming conventions
- Mock surface shape for downstream consumers
