# Showcase Agent Kickoff Prompts

Use these prompts when handing showcase-ROM work to a coding agent.

Rules for use:
- Grant approval for only one showcase task at a time.
- Do not let the agent skip the task-promotion and milestone-status steps.
- Require the agent to stop after writing the completion summary and moving the
  task file to `showcase/tasks/completed/`.
- Do not let the agent begin the next task or milestone without a new human
  approval message.

Core control files:
- `showcase/docs/current-milestone.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/NN.md`
- `showcase/tasks/README.md`
- `showcase/tasks/planned/NN/SRNN-T01-*.md`

## Generic Prompt Template

```text
Human approval granted to start showcase milestone NN task SRNN-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/NN.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/NN/SRNN-T01-<task-file>.md

Before coding:
1. Re-read the current milestone lock, the milestone NN contract, and the
   SRNN-T01 task file.
2. Promote the task from `showcase/tasks/planned/NN/` to
   `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` status from `planned` to
   `in_progress` if this is the first task in the milestone, or from
   `accepted`/`ready_for_verification` to `in_progress` only if the workflow
   docs explicitly require that transition.
4. Restate the milestone objective, allowed scope, non-goals, and verification
   commands before implementation.

Then implement only the scope of SRNN-T01 and milestone NN.

When finished:
1. Run the milestone NN verification commands, or explain precisely what is
   still missing and why verification cannot complete yet.
2. Append a concrete completion summary to the task markdown.
3. Move the task file to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   the milestone exit criteria are met, otherwise leave an accurate status and
   explain the gap.
5. Stop and wait for human review. Do not start the next task or milestone.
```

## Milestone 0 Kickoff

```text
Human approval granted to start showcase milestone 0 task SR00-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/00.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/00/SR00-T01-bootstrap-showcase-workspace-and-build-wrapper.md

Before coding:
1. Re-read the milestone 0 contract and the SR00-T01 task file.
2. Promote SR00-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` status from `planned` to
   `in_progress`.
4. Restate milestone 0 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 0 / SR00-T01 scope.

When finished:
1. Run the milestone 0 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 1.
```

## Milestone 1 Kickoff

```text
Human approval granted to start showcase milestone 1 task SR01-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/01.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/01/SR01-T01-bring-up-cartridge-loop-and-first-checkpoint.md

Before coding:
1. Re-read the current milestone lock, milestone 1 contract, and SR01-T01.
2. Promote SR01-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `1` and status
   `in_progress` only if milestone 0 has already been accepted.
4. Restate milestone 1 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 1 / SR01-T01 scope.

When finished:
1. Run the milestone 1 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 2.
```

## Milestone 2 Kickoff

```text
Human approval granted to start showcase milestone 2 task SR02-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/02.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/02/SR02-T01-generated-asset-guidance-and-compositing-scenes.md

Before coding:
1. Re-read the current milestone lock, milestone 2 contract, and SR02-T01.
2. Promote SR02-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `2` and status
   `in_progress` only if milestone 1 has already been accepted.
4. Restate milestone 2 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 2 / SR02-T01 scope.

When finished:
1. Run the milestone 2 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 3.
```

## Milestone 3 Kickoff

```text
Human approval granted to start showcase milestone 3 task SR03-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/03.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/03/SR03-T01-tile-and-sprite-verification-scenes.md

Before coding:
1. Re-read the current milestone lock, milestone 3 contract, and SR03-T01.
2. Promote SR03-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `3` and status
   `in_progress` only if milestone 2 has already been accepted.
4. Restate milestone 3 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 3 / SR03-T01 scope.

When finished:
1. Run the milestone 3 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 4.
```

## Milestone 4 Kickoff

```text
Human approval granted to start showcase milestone 4 task SR04-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/04.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/04/SR04-T01-mixed-mode-and-command-engine-scenes.md

Before coding:
1. Re-read the current milestone lock, milestone 4 contract, and SR04-T01.
2. Promote SR04-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `4` and status
   `in_progress` only if milestone 3 has already been accepted.
4. Restate milestone 4 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 4 / SR04-T01 scope.

When finished:
1. Run the milestone 4 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 5.
```

## Milestone 5 Kickoff

```text
Human approval granted to start showcase milestone 5 task SR05-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/05.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/05/SR05-T01-audio-verification-scene-and-checkpoints.md

Before coding:
1. Re-read the current milestone lock, milestone 5 contract, and SR05-T01.
2. Promote SR05-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `5` and status
   `in_progress` only if milestone 4 has already been accepted.
4. Restate milestone 5 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 5 / SR05-T01 scope.

When finished:
1. Run the milestone 5 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start milestone 6.
```

## Milestone 6 Kickoff

```text
Human approval granted to start showcase milestone 6 task SR06-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/06.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/06/SR06-T01-system-validation-regression-manifests-and-freeze.md

Before coding:
1. Re-read the current milestone lock, milestone 6 contract, and SR06-T01.
2. Promote SR06-T01 into `showcase/tasks/active/`.
3. Update `showcase/docs/current-milestone.md` to milestone `6` and status
   `in_progress` only if milestone 5 has already been accepted.
4. Restate milestone 6 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 6 / SR06-T01 scope.

When finished:
1. Run the milestone 6 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start any new task after milestone 6
   without a revised showcase plan.
```

## Milestone 7 Kickoff

```text
Human approval granted to start showcase milestone 7 task SR07-T01.

Follow the showcase workflow in:
/home/djglxxii/src/Vanguard8/showcase/docs/current-milestone.md
/home/djglxxii/src/Vanguard8/showcase/docs/showcase-implementation-plan.md
/home/djglxxii/src/Vanguard8/showcase/docs/milestones/07.md
/home/djglxxii/src/Vanguard8/showcase/tasks/README.md
/home/djglxxii/src/Vanguard8/showcase/tasks/planned/07/SR07-T01-scrolling-starfield-and-graphic6-hud-overlay.md

Before coding:
1. Re-read the current milestone lock, milestone 7 contract, and SR07-T01.
2. Confirm the matching emulator `Graphic 6` compatibility milestone has
   already been accepted.
3. Promote SR07-T01 into `showcase/tasks/active/`.
4. Update `showcase/docs/current-milestone.md` to milestone `7` and status
   `in_progress` only after the emulator dependency and milestone-6 acceptance
   are both satisfied.
5. Restate milestone 7 objective, allowed scope, non-goals, and verification
   commands.

Then implement only milestone 7 / SR07-T01 scope.

When finished:
1. Run the milestone 7 verification commands, or explain precisely what is
   still missing.
2. Append the completion summary to the task markdown.
3. Move the task to `showcase/tasks/completed/`.
4. Update `showcase/docs/current-milestone.md` to `ready_for_verification` if
   appropriate.
5. Stop and wait for human review. Do not start any further showcase task
   without another plan revision.
```
