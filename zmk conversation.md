***You***: Skip to main content
ZMK Logo
ZMK Firmware
Docs
Blog
Tools

Pre-Release

ZMK Studio

    Getting Started

Supported Hardware
FAQs
Installing ZMK
Customizing ZMK
ZMK CLI
Troubleshooting
Features
Keymaps
Configuration

    Development

You're viewing the documentation for the development version of ZMK. You may want the latest release v0.3.
Supported Hardware

With the solid technical foundation of Zephyr™ RTOS, ZMK can support a wide diversity of hardware targets, including but not limited to Nordic nRF52, Raspberry Pi RP2040/RP2350, most ST STM32 MCUs, and Microchip SAMD21. That being said, there are specific boards / shields that have been implemented and tested by the ZMK contributors, listed below.
Onboard Controller Keyboards

Keyboards with onboard controllers are single PCBs that contain all the components of a keyboard, including the controller chip, switch footprints, etc.

    Advantage 360 Pro (Boards: adv360pro_left, adv360pro_right)
    BDN9 (Rev2) (Board: bdn9)
    BT60 V1 Hotswap (Board: bt60_hs)
    BT60 V2 (Board: bt60)
    BT65 (Board: bt65)
    BT75 V1 (Board: bt75)
    Corneish Zen (Boards: corneish_zen_left, corneish_zen_right)
    Ferris 0.2 (Board: ferris)
    Glove80 (Boards: glove80_lh, glove80_rh)
    KBDfans Tofu65 2.0 (Board: tofu65)
    nice!60 (Board: nice60)
    Planck (Rev6) (Board: planck)
    Preonic Rev3 (Board: preonic)
    S40NC (Board: s40nc)

Composite Keyboards

Composite keyboards are composed of two main PCBs: a small controller board with exposed pads, and a larger keyboard PCB (a shield, in ZMK lingo) with switch footprints and a location where the controller is added. This location is called an interconnect. Multiple interconnects can be found below.
Pro Micro Interconnect

The SparkFun Pro Micro grew popular as a low cost ATmega32U4 board with sufficient GPIO and peripherals to work for many keyboard needs. Since the original Pro Micro, many pin compatible boards have appeared, with various changes or improvements, such as the Elite-C w/ USB-C, nice!nano with nRF52840 wireless. Note: ZMK doesn't support boards with AVR 8-bit processors, such as the ATmega32U4, because Zephyr™ only supports 32-bit and 64-bit platforms. As a result, controllers like the SparkFun Pro Micro and the Elite-C are *not* supported by ZMK.
Boards

    Adafruit KB2040 (Board: adafruit_kb2040)
    BoardSource blok (Board: boardsource_blok)
    nRFMicro (nRF52833) (Board: nrfmicro/nrf52833)
    nRFMicro nRF52840 (flipped) (Board: nrfmicro/nrf52840/flipped)
    nRFMicro (nRF52840) 1.1/1.2/1.3 (Board: nrfmicro/nrf52840)
    BlueMicro840 v1 (Board: bluemicro840)
    Puchi-BLE V1 (Board: puchi_ble)
    nice!nano (Board: nice_nano)
    QMK Proton-C (Board: proton_c)
    SparkFun Pro Micro RP2040 (Board: sparkfun_pro_micro_rp2040)
    Mikoto (Board: mikoto)

Shields

    A. Dux (Shields: a_dux_left, a_dux_right)
    BAT43 (Shield: bat43)
    BFO-9000 (Shields: bfo9000_left, bfo9000_right)
    Boardsource 3x4 Macropad (Shield: boardsource3x4)
    Boardsource 5x12 (Shield: boardsource5x12)
    Chalice (Shield: chalice)
    Clog (Shields: clog_left, clog_right)
    Contra (Shield: contra)
    Corne (Shields: corne_left, corne_right)
    Cradio/Sweep (Shields: cradio_left, cradio_right)
    CRBN Featherlight (Shield: crbn)
    eek! (Shield: eek)
    Elephant42 (Shields: elephant42_left, elephant42_right)
    Ergodash (Shields: ergodash_left, ergodash_right)
    Eternal Keypad Lefty (Shield: eternal_keypad_lefty)
    Eternal Keypad (Shield: eternal_keypad)
    Fourier Rev. 1 (Shields: fourier_left, fourier_right)
    Helix (Shields: helix_left, helix_right)
    Iris (Shields: iris_left, iris_right)
    Jian (Shields: jian_left, jian_right)
    Jiran (Shields: jiran_left, jiran_right)
    Jorne (Shields: jorne_left, jorne_right)
    Knob Goblin (Shield: knob_goblin)
    Kyria Rev2 (Shields: kyria_rev2_left, kyria_rev2_right)
    Kyria Rev3 (Shields: kyria_rev3_left, kyria_rev3_right)
    Kyria (Shields: kyria_left, kyria_right)
    Leeloo-Micro (Shields: leeloo_micro_left, leeloo_micro_right)
    Leeloo v2 (Shields: leeloo_rev2_left, leeloo_rev2_right)
    Leeloo (Shields: leeloo_left, leeloo_right)
    Lily58 (Shields: lily58_left, lily58_right)
    Lotus58 (Shields: lotus58_left, lotus58_right)
    Microdox V2 (Shields: microdox_v2_left, microdox_v2_right)
    Microdox (Shields: microdox_left, microdox_right)
    MurphPad (Shield: murphpad)
    Naked60 (Shield: naked60)
    Nibble (Shield: nibble)
    Osprette (Shield: osprette)
    Pancake (Shield: pancake)
    QAZ (Shield: qaz)
    Quefrency Rev. 1 (Shields: quefrency_left, quefrency_right)
    Redox (Shields: redox_left, redox_right)
    REVIUNG34 (Shield: reviung34)
    REVIUNG41 (Shield: reviung41)
    REVIUNG5 (Shield: reviung5)
    REVIUNG53 (Shield: reviung53)
    Romac+ Macropad (Shield: romac_plus)
    Romac Macropad (Shield: romac)
    SNAP (Shields: snap_left, snap_right)
    Sofle (Shields: sofle_left, sofle_right)
    splitkb.com Aurora Corne (Shields: splitkb_aurora_corne_left, splitkb_aurora_corne_right)
    splitkb.com Aurora Helix (Shields: splitkb_aurora_helix_left, splitkb_aurora_helix_right)
    splitkb.com Aurora Lily58 (Shields: splitkb_aurora_lily58_left, splitkb_aurora_lily58_right)
    splitkb.com Aurora Sofle (Shields: splitkb_aurora_sofle_left, splitkb_aurora_sofle_right)
    splitkb.com Aurora Sweep (Shields: splitkb_aurora_sweep_left, splitkb_aurora_sweep_right)
    Splitreus62 (Shields: splitreus62_left, splitreus62_right)
    TesterProMicro (Shield: tester_pro_micro)
    TG4x (Shield: tg4x)
    Tidbit Numpad (Shield: tidbit)
    2% Milk (Shield: two_percent_milk)
    Waterfowl (Shields: waterfowl_left, waterfowl_right)
    Zodiark (Shields: zodiark_left, zodiark_right)

Seeed XIAO Interconnect

The Seeed Studio XIAO is a popular smaller format micro-controller, that has gained popularity as an alternative to the SparkFun Pro Micro. Since its creation, several pin compatible controllers, such as the Seeed Studio XIAO nRF52840 (also known as XIAO BLE), Adafruit QT Py and Adafruit QT Py RP2040, have become available.
Boards

    Adafruit QT Py RP2040 (Board: adafruit_qt_py_rp2040)
    Seeed Studio XIAO SAMD21 (Board: seeeduino_xiao)
    Seeed Studio XIAO nRF52840 (Board: xiao_ble)
    Seeed Studio XIAO RP2040 (Board: xiao_rp2040)

Shields

    Hummingbird (Shield: hummingbird)
    TesterXiao (Shield: tester_xiao)

Arduino Uno Rev3 Interconnect

The Arduino Uno Rev3 is a board who's popularity lead to countless shields being developed for it. By natural extension, once there were many shields designed for it, many other *boards* began to be developed that were compatible to leverage the extensive available shields. Today, many dev kits come with Uno headers to make it easy to work with them. Note: ZMK doesn't support boards with AVR 8-bit processors, such as the ATmega32U4, because Zephyr™ only supports 32-bit and 64-bit platforms. As a result, boards like the original Arduino Uno Rev3 itself are *not* supported by ZMK.
Boards

    Nordic nRF52840 DK (Board: nrf52840dk/nrf52840)
    Nordic nRF5340 DK (Board: nrf5340dk/nrf5340/cpuapp)

Shields

    ZMK Uno (Shield: zmk_uno)

BlackPill Interconnect

The WeAct Studio BlackPill has grown in popularity due to its low price, availability, and utilization of the powerful STM32F4x1CEU6 microcontroller. The BlackPill features more GPIO than most other boards, but also has a comparatively larger footprint as a result. Many clones and variations of the original BlackPill are available on the market as an affordable and more powerful alternative to many popular boards. The official WeAct variations of the WeAct Studio BlackPill are powered by the STM32F411CEU6 and STM32F401CEU6 microcontrollers.
Boards

    PillBug (Board: pillbug)
    BlackPill F401CC (Board: blackpill_f401cc)
    BlackPill F401CE (Board: blackpill_f401ce)
    BlackPill F411CE (Board: blackpill_f411ce)

Shields

MakerDiary nRF52840 M.2 Interconnect

The MakerDiary nRF52840 M.2 module is a module using the M.2/NGFF form factor to expose a large number of GPIO pins, allowing use of a variety of peripherals such using I2C, SPI, etc.
Boards

    nRF52840 M.2 Module (Board: nrf52840_m2)

Shields

    MakerDiary m60 (Shield: m60)

Other Hardware

In addition to the basic keyboard functionality, there is also support for additional keyboard hardware such as encoders, RGB underglow, backlight and displays. Please see pages under the "Features" header in the sidebar for details.
Contributing

If you'd like to add support for a new keyboard shield, head over to the New Keyboard Shield documentation and note the clean room design requirements.
Edit this page
Previous
Introduction
Next
FAQs

    Onboard Controller Keyboards
    Composite Keyboards
        Pro Micro Interconnect
        Seeed XIAO Interconnect
        Arduino Uno Rev3 Interconnect
        BlackPill Interconnect
        MakerDiary nRF52840 M.2 Interconnect
    Other Hardware
    Contributing

Docs

    Getting Started
    Development

Community

    Discord
    Mastodon
    Twitter

More

    Blog
    GitHub
    Deploys by Netlify 

Copyright © 2026 ZMK Project Contributors. Creative Commons License

if I wish to make a custom pcb/board using nRF52832 QFAAB0, will I have to add it as a shield?
***ChatGPT***: Short answer: **yes — in almost all sane cases, you should add it as a shield** ✅

…but let me explain why, and when you wouldn’t.

 


## How ZMK thinks about hardware (important mental model)

ZMK (built on **Zephyr**) splits hardware into two layers:

  1. 
**Board** → the microcontroller itself

  2. 
**Shield** → the keyboard wiring (matrix, encoders, LEDs, etc.)

This separation is **intentional** and very powerful.

 


## Your case: custom PCB with **Nordic Semiconductor nRF52832 QFAAB0**


### What you are actually building

You’re making:

  - 
a **custom MCU carrier** (nRF52832 + power + crystal + SWD + antenna)

  - 
plus a **keyboard PCB** (matrix, switches, peripherals)

Even if they are on **one physical PCB**, ZMK still prefers:
Physical realityZMK abstractionOne PCBBoard + ShieldTwo PCBsBoard + ShieldHand-wiredBoard + Shield
➡️ **So yes: you add your keyboard as a shield.**

 


## Why you should NOT make it a board (unless you really must)

Creating a **board** means:

  - 
Writing full Zephyr board support

  - 
Defining clocks, flash, RAM, pinmux

  - 
Maintaining DTS, Kconfig, defconfig

  - 
Tracking Zephyr changes forever

That’s a **lot of work** and zero benefit unless:

  - 
you’re upstreaming to ZMK core, or

  - 
your hardware is truly reusable as a general MCU dev board

For a keyboard? ❌ Overkill.

 


## Correct architecture for your project


### 1️⃣ Reuse an existing nRF52832 board

ZMK already supports nRF52 families. You typically:

  - 
pick a close existing board (or create a minimal private one)

  - 
keep it generic (power, clock, radio)


### 2️⃣ Create **your own shield**

Your shield defines:

  - 
key matrix GPIOs

  - 
rows / columns

  - 
encoders

  - 
trackpad (I²C / SPI)

  - 
LEDs

  - 
battery pins

This is exactly what shields are for.

 


## Directory structure you’ll end up with


Build example:


 


## When would you NOT use a shield?

Only if:

  - 
your PCB is a **general-purpose nRF52832 dev board**

  - 
you expect many keyboards to reuse it

  - 
you want it upstreamed as a controller like `nice!nano`

That’s when you make a **board**.
Otherwise: **shield only**.

 


## Practical recommendation for you (based on your goals)

Given your past work with:

  - 
nRF52 BLE HID

  - 
custom keyboards + trackpads

  - 
productization mindset

💡 **Best path**

  - 
Treat your nRF52832 PCB as a controller

  - 
Put all keyboard logic in a shield

  - 
You can later reuse the same controller with:

    - 
macropads

    - 
split keyboards

    - 
trackpad variants


That flexibility is exactly why ZMK is designed this way.

 

If you want, next I can:

  - 
map **nRF52832 QFAAB0 pins → ZMK DTS**

  - 
help you choose a **closest existing board**

  - 
generate a **starter shield DTS + keymap**

  - 
sanity-check your schematic for ZMK compatibility

Just tell me which step you want to do next.
***You***: give me a tutorial to follow
***ChatGPT***: Below is a **clean, end-to-end tutorial** you can **follow linearly** to get from blank nRF52832 PCB → working ZMK keyboard firmware, using **the correct ZMK mental model (board + shield)**.
This is written assuming:

  - 
**MCU**: nRF52832 QFAAB0

  - 
**You are making a custom PCB**

  - 
**You want ZMK (BLE HID)**

  - 
**You are NOT upstreaming yet**


 


## ZMK Custom nRF52832 Keyboard — Step-by-Step Tutorial



 


## STEP 0 — What you are building (fix this in your head)

ZMK (built on **Zephyr**) splits hardware into:
LayerPurposeYou write this**Board**MCU, flash, clock, radio❌ (reuse / minimal)**Shield**Keys, matrix, encoders✅ YES
👉 Even if **everything is on one PCB**, you **still create a shield**.

 


## STEP 1 — Hardware checklist (must-have)

Before firmware, confirm your PCB includes **all of these**:

### Required for nRF52832


  - 
nRF52832 QFAAB0

  - 
32 MHz crystal + load caps (recommended)

  - 
SWDIO + SWCLK + GND pads

  - 
Decoupling caps (0.1 µF near every VDD)

  - 
VDD supply (LDO or battery + regulator)

  - 
Antenna (chip or PCB trace)

⚠️ If **SWD is missing**, stop. You cannot debug ZMK.

 


## STEP 2 — Prepare ZMK environment


Verify:


 


## STEP 3 — Choose a base board (DO NOT create one yet)

You have 3 sane options:
OptionWhenReuse existing nRF52 boardMost commonMinimal private boardCustom PCBUpstream boardLater

### For nRF52832

Start by **cloning an existing nRF52 board** (private):

Edit:

Rename inside files:

⚠️ **Do NOT add matrix pins here**

Board = MCU only.

 


## STEP 4 — Create your keyboard SHIELD (this is the main work)



### 4.1 `Kconfig.shield`



 


### 4.2 `my_keyboard.dtsi` (matrix definition)


👉 Replace GPIO numbers with **your PCB pins**.

 


### 4.3 `my_keyboard.overlay`



 


### 4.4 `my_keyboard.keymap`



 


### 4.5 `my_keyboard.conf`



 


## STEP 5 — Build firmware


If successful:


 


## STEP 6 — Flash firmware


### Using J-Link



### Or Nordic tools



 


## STEP 7 — Verify BLE


  1. 
Reset board

  2. 
Device advertises as:


  3. 
Pair from PC / phone

  4. 
Open text editor

  5. 
Press keys 🎉


 


## STEP 8 — Debug checklist (important)

SymptomCauseNo BLEWrong clock / powerNo keysWrong GPIO mappingRandom resetsBad decouplingWon’t flashSWD wiring
Enable logs:


 


## STEP 9 — Next upgrades (when ready)


  - 
Split keyboard → `*_left.overlay`, `*_right.overlay`

  - 
Trackpad → I²C device node

  - 
Battery → `zmk,battery`

  - 
Encoder → `zmk,behavior-encoder`


 


## Mental rule you should remember forever


**Boards describe silicon.
Shields describe keyboards.
Physical PCBs don’t matter.**


 

If you want, next I can:

  - 
generate a **pin-correct DTS from your schematic**

  - 
add **trackpad (Cirque / Azoteq)**

  - 
create a **split BLE setup**

  - 
review your **actual PCB design**

  - 
give you a **production-ready template repo**

Just say which one you want next.
***You***: west build -s app -b    
      nrf52dk/nrf52832 -- -DSHIELD=tkl_nrf52832?
***ChatGPT***: Almost correct 👍 — you’re **95% there**. Let’s fix the last 5% and explain why.

 


## ✅ Correct `west build` command (for your case)



### ❌ What was wrong in your command


The `?` **must not be there**.

ZMK / CMake does **exact string matching** on shield names.

 


## What each part means (so you can reason about errors)


→ Zephyr build system (used by ZMK)

→ Source directory = `app/` (this is where ZMK’s `CMakeLists.txt` lives)

→ **Board**

  - 
Uses the Nordic nRF52 DK definition

  - 
Safe choice for nRF52832 bring-up


→ **Shield**

  - 
Must match:



 


## Required folder check (VERY IMPORTANT)

Before building, this **must exist**:

If the folder name or files don’t match **exactly**, ZMK will fail silently or with confusing errors.

 


## How to verify ZMK sees your shield

Run:

If you **do not see** `tkl_nrf52832` in the list → ZMK cannot see your shield.

 


## Common failure modes (check these if it breaks)

ErrorMeaning`Unknown SHIELD`Folder name mismatch`No kscan chosen`Missing `chosen { zmk,kscan = ... }``dtc error`Bad GPIO syntax`undefined reference to kscan`DTS not included

 


## If you want this to be your own board later

Right now:

Later (custom PCB):

Shield stays the same — **that’s the whole point**.

 


## Minimal sanity test (recommended)

If this builds:

Then:

  - 
Your environment is correct

  - 
Any failure is **inside your shield**


 

If you want, paste:

  - 
the **exact error**

  - 
or your **shield directory tree**

  - 
or your **.overlay / .dtsi**

and I’ll pinpoint it in one reply.
***You***: if I want to add i2c trackpad, azoteq ones. what to do?
***ChatGPT***: Below is a **follow-this-exactly tutorial** for adding an **Azoteq I²C trackpad** to your **ZMK + nRF52832** keyboard.
This is written for **Azoteq IQS5xx series** (IQS550 / IQS572 / IQS5xx), which are the **best-supported trackpads in ZMK** today.

 


## Adding an Azoteq I²C Trackpad to ZMK (Step-by-Step)



 


## STEP 0 — What ZMK already supports (important)

ZMK **already has a driver** for Azoteq trackpads via:

  - 
`zmk,input-processor-azoteq`

So you **do NOT** write a driver.

You only:

  1. 
Wire it correctly

  2. 
Declare it in **DTS**

  3. 
Enable it in **config**


 


## STEP 1 — Hardware wiring (non-negotiable)


### Minimum required connections

Trackpad PinnRF52832**VDD**3.3 V**GND**GND**SDA**Any GPIO (I²C SDA)**SCL**Any GPIO (I²C SCL)**RDY / INT**Optional (recommended)**RST**Optional
⚠️ **Important**

  - 
Pull-ups on SDA/SCL **required** (2.2k–4.7k)

  - 
Azoteq runs at **3.3 V only**

  - 
I²C speed: **100 kHz** (safe default)


 


## STEP 2 — Pick GPIOs (example)

Example mapping (replace with your PCB pins):
FunctionGPIOSDAP0.26SCLP0.27INTP0.28

 


## STEP 3 — Enable I²C on your board (if not already)

In your **board DTS or overlay** (not shield yet):

If `i2c0` doesn’t exist, use `i2c1`.

 


## STEP 4 — Add the trackpad to your SHIELD DTS

📁 `app/boards/shields/tkl_nrf52832/tkl_nrf52832.dtsi`


Most Azoteq modules use `0x74`.

If unsure, scan I²C later.


 


## STEP 5 — Tell ZMK to use it as a pointing device

Add to the **same DTS**:

This is **mandatory**.

 


## STEP 6 — Enable mouse support in config

📁 `tkl_nrf52832.conf`

Optional tuning:


 


## STEP 7 — Add mouse bindings to your keymap

📁 `tkl_nrf52832.keymap`

Example layer snippet:


 


## STEP 8 — Build



 


## STEP 9 — Debug checklist (very important)


### If cursor doesn’t move

Enable logs:

Watch for:


### I²C scan (optional but powerful)

Add temporarily:

Then in shell:


 


## Common failure modes (read this)

SymptomCauseNo cursorMissing `chosen zmk,pointing-device`Build errorWrong compatible stringRandom jumpsMissing pull-upsLagI²C too slow / power noiseNot detectedWrong I²C address

 


## Azoteq vs Cirque (quick truth)

FeatureAzoteqCirqueGesture quality⭐⭐⭐⭐⭐⭐⭐⭐⭐ZMK support✅ Native⚠ PartialConfig flexibilityHighMediumProduct readiness**Best**Good
👉 You picked the **right one** for a product.

 


## What I can do next (high value)

If you want, I can:

  - 
generate **pin-accurate DTS** from your schematic

  - 
tune **gesture curves** (Mac-like feel)

  - 
add **tap-to-click / scroll zones**

  - 
help with **PCB layout rules for Azoteq**

  - 
make this **production-stable** (ESD, noise, firmware guards)

Just tell me what you want next.
***You***: Bash(source ~/.zshrc && conda activate zmk && cd ~/zmk && rm -rf build && west build -s app -b    timeout: 5m 
      nrf52dk/nrf52832 -- -DSHIELD=tkl_nrf52832 2>&1 | tail -40)                                    0s

  ⎿   See https://zmk.dev/docs/development/hardware-integration/new-shield and                             
      https://zmk.dev/docs/development/module-creation for more information.
    Call Stack (most recent call first):
      /Users/chaitanyamalhotra/zmk/zephyr/cmake/modules/zephyr_module.cmake:126 (include)
      /Users/chaitanyamalhotra/zmk/zephyr/cmake/modules/zephyr_default.cmake:134 (include)
      /Users/chaitanyamalhotra/zmk/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake:66 (include)
      /Users/chaitanyamalhotra/zmk/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake:92
    (include_boilerplate)
      CMakeLists.txt:9 (find_package)


    -- Board: nrf52dk, qualifiers: nrf52832
    -- Shield(s): tkl_nrf52832
    Checking /Users/chaitanyamalhotra/zmk/app
    found /Users/chaitanyamalhotra/zmk/app
    -- ZMK Config directory: /Users/chaitanyamalhotra/zmk-config
    nrf52dk_nrf52832 for _nrf52832 for nrf52dk with version
    -- Using keymap file: /Users/chaitanyamalhotra/zmk-config/tkl_nrf52832.keymap
    Checking /Users/chaitanyamalhotra/zmk-config
    Checking /Users/chaitanyamalhotra/zmk/zephyr
    -- Found host-tools: zephyr 0.17.4 (/Users/chaitanyamalhotra/zephyr-sdk-0.17.4)
    -- Found toolchain: zephyr 0.17.4 (/Users/chaitanyamalhotra/zephyr-sdk-0.17.4)
    -- Found Dtc: /opt/homebrew/bin/dtc (found suitable version "1.7.2", minimum required is "1.4.6")
    -- Found BOARD.dts: /Users/chaitanyamalhotra/zmk/zephyr/boards/nordic/nrf52dk/nrf52dk_nrf52832.dts
    -- Found devicetree overlay:
    /Users/chaitanyamalhotra/zmk/app/boards/shields/tkl_nrf52832/tkl_nrf52832.overlay
    -- Found devicetree overlay: /Users/chaitanyamalhotra/zmk-config/tkl_nrf52832.keymap
    devicetree error: /Users/chaitanyamalhotra/zmk-config/tkl_nrf52832.keymap:13 (column 439): parse error:
     expected number or parenthesized expression
    CMake Error at /Users/chaitanyamalhotra/zmk/zephyr/cmake/modules/dts.cmake:303 (execute_process):
      execute_process failed command indexes:

        1: "Child return code: 1"

    Call Stack (most recent call first):
      /Users/chaitanyamalhotra/zmk/zephyr/cmake/modules/zephyr_default.cmake:134 (include)
      /Users/chaitanyamalhotra/zmk/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake:66 (include)
      /Users/chaitanyamalhotra/zmk/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake:92
    (include_boilerplate)
      CMakeLists.txt:9 (find_package)


    -- Configuring incomplete, errors occurred!
    FATAL ERROR: command exited with status 1: /opt/homebrew/bin/cmake
    -DWEST_PYTHON=/opt/miniconda3/envs/zmk/bin/python3.11 -B/Users/chaitanyamalhotra/zmk/build -GNinja
    -DBOARD=nrf52dk/nrf52832 -DSHIELD=tkl_nrf52832 -S/Users/chaitanyamalhotra/zmk/app
***ChatGPT***: Good news first: **your build setup is correct** ✅

Bad news (also good): this is a **pure syntax error in your .keymap**, not ZMK, not Zephyr, not your board.
Let’s fix it cleanly.

 


## What the error actually means


Key facts:

  - 
`.keymap` files are **devicetree (DTS)**, not C

  - 
The parser choked at **line 13**

  - 
Column 439 = **a very long bindings = < ... >; line**

  - 
This almost always means:

    - 
missing `&`

    - 
missing `< >`

    - 
stray comma

    - 
comment in the wrong place

    - 
mouse bindings written incorrectly



 


## 99% likely cause (based on what you added)

You recently added **mouse / trackpad bindings**, and this is the most common mistake:
❌ **WRONG**

DTS **requires phandles**, so **every behavior MUST start with &**.

 


## Correct minimal keymap (COPY THIS EXACTLY)

Open:

Replace the entire contents with this **known-good** version:

Now build again:

👉 If this builds, we have **confirmed the issue is inside your bindings**, not infrastructure.

 


## Correct mouse / trackpad bindings (safe syntax)

When you re-add the trackpad, use **this exact format**:


### Rules you must follow (DTS rules, not ZMK rules)


  1. 
✅ Every behavior starts with `&`

  2. 
✅ No commas inside `< >`

  3. 
✅ Comments only use `/* */`, not `//`

  4. 
✅ Line breaks are allowed, commas are not

  5. 
❌ No trailing junk after `>;`


 


## Very common hidden mistakes (check these)


### ❌ Trailing comma



### ❌ Inline `//` comment



### ❌ Missing angle brackets



### ❌ Parentheses (DTS is NOT C)



 


## Why the error column is huge (column 439)

Devicetree treats:

as **one giant token stream**.

One bad token anywhere → error reported at the end.
That’s why DTS errors feel “unhelpful” — but they’re deterministic.

 


## Next step (do this in order)


  1. 
✅ Build with **minimal keymap** (above)

  2. 
✅ Confirm build succeeds

  3. 
✅ Re-add mouse bindings **exactly as shown**

  4. 
❌ Do NOT edit DTS casually — it is very strict


 


## If it still fails

Paste **only this file** here:

I’ll spot the exact bad token in one glance and tell you the precise fix.
You’re very close — this is the last annoying Zephyr hurdle, not a real problem.