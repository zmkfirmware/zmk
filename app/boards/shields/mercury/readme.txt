from Windows CMD:
cd zmk\app
west build -d build/left  -p -b nice_nano -- -DSHIELD=mercury_left
west build -d build/right -p -b nice_nano -- -DSHIELD=mercury_right

west build -d build/left  -p -b nice_nano_v2 -- -DSHIELD=mercury_left
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=mercury_right