# Changelog

## [0.3.0](https://github.com/zmkfirmware/zmk/compare/v0.2.1...v0.3.0) (2025-08-01)


### Features

* **ble:** Add function to get profile address by index ([#2992](https://github.com/zmkfirmware/zmk/issues/2992)) ([9e905d6](https://github.com/zmkfirmware/zmk/commit/9e905d65936348824588dc3f424755353ac61186))
* **ci:** Add stale GitHub Action to automatically close stale PRs ([#2924](https://github.com/zmkfirmware/zmk/issues/2924)) ([6d7bbc8](https://github.com/zmkfirmware/zmk/commit/6d7bbc8670d175fd63e8c834feb41f80e7b52e74))
* **display:** nice!view individual profile status ([#2265](https://github.com/zmkfirmware/zmk/issues/2265)) ([d09087f](https://github.com/zmkfirmware/zmk/commit/d09087f4dc280b8fdb1d32d63b03cc10162b89ce))
* Full-Duplex Wired Split ([#2766](https://github.com/zmkfirmware/zmk/issues/2766)) ([147c340](https://github.com/zmkfirmware/zmk/commit/147c340c6e8d377304acfdd64dc86cf83ebdfef2))
* **metadata:** Add metadata to the mouse_key_press behavior ([#2950](https://github.com/zmkfirmware/zmk/issues/2950)) ([239baa4](https://github.com/zmkfirmware/zmk/commit/239baa487509ace108d36f0e5c627d61a3d95f53))
* **pointing:** Allow peripheral input processing to stop propagation ([#2844](https://github.com/zmkfirmware/zmk/issues/2844)) ([462d48b](https://github.com/zmkfirmware/zmk/commit/462d48b78edac8bedb75666699ea4fa446d2152c))
* **shield:** Add underglow for reviung5 ([#2191](https://github.com/zmkfirmware/zmk/issues/2191)) ([ad6a181](https://github.com/zmkfirmware/zmk/commit/ad6a181d7ec34fb6e31134f6bb991a9b2d0b8f78))
* **shields:** Add a physical layout for a_dux ([#3000](https://github.com/zmkfirmware/zmk/issues/3000)) ([7292df0](https://github.com/zmkfirmware/zmk/commit/7292df02d4b05d783f432f8658de22d940909fe4))
* **shields:** Add physical layouts for tester_xiao and tester_pro_micro ([#2852](https://github.com/zmkfirmware/zmk/issues/2852)) ([eb170c9](https://github.com/zmkfirmware/zmk/commit/eb170c930f56e3fb3df0b813d987abfd1dc31b9a))
* **shields:** Add tester_pro_micro layouts ([eb170c9](https://github.com/zmkfirmware/zmk/commit/eb170c930f56e3fb3df0b813d987abfd1dc31b9a))
* **shields:** Add tester_xiao layouts ([eb170c9](https://github.com/zmkfirmware/zmk/commit/eb170c930f56e3fb3df0b813d987abfd1dc31b9a))
* **split:** Add full-duplex wired split support ([147c340](https://github.com/zmkfirmware/zmk/commit/147c340c6e8d377304acfdd64dc86cf83ebdfef2))
* **split:** Runtime selection of split transport ([6b44d33](https://github.com/zmkfirmware/zmk/commit/6b44d33db2f4bad7d98e475e6f7968493b05af73))
* **split:** Runtime selection of split transport ([#2886](https://github.com/zmkfirmware/zmk/issues/2886)) ([6b44d33](https://github.com/zmkfirmware/zmk/commit/6b44d33db2f4bad7d98e475e6f7968493b05af73))
* **split:** Suspend/resume wired UART devices. ([6b44d33](https://github.com/zmkfirmware/zmk/commit/6b44d33db2f4bad7d98e475e6f7968493b05af73))


### Bug Fixes

* **behaviors:** Correct macro release state for parametrized ([1bac680](https://github.com/zmkfirmware/zmk/commit/1bac680c4fb4f07e43c01754b6f1e72dab455e50))
* **behaviors:** Correct macro release state for parametrized macros ([#2942](https://github.com/zmkfirmware/zmk/issues/2942)) ([1bac680](https://github.com/zmkfirmware/zmk/commit/1bac680c4fb4f07e43c01754b6f1e72dab455e50))
* **ble,hid:** Fix smooth scrolling over BLE ([#2998](https://github.com/zmkfirmware/zmk/issues/2998)) ([342d838](https://github.com/zmkfirmware/zmk/commit/342d83891301b1be53233a12c7723bb99cbe5ff6)), closes [#2957](https://github.com/zmkfirmware/zmk/issues/2957)
* **boards:** Disable high voltage DC-DC by default ([#2995](https://github.com/zmkfirmware/zmk/issues/2995)) ([8059e67](https://github.com/zmkfirmware/zmk/commit/8059e671b24a261939401afb5a65c4fa756adc2d)), closes [#2990](https://github.com/zmkfirmware/zmk/issues/2990)
* changed shebang to make scripts more platform independent ([#2893](https://github.com/zmkfirmware/zmk/issues/2893)) ([84772eb](https://github.com/zmkfirmware/zmk/commit/84772ebf14e5a7c67ba573a61f0a50048802c799))
* **ci:** pin tj-actions/changed-files due to compromise ([#2874](https://github.com/zmkfirmware/zmk/issues/2874)) ([4da89bd](https://github.com/zmkfirmware/zmk/commit/4da89bd99716bf6c1d7d788f3cdaec4cee7403e9))
* **combos:** Properly clean up all old candidates. ([#2928](https://github.com/zmkfirmware/zmk/issues/2928)) ([e3030bf](https://github.com/zmkfirmware/zmk/commit/e3030bfcc87b7f511b0ebe993fb1f1f06215982e))
* **combos:** Restore prompts for two deprecated Kconfigs ([#2926](https://github.com/zmkfirmware/zmk/issues/2926)) ([00ff486](https://github.com/zmkfirmware/zmk/commit/00ff48693113ed74a3345aa1ac81fdea302b3a09))
* **core:** Correctly sync BAS battery level ([#2977](https://github.com/zmkfirmware/zmk/issues/2977)) ([af96766](https://github.com/zmkfirmware/zmk/commit/af967667b0e139a963178e63028c7be341cade9e))
* **display:** Make stock battery widget depend on the right symbol ([#2953](https://github.com/zmkfirmware/zmk/issues/2953)) ([9da5d3b](https://github.com/zmkfirmware/zmk/commit/9da5d3ba82b38b74ad798a82a838d84c52220bbe))
* **docs:** Fix soft off waker configuration example ([#2960](https://github.com/zmkfirmware/zmk/issues/2960)) ([eb99b4e](https://github.com/zmkfirmware/zmk/commit/eb99b4ede06bc01674ce16217ebbad40bc11ec50))
* **docs:** remove title as alt text ([#2922](https://github.com/zmkfirmware/zmk/issues/2922)) ([d9576c5](https://github.com/zmkfirmware/zmk/commit/d9576c55346acfc8eed36709aaae28f91e0d06ad))
* Fix build with Studio and USB but not UART ([#2996](https://github.com/zmkfirmware/zmk/issues/2996)) ([cef7af4](https://github.com/zmkfirmware/zmk/commit/cef7af4408cc44c20fab93a0b2e20b3429d0a98e))
* **hid:** Fix scroll value truncation ([#2865](https://github.com/zmkfirmware/zmk/issues/2865)) ([2c0e7da](https://github.com/zmkfirmware/zmk/commit/2c0e7daced1421c34a9d417b7d3e9bccbf0ebd7f)), closes [#2864](https://github.com/zmkfirmware/zmk/issues/2864)
* **pointing:** Avoids mutex leak for default layer toggle event ([#2934](https://github.com/zmkfirmware/zmk/issues/2934)) ([461f5c8](https://github.com/zmkfirmware/zmk/commit/461f5c832fb8854d87dca54d113d306323697219))
* Properly override stack size on RP2040 ([147c340](https://github.com/zmkfirmware/zmk/commit/147c340c6e8d377304acfdd64dc86cf83ebdfef2))
* **split:** add source to battery event ([#2901](https://github.com/zmkfirmware/zmk/issues/2901)) ([6f85f48](https://github.com/zmkfirmware/zmk/commit/6f85f48b19afae04f98e9abacb36ce1425b61f78))
* **split:** Compile and run properly in wired polling mode. ([#3012](https://github.com/zmkfirmware/zmk/issues/3012)) ([2ae5185](https://github.com/zmkfirmware/zmk/commit/2ae51854192aed92af95536f79547e2928cd1bf5))
* **split:** Conditionally build all split code ([#2884](https://github.com/zmkfirmware/zmk/issues/2884)) ([5bb39ec](https://github.com/zmkfirmware/zmk/commit/5bb39ec3eae23055593350cb3689a8240028181e))
* **split:** Enable wired split by default if DTS is set ([#3010](https://github.com/zmkfirmware/zmk/issues/3010)) ([1530ae3](https://github.com/zmkfirmware/zmk/commit/1530ae36c22e3e2285e895737c74de5d960a5ae4))
* **split:** Minor wired split fixes. ([6b44d33](https://github.com/zmkfirmware/zmk/commit/6b44d33db2f4bad7d98e475e6f7968493b05af73))
* Unconditionally define HID payloads to avoid error. ([6b44d33](https://github.com/zmkfirmware/zmk/commit/6b44d33db2f4bad7d98e475e6f7968493b05af73))

## [0.2.1](https://github.com/zmkfirmware/zmk/compare/v0.2.0...v0.2.1) (2025-03-02)


### Bug Fixes

* **behaviors:** Proper comma separated device list ([#2850](https://github.com/zmkfirmware/zmk/issues/2850)) ([f20e6ea](https://github.com/zmkfirmware/zmk/commit/f20e6ea7594b49eef1e3acc017529073a0409962))

## [0.2.0](https://github.com/zmkfirmware/zmk/compare/v0.1.0...v0.2.0) (2025-03-01)


### Features

* Added `toggle-mode`, allowing toggle-on and toggle-off ([#2555](https://github.com/zmkfirmware/zmk/issues/2555)) ([4ef231f](https://github.com/zmkfirmware/zmk/commit/4ef231f4bba87151acfbd1cf3babd83b69813e45))
* added toggle mode to key and layer toggles ([4ef231f](https://github.com/zmkfirmware/zmk/commit/4ef231f4bba87151acfbd1cf3babd83b69813e45))
* **boards:** Update for mikoto board definition ([#1946](https://github.com/zmkfirmware/zmk/issues/1946)) ([b26058b](https://github.com/zmkfirmware/zmk/commit/b26058b6c7c83f8d1f095d2f9c6c3998b391a61b))
* **core:** Make physical layout key rotation optional ([#2770](https://github.com/zmkfirmware/zmk/issues/2770)) ([c367d8f](https://github.com/zmkfirmware/zmk/commit/c367d8f636f0842b414c2b58df6101761cdd676d))
* **display:** Add ability to set display on/off pin. ([#2814](https://github.com/zmkfirmware/zmk/issues/2814)) ([627e6db](https://github.com/zmkfirmware/zmk/commit/627e6dbec99211b3d7cce55904fb1c824ed87bf3))
* **display:** Add config for display update period ([#2819](https://github.com/zmkfirmware/zmk/issues/2819)) ([aa3e5dd](https://github.com/zmkfirmware/zmk/commit/aa3e5dd70fdd1b364fa9ad26f14425be613d180c))
* input processor behavior invocation ([#2714](https://github.com/zmkfirmware/zmk/issues/2714)) ([cb867f9](https://github.com/zmkfirmware/zmk/commit/cb867f92dbe4e32675c2137fc6aa914a44ecc8dc))
* **Kconfig:** Allow overriding ZMK Kconfig defaults ([#2537](https://github.com/zmkfirmware/zmk/issues/2537)) ([40925d4](https://github.com/zmkfirmware/zmk/commit/40925d48e67b3eeaeb3e848a2287ed628de9f674))
* **mouse:** Add mouse move and scroll support ([#2477](https://github.com/zmkfirmware/zmk/issues/2477)) ([6b40bfd](https://github.com/zmkfirmware/zmk/commit/6b40bfda53571f7a960ccc448aa87f29da7496ac))
* **pointing:** Add behavior input processor ([cb867f9](https://github.com/zmkfirmware/zmk/commit/cb867f92dbe4e32675c2137fc6aa914a44ecc8dc))
* **pointing:** Add pre-defined scroll scaler ([0f7c112](https://github.com/zmkfirmware/zmk/commit/0f7c11248a1ddb7c6559064c2a1e7a3c446d5d55))
* **pointing:** Add pre-defined scroll scaler and mouse scroll tests ([#2759](https://github.com/zmkfirmware/zmk/issues/2759)) ([0f7c112](https://github.com/zmkfirmware/zmk/commit/0f7c11248a1ddb7c6559064c2a1e7a3c446d5d55))
* **shields:** Add physical layout for Lotus58 ([#2753](https://github.com/zmkfirmware/zmk/issues/2753)) ([424e532](https://github.com/zmkfirmware/zmk/commit/424e53210ea16c2287abaf770ebf45be432d841a))
* **studio:** Add ortho_4x10 grid layout ([#2651](https://github.com/zmkfirmware/zmk/issues/2651)) ([7e8c542](https://github.com/zmkfirmware/zmk/commit/7e8c542c94908ac011ec7272a5f8ab10d2102632))


### Bug Fixes

* allow kscan-composite to wake up device. ([#2682](https://github.com/zmkfirmware/zmk/issues/2682)) ([a8f5ab6](https://github.com/zmkfirmware/zmk/commit/a8f5ab67b5d449a2624e2de7ddfb264da778ea6c))
* **behaviors:** Make multiple sticky keys work on same key position ([7186528](https://github.com/zmkfirmware/zmk/commit/7186528f77bf077173927c1c8506b4d434e5c371))
* **behaviors:** Make multiple sticky keys work on same key position ([#2758](https://github.com/zmkfirmware/zmk/issues/2758)) ([7186528](https://github.com/zmkfirmware/zmk/commit/7186528f77bf077173927c1c8506b4d434e5c371))
* **ble:** enforce maximum length for dynamic device name ([#2784](https://github.com/zmkfirmware/zmk/issues/2784)) ([ea267b0](https://github.com/zmkfirmware/zmk/commit/ea267b0f35f862b882ac568dde6365c3a0c85099))
* **combos:** Properly report combos len with emply block ([#2739](https://github.com/zmkfirmware/zmk/issues/2739)) ([f0a77b8](https://github.com/zmkfirmware/zmk/commit/f0a77b888ac482a863386ced08e04660ddacb026))
* **display:** Only default mono theme when 1bpp ([#2804](https://github.com/zmkfirmware/zmk/issues/2804)) ([425256b](https://github.com/zmkfirmware/zmk/commit/425256bc0de7ed08802533b170abba78ee90f546))
* **display:** POSIX lvgl fixes ([#2812](https://github.com/zmkfirmware/zmk/issues/2812)) ([4b4a8a3](https://github.com/zmkfirmware/zmk/commit/4b4a8a35f3f90f1af75cdf5d9c26b47d4b8dcabb))
* **drivers:** Proper static/const for data/config ([#2769](https://github.com/zmkfirmware/zmk/issues/2769)) ([6941abc](https://github.com/zmkfirmware/zmk/commit/6941abc2afab16502cff9c5149d8dc0fcd5112c9))
* Fix warnings in nanopb encoding code ([#2643](https://github.com/zmkfirmware/zmk/issues/2643)) ([7013158](https://github.com/zmkfirmware/zmk/commit/7013158a6715d94b34e8c471ce25bb5005f3bb49))
* Kconfig refactor now works correctly with external modules ([#2711](https://github.com/zmkfirmware/zmk/issues/2711)) ([bb48661](https://github.com/zmkfirmware/zmk/commit/bb486619a183f6df7fbb4620c80164555a22da0b))
* **Kconfig:** Added a name to EC11's trigger mode choice ([40925d4](https://github.com/zmkfirmware/zmk/commit/40925d48e67b3eeaeb3e848a2287ed628de9f674))
* **kscan:** Remove warning when keyboard is built without CONFIG_PM_DEVICE ([#2808](https://github.com/zmkfirmware/zmk/issues/2808)) ([8e065d5](https://github.com/zmkfirmware/zmk/commit/8e065d55b916481ef06ce37cddedb84cf1d15d99))
* **pointing:** Complete header rename missed in refactor ([#2702](https://github.com/zmkfirmware/zmk/issues/2702)) ([84baf92](https://github.com/zmkfirmware/zmk/commit/84baf929c9bb95f255d4bafd0e57f2ec47455fca))
* **pointing:** Temp layer threading protection. ([#2729](https://github.com/zmkfirmware/zmk/issues/2729)) ([1e3e62c](https://github.com/zmkfirmware/zmk/commit/1e3e62c13d0666d98831ee302ae2fb17e68196c9))
* **studio:** Allow adding layers after a layer move ([#2748](https://github.com/zmkfirmware/zmk/issues/2748)) ([36508c2](https://github.com/zmkfirmware/zmk/commit/36508c27fddfb84d912e0122e313ad3904ceb946))
* **studio:** Properly return complete keymap from RPC ([#2696](https://github.com/zmkfirmware/zmk/issues/2696)) ([0820991](https://github.com/zmkfirmware/zmk/commit/0820991901a95ab7a0eb1f1cc608a631d514e26c))

## 0.1.0 (2024-11-29)


### Features

* **boards:** Add glove80 nexus node for extension GPIO. ([#2594](https://github.com/zmkfirmware/zmk/issues/2594)) ([fb359f5](https://github.com/zmkfirmware/zmk/commit/fb359f576619940164ca2e770b49b7b34f13428e))
* **boards:** add nrf52833-nosd snippet ([63af296](https://github.com/zmkfirmware/zmk/commit/63af296b6efd8d677d584f372c9da9a4fedaa496))
* **boards:** add nrf52840-nosd snippet ([4438b7b](https://github.com/zmkfirmware/zmk/commit/4438b7b835bfd1d4e89cdd955a4ab0fd2e2ae3bf))
* **ci:** Add release-please automation with VERSION ([#2622](https://github.com/zmkfirmware/zmk/issues/2622)) ([ffa485c](https://github.com/zmkfirmware/zmk/commit/ffa485c11b48444acf3adf1e3c1cb3eed16fad94))
* **drivers:** Support init high/low in 595 driver ([888c0d9](https://github.com/zmkfirmware/zmk/commit/888c0d966cd52f3ab5145992f61b14d6262c1951))


### Bug Fixes

* **boards:** Disable uart serial node in Xiao BLE by default ([#2672](https://github.com/zmkfirmware/zmk/issues/2672)) ([230b860](https://github.com/zmkfirmware/zmk/commit/230b860f31063774c3bcc19afb6f92479462de24))
* **boards:** Fix typo in BT75 metadata ([c9553c3](https://github.com/zmkfirmware/zmk/commit/c9553c31e3a3f39964391b006492995b5bb09c39))
* Disable display feature for settings_reset ([b0f5789](https://github.com/zmkfirmware/zmk/commit/b0f5789b128f0f5599341398898fdb0e0407b2d3))
* Fix inconsistent column offset property ([c7473fc](https://github.com/zmkfirmware/zmk/commit/c7473fc32557d2d384ab78d3acf51a05488f0214))
* include a header file for RC macros ([#2649](https://github.com/zmkfirmware/zmk/issues/2649)) ([f8eff2f](https://github.com/zmkfirmware/zmk/commit/f8eff2fe34609c91211c25113f9d7db09f7d1689))
* **studio:** Improved error message when keyboard is missing a physical layout. ([fed66a9](https://github.com/zmkfirmware/zmk/commit/fed66a92d000f4c8e0019d9ccdd167271324e8e9))
