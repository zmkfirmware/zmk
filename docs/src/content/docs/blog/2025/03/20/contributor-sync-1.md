---
title: Contributor Sync February 2025
authors: nmunnich
tags: [sync, keyboards, firmware, oss, ble]
---

On the 26th of February some of the ZMK contributors got together for a contributor sync. In the interest of transparency, it was decided that meeting notes from the sync would be published on the ZMK webpage.

<!-- truncate -->

The following contributors were able to attend the sync:

- [petejohanson]
- [caksoylar]
- [nmunnich]

[joelspadin] was also able to comment on the meeting notes after the meeting. The notes have been adjusted accordingly.

## Meeting Notes

### Meeting Purpose

- Purpose of meeting is to sync up, discuss blockers, talk about big picture work.
- Meeting notes should be published on the webpage, while the meetings themselves are private.

### Pete's Summary

- Starting a new job Mon. 2025-03-03. Reduced time for ZMK, but more freedom to work on what Pete wants/ZMK needs.
- Two main ZMK-contracts need to be finished:
  - Pointer work. Basically good to go, just needs 2721 to be tested and verified as as fix. Then ZMK release v0.2 will happen. (This has already happened at time of publishing these notes)
  - Wired split work. Separated into parts:
    - UART full duplex, PR 2766. What's there seems very solid, barring some final touchups. Should have v0.2 released prior to merging this. (This has already happened at time of publishing these notes)
    - Half-duplex with a single wire. To be a separate PR, but framework should be good for when that happens. To work on after UART full duplex is merged.
    - Runtime dynamic switching between wired and wireless split using a GPIO as sense pin for wired.
- After contracts finish, look at versioning again. Better documentation, defaults, workflow improvements, etc.
- More Studio work is also on the cards. Studio work will continue, but slowly.
  - Hoping for help from community around UI/UX, in particular clarity around saving edited keymaps would be good.
  - Want to add runtime property modification next, allowing for e.g. changing hold-tap flavors in Studio.
- Investigate zmk-cli a bit; Pete has not investigated this enough and wants to get a solid overview of it.

### Nick's summary:

- Trying to make combo PR feasible (allows combo triggers inside behaviors, enabling e.g. tap-only combos)
  - Adding `behavior_state_changed` event has a precursor (see 573, 532)
    - Requires deleting ZMK_BEHAVIOR_TRANSPARENT
    - Requires some refactoring/investigating of sensors/encoders, as they rely on ZMK_BEHAVIOR_TRANSPARENT
- Locking PR, needs review. Same with 2811.
- RP2040-Zero board definition matches upstream, acts as a temporary board definition until we update Zephyr (introduced to Zephyr in 4.1)
- Parameterised mod-morph and tap-dance
  - Question: Do we move hold-tap over to the same system?
    - Plan after discussion is to add a second hold-tap `compatible` and move over to it, to avoid breaking existing keymaps.
- Config index page and "Customising ZMK" docs overhaul, making them more visible and easier to understand.

### Cem's summary:

- Keymap recipes has been shelved for the time being. Prefer to have better examples in the behavior pages themselves, e.g. timeless homerow mods as an example under hold-tap.
- Hold-tap page needs overhauling. (This has already happened at time of publishing these notes)
- New behavior guide needs overhauling and should be aimed towards modules.
- 2758 needs a review and merge. (This has already happened at time of publishing these notes)

### Other Discussion:

#### ZMK-CLI:

- It is likely time to rip the bandaid off and switch over to it, getting rid of the old setup script.
- CLI has some flaws that may need improvement (e.g. around modules), but is already much better than the existing script.
- New shield guide should be adjusted to take advantage of ZMK-CLI's templates.
- Joel requests issues be opened talking about points to improve on.

#### zmk-locales:

- Should be moved under zmkfirmware organisation
  - Should do the same for zmk-locales-generator, and maybe figure out a way to set up a GH action that makes it send PRs to zmk-locales.
    - Move first, make these adjustments afterwards
  - Documenting them in list of keycodes page with a locale selector would be ideal, but complex
    - Refer people to headers directly in the docs for the time being
- Runtime locale changing would be preferred, but would also be a lot of work.
  - zmk-locales could be viewed as a (semi-permanent) bandaid until such a time

#### Modules:

- Need to find the right balance between overloading contributers and overfragmentation
- ZMK module collection https://github.com/nmunnich/zmk-module-collection seems like a good way to go forward
  - Main concerns:
    - Need to handle breaking changes elegantly, informing users _which_ module is to blame for the new version not working
    - Need to have tests for modules (can assume keyboard works if it builds, what about other stuff?)
    - Maintainability
      - Hopefully have more volunteers from the community to look at modules and verify them, should be much easier than reviewing PRs
      - Automated tests
      - Add an "experimental" group tag for unverified modules
  - Despite concerns, plenty of upsides. Seems like the best way forwards, and it is worth putting more time into.
  - Some upsides:
    - Very easy for users to add modules, particularly ones that depend on other modules
    - Automated testing and notification for module owners
    - Better discoverability
    - Versioning parity between the collection and ZMK itself to allow for easy upgrades
  - Likely will want to introduce "RC" releases, to allow module owners the time to fix any issues with their module before an actual release
  - Likely will only accept keyboards and drivers initially, until better tooling and systems are in place, given they are easier to validate

#### Minor notes:

- Documenting BLE tests would be useful
- Need new display and lighting systems at some point
- Zephyr upgrade was not discussed in this meeting
- Would be good to give some extra priority to PRs from new contributors, to help motivate them.
- Would be nice to have another SOTF at some point. Doing a full commit overview is unfeasible at this point, but touching on the major issues seems doable.

[petejohanson]: https://github.com/petejohanson
[joelspadin]: https://github.com/joelspadin
[caksoylar]: https://github.com/caksoylar
[nmunnich]: https://github.com/nmunnich
