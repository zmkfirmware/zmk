cd app
rm -rf build
# west build -t pristine
# west build -b nice_nano_v2 -- -DSHIELD=corne_left
west build -b nice_nano -- -DSHIELD=corne_left
# west build -b planck_rev6
#  west build -b arm/nrf52840dk/nrf52840 -- -DSHIELD=corne_left

# west build -b nrf52840dk/nrf52840 -- -DSHIELD=nrf52840dk
# //west build -b nrf52840dk/nrf52840 -- -DSHIELD=corne_left
# west build -b seeed/xiao_ble -- -DSHIELD=corne_left
