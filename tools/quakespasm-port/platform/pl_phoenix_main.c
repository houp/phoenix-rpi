/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2026 Phoenix Systems
 * Author: Witold Bołt
 *
 * Phoenix-RTOS platform backend for QuakeSpasm (QuakeSpasm is Copyright (C) id
 * Software, Inc. and the QuakeSpasm developers, GPL-2.0-or-later). It implements
 * the QuakeSpasm platform interface and is distributed under the same license as
 * the program it is built into; see COPYING in this directory.
 */
/*
 * pl_phoenix_main.c — Phoenix entry point + host loop for Quakespasm: replaces
 * main_sdl.c. No SDL init; just COM_InitArgv -> Sys_Init -> Host_Init, then the
 * Host_Frame loop driven by Sys_DoubleTime.
 *
 * USE_SDL2 build (build-quakespasm-sdl-phoenix.py, the real-SDL variant) ONLY:
 * this file is shared with the proven flagship build, which does NOT define
 * USE_SDL2 and is therefore completely unaffected by every USE_SDL2 block below.
 * SDL_MAIN_HANDLED is defined before any SDL header is pulled in (quakedef.h
 * includes <SDL2/SDL.h> under -DNO_SDL_CONFIG -DUSE_SDL2) so SDL never rewrites
 * our main() to SDL_main. main() then does the global SDL_Init(0) that stock
 * main_sdl.c's Sys_InitSDL did; the compiled-in gl_vidsdl/in_sdl/snd_sdl call
 * SDL_InitSubSystem for their own subsystems as upstream. PL_VID_Shutdown /
 * PL_SetWindowIcon (stock hooks gl_vidsdl.c references, normally in the excluded
 * pl_linux.c) are provided here as no-ops.
 */
#ifdef USE_SDL2
#define SDL_MAIN_HANDLED
#endif

#include "quakedef.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Shareware Quake needs far less than the upstream 256 MB default; use a modest
 * heap that Phoenix can reliably back, and memset it after malloc to force every
 * page committed/mapped up front (the hunk faulted at a low offset during BSP load,
 * i.e. untouched malloc pages weren't mapped). */
#define DEFAULT_MEMORY (96 * 1024 * 1024)

/* The host runs on the MAIN thread (not a pthread). Quake has large stack frames +
 * recursive renderers, so the main-thread stack is enlarged via PT_GNU_STACK in the
 * link (-z stack-size=... in phoenix-rtos-project/_user/rpi4-quake/Makefile). Running
 * on the main thread is deliberate: Mesa's glapi dispatch is TLS and the kernel sets up
 * TLS for the main thread (process.c), whereas GL on a libphoenix pthread faulted in the
 * dispatch (far=0x100030428) — identical GL ran on the main thread fine. */

static quakeparms_t parms;      /* host_parms (the pointer) is owned by host.c */

/* Discovered basedir (set by wait_for_gamedata): "<basedir>/id1/pak0.pak" is the data. */
static const char *g_basedir = "/usr/share/quake";

/* Wait (bounded) for the game data to be reachable before Host_Init, and discover WHERE
 * it lives. The data (id1/pak0.pak) is installed FHS-style under /usr/share/quake on the
 * NFS root (#46). Probe a few standard locations each poll and adopt whichever first
 * exposes id1/pak0.pak. The bounded retry also covers a syspage-launched process racing
 * the nfs-fs takeover (the root briefly resolves to the pre-takeover dummyfs) and the
 * libnfs first-read dircache ENOENT (#156): a later retry succeeds. */
static void wait_for_gamedata(void)
{
	static const char *cands[] = { "/usr/share/quake", "/opt/quake", "/" };
	char path[80];
	int i, c;
	for (i = 0; i < 360; i++) {     /* ~180 s — NFS mount + DHCP can be slow/variable (#156) */
		for (c = 0; c < (int)(sizeof(cands) / sizeof(cands[0])); c++) {
			FILE *f;
			snprintf(path, sizeof(path), "%s/id1/pak0.pak", cands[c]);
			f = fopen(path, "rb");
			if (f) {
				fclose(f);
				g_basedir = cands[c];
				Sys_Printf("quakespasm: found %s after %d tries (basedir=%s)\n",
				           path, i + 1, g_basedir);
				return;
			}
		}
		usleep(500000);
	}
	Sys_Printf("quakespasm: pak0.pak not found after wait (continuing; Host_Init will report)\n");
}

#ifdef USE_SDL2
/* Stock platform hooks gl_vidsdl.c calls (upstream live in pl_linux.c, which this
 * build excludes). No-ops: this port owns no SDL window icon and needs no extra
 * VID teardown beyond gl_vidsdl.c's own SDL_GL_DeleteContext/DestroyWindow. */
void PL_VID_Shutdown(void) {}
void PL_SetWindowIcon(void) {}
#endif

int main(int argc, char *argv[])
{
	double time, oldtime, newtime;

	/* LINE-buffered stdout: each printf line is written to the shared UART console in one
	 * write() instead of per character, so our log lines no longer interleave character-by-
	 * character with the concurrently-running lwip process's output (which made boot messages
	 * like "Initializing QuakeSpasm" unreadable, one fragment per line). stderr stays unbuffered
	 * so crash/wedge diagnostics reach the UART immediately even on an early fault. */
	static char qs_stdout_buf[2048];
	setvbuf(stdout, qs_stdout_buf, _IOLBF, sizeof(qs_stdout_buf));
	setvbuf(stderr, NULL, _IONBF, 0);
	printf("quakespasm: main() entered (argc=%d)\n", argc);

#ifdef QDET_EXECPROBE
	/* exec-cold-open reproduction probe (DET build only; compiled in via -DQDET_EXECPROBE in
	 * build-quakespasm-det.py, NEVER in the shipping rpi4-quake). When /qexec-probe exists on the
	 * root fs, exit immediately at main() entry — BEFORE any GPU/pak init — so distinct copies of
	 * this large binary can be exec'd in a tight loop, stressing the kernel exec metadata force-in
	 * (object_fetchCluster cold `proc_open`) at high frequency to reproduce the intermittent
	 * exec-over-NFS -EIO. Reaching this print means the exec SUCCEEDED (no err=-5); a failed exec
	 * never runs main(). Marker absent => normal DET behaviour (torch/glitch harness). */
	{
		FILE *qmk = fopen("/qexec-probe", "r");
		if (qmk != NULL) {
			fclose(qmk);
			printf("QEXECPROBE ok\n");
			fflush(stdout);
			exit(0);
		}
	}
#endif

	host_parms = &parms;
	parms.basedir = "/usr/share/quake";    /* FHS data dir; wait_for_gamedata() refines it (#46) */
	parms.argc = argc;
	parms.argv = argv;
	parms.errstate = 0;

	COM_InitArgv(parms.argc, parms.argv);

	isDedicated = (COM_CheckParm("-dedicated") != 0);

#ifdef USE_SDL2
	/* Global SDL init, matching stock main_sdl.c's Sys_InitSDL (SDL_Init(0) with no
	 * subsystems; gl_vidsdl/in_sdl/snd_sdl SDL_InitSubSystem their own). SDL_SetMainReady
	 * tells SDL this app owns main() (paired with SDL_MAIN_HANDLED at the top). */
	SDL_SetMainReady();
	if (SDL_Init(0) < 0)
		Sys_Error("Couldn't init SDL: %s", SDL_GetError());
	atexit(SDL_Quit);
#endif

	Sys_Init();

	Sys_Printf("Initializing QuakeSpasm (Phoenix/V3D)\n");

	parms.memsize = DEFAULT_MEMORY;
	parms.membase = malloc(parms.memsize);
	if (!parms.membase)
		Sys_Error("Not enough memory free; check disk space\n");
	/* Touch every page so the whole hunk is committed/mapped (Phoenix does not
	 * demand-page large anonymous malloc: untouched pages translation-fault). */
	memset(parms.membase, 0, parms.memsize);
	Sys_Printf("quakespasm: heap %d MB committed at %p\n",
	           (int)(parms.memsize >> 20), parms.membase);

	wait_for_gamedata();
	parms.basedir = g_basedir;      /* whichever path exposed id1/pak0.pak */

	Sys_Printf("Host_Init\n");
	Host_Init();

	/* Force the classic per-vertex water warp. r_oldwater defaults to 1 in this port
	 * (the modern warpimage path needs glCopyTexSubImage2D, unimplemented on V3D ->
	 * water samples RGB noise), but config.cfg is CVAR_ARCHIVE and a saved one on the
	 * rootfs sets r_oldwater "0" — which execs AFTER our default. Re-assert it here,
	 * after Host_Init has queued the config exec, so it wins regardless of the config. */
	Cbuf_AddText("r_oldwater 1\n");

	/* MP (#68): if id1/phoenix-connect.cfg exists (one line = a dedicated-server
	 * host/IP), connect to it at boot instead of the demo loop. Absent -> the
	 * unchanged attract loop. Lets the netboot harness drive a multiplayer join
	 * to the host server (scripts/quake-mp-server.sh) for diagnosing #68. */
	{
		char cpath[256], line[80];
		FILE *cf;
		snprintf(cpath, sizeof(cpath), "%s/id1/phoenix-connect.cfg", g_basedir);
		cf = fopen(cpath, "r");
		if (cf != NULL) {
			if (fgets(line, sizeof(line), cf) != NULL) {
				int n;
				for (n = 0; line[n] != '\0'; n++) {
					char c = line[n];
					if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
							(c >= 'A' && c <= 'Z') || c == '.' || c == '-')) {
						line[n] = '\0';
						break;
					}
				}
				if (line[0] != '\0') {
					char cmd[96];
					snprintf(cmd, sizeof(cmd), "connect %s\n", line);
					Sys_Printf("PHXNET68: boot connect -> %s\n", line);
					Cbuf_AddText(cmd);
				}
			}
			fclose(cf);
		}
	}

	/* Boot into the attract demo loop (cl_startdemos default = 1 -> demo1.dem, a recorded
	 * E1M3 walkthrough) as the no-input attract mode. Full single-player "map" loading also
	 * works now (server + QuakeC VM + loopback connect, see pl_phoenix_stubs.c net_drivers)
	 * — once /dev/kbd0 input lands, the menu's New Game path is functional. With the MMU
	 * TLB-flush fix the 3D frames render to the V3D, and with the 1MB NFS readmax the pak0
	 * load is faster. (The BSP/lightmap build is still CPU-bound with caches off (TD-16) —
	 * that is the remaining wall for fast 3D load.) */

	oldtime = Sys_DoubleTime();
	while (1) {
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		Host_Frame(time);
		oldtime = newtime;
		usleep(1000);
	}
	return 0;
}
