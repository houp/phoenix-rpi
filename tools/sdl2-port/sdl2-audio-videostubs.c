/* SPDX-License-Identifier: Zlib
 *
 * sdl2-audio-videostubs — link-only no-op stubs for the audio smoke test.
 *
 * libSDL2.a always contains the phoenix VIDEO driver (src/video/phoenix/), and
 * SDL's core (SDL.c) references SDL_VideoInit unconditionally, so a static link
 * of ANY SDL program pulls the video driver object in. That driver references
 * the in-process Mesa V3D GL entry points (gl*) and the winsys / GL-context glue
 * (phxgl_*, v3d_phoenix_*), which normally live outside libSDL2.a and are
 * provided by the GL glue TU + libGL/libv3d that a GL app links.
 *
 * An AUDIO-only test needs none of that, and must NOT pull in the GL-context
 * glue (which needs Mesa-internal headers/flags) or the Mesa GL libraries. These
 * no-op definitions resolve exactly the video-path symbols the linker names so
 * the audio-only ELF links cleanly with pure zlib code. They are never called:
 * the test opens no window and creates no GL context. Signatures are irrelevant
 * to the static link (symbol-name resolution only); each stub is a bare
 * `void NAME(void)` because the video path is dead code in this binary.
 *
 * Regenerate if the video driver's external surface changes: relink the test
 * against libSDL2.a + -lm alone and add any newly-undefined GL entry points or
 * phxgl_ / v3d_phoenix_ glue symbols here.
 */

void glActiveTexture(void) { }
void glAlphaFunc(void) { }
void glAttachShader(void) { }
void glBegin(void) { }
void glBindAttribLocation(void) { }
void glBindBuffer(void) { }
void glBindFramebuffer(void) { }
void glBindRenderbuffer(void) { }
void glBindTexture(void) { }
void glBlendFunc(void) { }
void glBlitFramebuffer(void) { }
void glBufferData(void) { }
void glBufferSubData(void) { }
void glCheckFramebufferStatus(void) { }
void glClear(void) { }
void glClearColor(void) { }
void glClearDepth(void) { }
void glClientActiveTexture(void) { }
void glColor3f(void) { }
void glColor4f(void) { }
void glColorMask(void) { }
void glColorPointer(void) { }
void glCompileShader(void) { }
void glCreateProgram(void) { }
void glCreateShader(void) { }
void glCullFace(void) { }
void glDeleteBuffers(void) { }
void glDeleteFramebuffers(void) { }
void glDeleteProgram(void) { }
void glDeleteRenderbuffers(void) { }
void glDeleteShader(void) { }
void glDeleteTextures(void) { }
void glDepthFunc(void) { }
void glDepthMask(void) { }
void glDisable(void) { }
void glDisableClientState(void) { }
void glDisableVertexAttribArray(void) { }
void glDrawArrays(void) { }
void glDrawElements(void) { }
void glEnable(void) { }
void glEnableClientState(void) { }
void glEnableVertexAttribArray(void) { }
void glEnd(void) { }
void glFinish(void) { }
void glFlush(void) { }
void glFramebufferRenderbuffer(void) { }
void glFramebufferTexture2D(void) { }
void glFrontFace(void) { }
void glFrustum(void) { }
void glGenBuffers(void) { }
void glGenerateMipmap(void) { }
void glGenFramebuffers(void) { }
void glGenRenderbuffers(void) { }
void glGenTextures(void) { }
void glGetAttribLocation(void) { }
void glGetError(void) { }
void glGetFloatv(void) { }
void glGetIntegerv(void) { }
void glGetProgramInfoLog(void) { }
void glGetProgramiv(void) { }
void glGetShaderInfoLog(void) { }
void glGetShaderiv(void) { }
void glGetString(void) { }
void glGetUniformLocation(void) { }
void glLinkProgram(void) { }
void glLoadIdentity(void) { }
void glLoadMatrixf(void) { }
void glMatrixMode(void) { }
void glMultiTexCoord2f(void) { }
void glOrtho(void) { }
void glPixelStorei(void) { }
void glPopMatrix(void) { }
void glPushMatrix(void) { }
void glReadBuffer(void) { }
void glReadPixels(void) { }
void glRenderbufferStorage(void) { }
void glRotatef(void) { }
void glScalef(void) { }
void glScissor(void) { }
void glShadeModel(void) { }
void glShaderSource(void) { }
void glTexCoord2f(void) { }
void glTexCoordPointer(void) { }
void glTexEnvf(void) { }
void glTexEnvi(void) { }
void glTexImage2D(void) { }
void glTexParameterf(void) { }
void glTexParameteri(void) { }
void glTexSubImage2D(void) { }
void glTranslatef(void) { }
void glUniform1f(void) { }
void glUniform1i(void) { }
void glUniform2f(void) { }
void glUniform3f(void) { }
void glUniform4f(void) { }
void glUniformMatrix4fv(void) { }
void glUseProgram(void) { }
void glVertex2f(void) { }
void glVertex3f(void) { }
void glVertexAttribPointer(void) { }
void glVertexPointer(void) { }
void glViewport(void) { }
void phxgl_bind_fbo(void) { }
void phxgl_init(void) { }
void phxgl_make_current(void) { }
void phxgl_resolve(void) { }
void v3d_phoenix_flip(void) { }
void v3d_phoenix_scanout_active(void) { }
void v3d_phoenix_scanout_init(void) { }
