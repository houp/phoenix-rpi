export const meta = {
  name: 'rpi4-pre-publication-review',
  description: 'Deep pre-publication code review of the Phoenix-RTOS RPi4 port across all repos',
  phases: [
    { title: 'Review', detail: 'one reviewer per repo-subsystem unit' },
    { title: 'Verify', detail: 'adversarially confirm each finding is real' },
  ],
}

// Review units. Each is reviewed by one agent against its diff (base..HEAD for siblings)
// or its source files (coord tools / externals). Grouped to keep each diff deeply reviewable.
const UNITS = [
  // ---- kernel (correctness-critical + Linux-provenance risk) ----
  { id: 'kernel-hal-mem',   repo: 'sources/phoenix-rtos-kernel', paths: 'hal/aarch64/pmap.c hal/aarch64/mmu.h hal/aarch64/_init.S hal/aarch64/spinlock.c hal/aarch64/spinlock.h' },
  { id: 'kernel-hal-core',  repo: 'sources/phoenix-rtos-kernel', paths: 'hal/aarch64/interrupts.c hal/aarch64/timer.c hal/aarch64/hal.c hal/aarch64/cpu.c hal/aarch64/string.c hal/aarch64/exceptions.c' },
  { id: 'kernel-hal-rest',  repo: 'sources/phoenix-rtos-kernel', paths: 'hal/aarch64/generic hal/aarch64/arch' },
  { id: 'kernel-vm-proc',   repo: 'sources/phoenix-rtos-kernel', paths: 'vm proc syspage.c syscalls.c include/arch' },
  // ---- plo (bootloader) ----
  { id: 'plo',              repo: 'sources/plo', paths: 'hal/aarch64 devices/' },
  { id: 'plo-rest',         repo: 'sources/plo', paths: '. ":(exclude)hal/aarch64" ":(exclude)devices"' },
  // ---- devices ----
  { id: 'dev-tty',          repo: 'sources/phoenix-rtos-devices', paths: 'tty' },
  { id: 'dev-misc',         repo: 'sources/phoenix-rtos-devices', paths: 'misc' },
  { id: 'dev-storage',      repo: 'sources/phoenix-rtos-devices', paths: 'storage' },
  { id: 'dev-usb-pcie',     repo: 'sources/phoenix-rtos-devices', paths: 'usb pcie' },
  { id: 'dev-video-gpio-audio-sensors', repo: 'sources/phoenix-rtos-devices', paths: 'video gpio audio sensors' },
  { id: 'dev-targets',      repo: 'sources/phoenix-rtos-devices', paths: '_targets' },
  // ---- networking ----
  { id: 'lwip',             repo: 'sources/phoenix-rtos-lwip', paths: '.' },
  // ---- filesystems (nfs) ----
  { id: 'fs-nfs',           repo: 'sources/phoenix-rtos-filesystems', paths: '.' },
  // ---- libc / userspace libs ----
  { id: 'libphoenix',       repo: 'sources/libphoenix', paths: '.' },
  { id: 'utils',            repo: 'sources/phoenix-rtos-utils', paths: '.' },
  { id: 'usb-stack',        repo: 'sources/phoenix-rtos-usb', paths: '.' },
  { id: 'corelibs-posixsrv',repo: 'sources/phoenix-rtos-corelibs', paths: '.', extraRepo: 'sources/phoenix-rtos-posixsrv' },
  // ---- ports + build + project (build system + boot config) ----
  { id: 'ports',            repo: 'sources/phoenix-rtos-ports', paths: '.' },
  { id: 'build',            repo: 'sources/phoenix-rtos-build', paths: '.' },
  { id: 'project',          repo: 'sources/phoenix-rtos-project', paths: '.' },
  // ---- coordination-repo porting glue (all Phoenix-authored) ----
  { id: 'tools-v3d',        kind: 'files', paths: 'tools/v3d-driver-port' },
  { id: 'tools-quakespasm', kind: 'files', paths: 'tools/quakespasm-port/platform tools/quakespasm-port/sdl-shim tools/quakespasm-port/build-quakespasm-phoenix.py' },
  { id: 'tools-x11-other',  kind: 'files', paths: 'tools/x11-port tools/vkquake-port tools/v3d-shader-tool' },
  { id: 'scripts',          kind: 'files', paths: 'scripts' },
  // ---- external forks ----
  { id: 'ext-mesa-v3d',     kind: 'fork', repo: 'external/mesa', paths: 'src/gallium/drivers/v3d src/broadcom src/mesa/state_tracker' },
  { id: 'ext-quakespasm',   kind: 'fork', repo: 'external/quakespasm', paths: '.' },
]

const FINDING_SCHEMA = {
  type: 'object',
  properties: {
    findings: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          file: { type: 'string' },
          line: { type: 'integer' },
          category: { type: 'string', enum: ['correctness', 'typo', 'duplication', 'hack', 'legal', 'quality'] },
          severity: { type: 'string', enum: ['high', 'medium', 'low'] },
          summary: { type: 'string' },
          detail: { type: 'string' },
          suggested_fix: { type: 'string' },
          confidence: { type: 'string', enum: ['high', 'medium', 'low'] },
        },
        required: ['file', 'category', 'severity', 'summary', 'detail'],
      },
    },
  },
  required: ['findings'],
}

const VERDICT_SCHEMA = {
  type: 'object',
  properties: {
    verdict: { type: 'string', enum: ['CONFIRMED', 'FALSE_POSITIVE', 'UNCERTAIN'] },
    reasoning: { type: 'string' },
    corrected_severity: { type: 'string', enum: ['high', 'medium', 'low'] },
  },
  required: ['verdict', 'reasoning'],
}

function reviewPrompt(u) {
  const siblingDiff = u.kind === 'files'
    ? `These are Phoenix-authored new files in the coordination repo. Review the CURRENT content of the files/dirs: ${u.paths}. Use \`git -C /home/houp/phoenix-rpi log --oneline -5 -- <path>\` for history if useful.`
    : u.kind === 'fork'
      ? `This is a fork of an upstream project (${u.repo}). Review ONLY the Phoenix port changes: run \`git -C /home/houp/phoenix-rpi/${u.repo} log --oneline -20\` to find the port commit(s), then \`git -C /home/houp/phoenix-rpi/${u.repo} show <sha> -- ${u.paths}\` (or diff vs the upstream base). Focus on the Phoenix edits, not upstream code.`
      : `Repo ${u.repo}. Compute the base: \`git -C /home/houp/phoenix-rpi/${u.repo} merge-base HEAD origin/master\`. Review the diff of the LOCAL changes: \`git -C /home/houp/phoenix-rpi/${u.repo} diff <base>..HEAD -- ${u.paths}\`${u.extraRepo ? ` AND also repo ${u.extraRepo} (same base logic, all paths)` : ''}. Read surrounding code with the Read tool as needed for context.`

  return `You are a top-tier OS-kernel code reviewer doing the FINAL pre-publication review of the Phoenix-RTOS Raspberry Pi 4 (BCM2711) port. Phoenix-RTOS is a permissively-licensed (BSD-style) microkernel RTOS. The code will be published publicly. Be HIGHLY CRITICAL and detail-oriented — catch subtle bugs, not just obvious ones. But report only REAL issues; do not pad with style nitpicks.

Review unit: ${u.id}
${siblingDiff}

Look for, in priority order:
1. CORRECTNESS — logic bugs, races/missing locks, memory errors (leaks, use-after-free, overflow), off-by-one, wrong error handling / ignored return values, endianness, alignment, integer overflow/truncation, uninitialized use.
2. LEGAL/PROVENANCE (important) — code copied from GPL / the Linux kernel / other restricted sources into these permissive repos. A local Linux clone is at /home/houp/phoenix-rpi/external/linux and Mesa (MIT) at /home/houp/phoenix-rpi/external/mesa. If a block looks Linux-derived (distinctive identifiers, comments, magic constants, struct layouts), grep the Linux clone (\`grep -rn "<distinctive string>" /home/houp/phoenix-rpi/external/linux\`) to check. Also flag vendored third-party code that is missing its license header/attribution (e.g. FreeBSD teken under tty). Register-definition constants from hardware datasheets are NOT a legal problem; verbatim GPL source is.
3. OBVIOUS ERRORS — typos in code/identifiers, copy-paste mistakes (wrong variable/register reused after a paste), botched merges (duplicate/leftover conflict remnants), dead/unreachable code, wrong constants.
4. DUPLICATION — copy-pasted blocks that should be a shared helper; near-identical functions across files.
5. HACKS/TEMPORARY — leftover diagnostic/probe code, disproved-hypothesis debug code, magic numbers without explanation. NOTE: this project tracks intentional debt with \`TODO(TD-NN):\` markers — those are OK, but flag a hack that has NO marker, or a TD marker whose described condition looks already-resolved/stale.
6. QUALITY — poor names, over-long or noisy comments (this project prefers short comments), over-engineering, inconsistent style vs surrounding upstream code.

For each real issue return a finding with: file (repo-relative path), line (best estimate, integer), category, severity (high=shipping bug or legal exposure; medium=should fix; low=polish), summary (one line), detail (what's wrong + why), suggested_fix (concrete), confidence. Return an empty findings array if the unit is genuinely clean. Do not invent issues.`
}

phase('Review')
const results = await pipeline(
  UNITS,
  (u) => agent(reviewPrompt(u), { label: `review:${u.id}`, phase: 'Review', schema: FINDING_SCHEMA, effort: 'high' })
    .then((r) => ({ unit: u.id, findings: (r && r.findings) || [] })),
  // Verify each finding adversarially, as soon as its unit's review completes.
  (rev) => {
    if (!rev || !rev.findings.length) return { unit: rev ? rev.unit : '?', confirmed: [] }
    return parallel(rev.findings.map((f) => () =>
      agent(`You are an adversarial verifier. A prior reviewer flagged this issue in the Phoenix-RTOS RPi4 port. Your job: determine if it is REAL, not a false positive. Read the actual code (Read/Grep/Bash git tools; repos under /home/houp/phoenix-rpi). Default to FALSE_POSITIVE unless you can confirm the defect from the code.

Repo/unit: ${rev.unit}
Finding: [${f.category}/${f.severity}] ${f.file}:${f.line || '?'} — ${f.summary}
Detail: ${f.detail}
${f.suggested_fix ? 'Suggested fix: ' + f.suggested_fix : ''}

Confirm only if the code actually exhibits the problem. For 'legal' findings, verify against /home/houp/phoenix-rpi/external/linux or /external/mesa before confirming. For 'correctness', trace the concrete failure path. Give a verdict, reasoning, and corrected_severity.`,
      { label: `verify:${rev.unit}:${(f.file||'').split('/').pop()}`, phase: 'Verify', schema: VERDICT_SCHEMA, effort: 'high' })
        .then((v) => ({ ...f, unit: rev.unit, verdict: v && v.verdict, verify_reasoning: v && v.reasoning, corrected_severity: (v && v.corrected_severity) || f.severity }))
        .catch(() => null)
    )).then((verified) => ({ unit: rev.unit, confirmed: verified.filter(Boolean).filter((x) => x.verdict === 'CONFIRMED') }))
  }
)

const all = results.filter(Boolean).flatMap((r) => r.confirmed)
const bySeverity = (s) => all.filter((f) => (f.corrected_severity || f.severity) === s)
log(`Review complete: ${all.length} confirmed findings (high=${bySeverity('high').length} medium=${bySeverity('medium').length} low=${bySeverity('low').length})`)
return { total: all.length, findings: all }
