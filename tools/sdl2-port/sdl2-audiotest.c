/* SPDX-License-Identifier: Zlib
 *
 * sdl2-audiotest — Phoenix SDL2 port audio smoke test.
 *
 * Opens the default SDL audio device (the phoenix /dev/audio0 driver on the Pi)
 * at the device-native format (44100 Hz, signed 16-bit, stereo) and plays a
 * ~440 Hz sine tone for ~1 second via a pull-model audio callback, then closes
 * the device. Requesting exactly the device-native spec means SDL builds no
 * conversion stream, so this exercises the driver's OpenDevice -> GetDeviceBuf
 * -> PlayDevice -> CloseDevice path directly.
 *
 * This pass, a successful cross-LINK against libSDL2.a (+ -lm) into an
 * aarch64-phoenix ELF is the milestone (phase-1 step-5). Audible sign-off on the
 * Pi's 3.5 mm jack is a later manual check.
 */
#include <SDL.h>
#include <math.h>

#define TONE_HZ   440.0
#define SAMPLE_HZ 44100
#define CHANNELS  2
#define AMPLITUDE 12000  /* well under INT16_MAX to leave headroom */

static double g_phase; /* radians, carried across callbacks for a continuous tone */

/* Pull-model callback: SDL's audio thread calls this to fill `stream` with
 * `len` bytes of S16 stereo samples; we synthesize a sine and interleave L/R. */
static void fill_tone(void *userdata, Uint8 *stream, int len)
{
    Sint16 *out = (Sint16 *)stream;
    int frames = len / (int)(sizeof(Sint16) * CHANNELS);
    const double step = 2.0 * M_PI * TONE_HZ / (double)SAMPLE_HZ;
    int i;

    (void)userdata;

    for (i = 0; i < frames; i++) {
        Sint16 s = (Sint16)(AMPLITUDE * sin(g_phase));
        out[i * CHANNELS + 0] = s; /* left  */
        out[i * CHANNELS + 1] = s; /* right */
        g_phase += step;
        if (g_phase >= 2.0 * M_PI) {
            g_phase -= 2.0 * M_PI;
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init(AUDIO) failed: %s", SDL_GetError());
        return 1;
    }

    SDL_zero(want);
    want.freq = SAMPLE_HZ;
    want.format = AUDIO_S16SYS;
    want.channels = CHANNELS;
    want.samples = 1024; /* ~23 ms per callback */
    want.callback = fill_tone;
    want.userdata = NULL;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Log("audio open: driver=%s freq=%d format=0x%04x channels=%d samples=%d",
            SDL_GetCurrentAudioDriver(), have.freq, have.format, have.channels, have.samples);

    SDL_PauseAudioDevice(dev, 0); /* start playback */
    SDL_Delay(1000);              /* ~1 s of tone   */
    SDL_PauseAudioDevice(dev, 1);

    SDL_CloseAudioDevice(dev);
    SDL_Quit();

    SDL_Log("audio smoke test done");
    return 0;
}
