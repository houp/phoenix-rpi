# Demo web page for the Dillo reel segment

`index.html` is what Dillo fetches in the showcase recording. The Pi's
`/etc/dillo/dillorc` sets both `home=` and `start_page=` to
`http://10.42.0.1:8000/`, which is the development host on the Pi's netboot
network — so a Dillo capture needs a server there, or the browser opens on
nothing and the segment shows an empty window.

Serve it before recording `startx_gpu browse`:

    python3 -m http.server 8000 --bind 10.42.0.1 --directory tools/demo-apps/webroot

Plain HTML on purpose: Dillo's CSS support is limited, and the page has to stay
legible when the 1080p capture is scaled down into the reel.
