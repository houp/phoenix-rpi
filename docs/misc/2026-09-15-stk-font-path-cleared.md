# The STK font path is CLEARED — `stk-highbits-pointer` is an allocator bug

*2026-09-15. Source-only review of `FontWithFace::render` and everything it calls, against the
`stk-highbits-pointer` and `freebin-corruption` signatures. Companion to
[`2026-09-12-two-corruption-signatures.md`](2026-09-12-two-corruption-signatures.md) and
[`2026-09-12-stk-highbits-pointer-analysis.md`](2026-09-12-stk-highbits-pointer-analysis.md).*

Line numbers are in the port work tree,
`.buildroot/_build/aarch64a72-generic-rpi4b/port-sources/supertuxkart-1.4/stk-code-1.4/`.

## 1. Patch 0019 cannot have fixed this crash

The faulting `ldrb` is `src/font/font_with_face.cpp:787`, inside the **collect** loop (630–805).
The two loops patch 0019 touches start at **834**. Same-call causation is structurally impossible.

⇒ **The 11 clean STK runs since ports `b5ce28f` are not evidence that this is fixed** (they are
p ≈ 0.31 against the old ~1-in-10 rate anyway). Do not close it on a run count.

The skew 0019 fixes is nevertheless real: `fallback` is sized at the glyph-loop index while
`indices`/`offsets` are compact (the `GLF_NEWLINE` branch `continue`s at `:653` before either
`push_back`), and the pre-patch bounds test was skipped entirely when `fallback[n]` was true, so a
main-bank `sprite_id` indexed the smaller fallback bank through `core::array::operator[]` — which
under `NDEBUG` has **no check at all** (`lib/irrlicht/include/irrArray.h:315-329`).

⚠ **One claim in 0019's own header is wrong and is corrected here.** It says the bad index yields a
wild `video::ITexture*` whose `grab()`/`drop()` do 4-byte read-modify-writes. It does not:
`CGUISpriteBank::getTexture` is bounds-checked and returns `0` out of range
(`lib/irrlicht/source/Irrlicht/CGUISpriteBank.cpp:64-70`). The real exposure is the bogus
`SGUISprite` — a `core::array<SGUISpriteFrame>` with a garbage `data` pointer — whose
`.Frames[0].textureNumber` is an arbitrary **read** (`font_with_face.cpp:866, 955`).
ⓘ Still open, and 0019 does not close it: a `NULL` texture reaches `FontDrawer::addGlyph` →
`texture->getSize()` (`src/font/font_drawer.cpp:92`) with no check — a NULL vptr deref, `far ≈ 0`.

## 2. There is no out-of-bounds 4-byte store anywhere in the render path

Every indexed store reachable from `render()` was checked and is in bounds:

| site | why it is safe |
|---|---|
| `font_drawer.cpp:103-137` `copy_glyph` | memcpy into `g_glyphs[texture]` bounded by the preceding `resize(old_size + stride*4)` |
| `ge_gl_texture.cpp:212-215, 222-227` | writes into a vector sized `size*4`; the ARGB swizzle runs over exactly the locked image |
| `grab()`/`drop()` | `ReferenceCounter` is at **+16** of a virtually-inherited, 8-aligned `IReferenceCounted` (`IReferenceCounted.h:52,161,164`; `ITexture.h:98`) — an 8-aligned store, not "8-aligned **+4**" |
| `font_with_face.cpp:320-321` `insertGlyph` | pushes to the real `Rectangles`/`Sprites`; fonts use a plain `CGUISpriteBank` (`:61`) |

⇒ **Signature A is not a stray font-code write.** If a font-path store produced it, the store was
*legitimate and the buffer was wrong* — i.e. the allocator handed the same memory to two owners.
That also explains the self-consistent bogus `{_M_start,_M_finish}` pair: it is another live object's
coherent 16 bytes, arriving via an aliased allocation.

⚠ The **"RGBA8 pixel" reading of `0x80000001` is unconfirmed.** `SColor black(color.getAlpha(),0,0,0)`
(`font_with_face.cpp:831`) gives `0x80000000` at alpha 128 — nothing in the path produces `b = 1`.

## 3. The `m_cached_gls` dangling-reference theory is dead

`FontWithFace::drawText` and `getDimension` do hold a `std::vector<GlyphLayout>&` into the map across
`shape()` and `render()`, but there is **no invalidation path**: three call sites only
(`font_manager.hpp:141`, `font_manager.cpp:679`, `options_screen_language.cpp:194`), **no `erase`
anywhere**, `shape()` (`font_manager.cpp:265-665`) is self-contained harfbuzz/FreeType, and nothing
`render()` reaches re-enters `getCachedLayouts`. Inserting a new key into a `std::map` does not
invalidate references to existing elements. The `:674` clear gate is additionally off during a
`--profile-laps` run (state is `GAME`).

★ **Worth keeping from this:** because that gate is off in `GAME` while HUD strings change every
frame, `m_cached_gls` **grows unbounded for a whole race**. That is real per-frame heap churn, and it
is a plausible reason STK — and not the Quakes — is where the allocator bug shows up.

## 4. What to do next

No STK-side change is justified. **The fix belongs in the allocator / VM.**

The decisive next step is to make the *next* occurrence conclusive rather than to guess again: extend
the existing 0016 guard (`font_with_face.cpp:763-782`) to

1. re-read `dfv[0]`/`dfv[1]` three or four times through a **`volatile`** pointer — without
   `volatile` the compiler reuses the first load, so a re-read proves nothing. A sane re-read means a
   transient load/TLB fault; a stable bad value means corrupt memory;
2. dump all 88 bytes of `gl[i-1]`, `gl[i]` and `gl[i+1]`, plus `&gl[0]`, `size()`, `capacity()` and
   `i`.

Neighbours also foreign ⇒ the whole buffer is aliased (allocator). Only this element's
post-page-boundary half foreign ⇒ page-level (VM), which matches the 11/11 page-aligned siblings
already recorded.

To settle 0019 empirically, count `STK-FONT-OOB` guard fires against highbits recurrences over N
runs: fires **and** still recurs ⇒ unrelated; never fires ⇒ its bug was not even live in this
workload.
