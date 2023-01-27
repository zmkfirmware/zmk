To build locally, open project in Docker container :
cd /workspaces/zmk/app && west build -d build/left  -p -b nice_nano_v2 -- -DSHIELD=mercury_left
cd /workspaces/zmk/app && west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=mercury_right