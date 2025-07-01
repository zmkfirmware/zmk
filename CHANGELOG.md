# Changelog

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
