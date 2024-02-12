---
title: Fixing the Mysterious Broken Bootloader
author: Nick Winans
author_title: Contributor
author_url: https://github.com/Nicell
author_image_url: https://avatars1.githubusercontent.com/u/9439650
tags: [bootloader, keyboards, firmware, oss, ble]
---

Recently I was able to fix the "stuck in the bootloader" issue in
[#322](https://github.com/zmkfirmware/zmk/pull/322) that had been plaguing us
for quite some time. I want to go over what the issue was, how the issue was
diagnosed, and how it was fixed.

## Background

What exactly is the "stuck in the bootloader" issue? Seemingly randomly, users'
keyboards would suddenly stop working and when they would reset their keyboard
they would get put into the bootloader instead of back into the firmware. This
would require the user to re-flash the firmware again to get into the firmware.
That wouldn't be so bad except for the fact that once this occurs, every reset
would require the user to re-flash the firmware again. The only way to really
fix this issue was to re-flash the bootloader itself, which is a huge pain.

Going into this, all we knew was that this issue was most likely introduced
somewhere in the [#133](https://github.com/zmkfirmware/zmk/pull/133), which
added Bluetooth profile management. We've had quite a few attempts at trying to
recreate the issue, but we never were able to get it to happen consistently.

## Diagnosing the issue

This issue had been happening sporadically for the past month, and I finally
decided to dig in to see what was going on. We started in the Discord and
discussed what was common between all of the people who have experienced this
issue. Everyone who had this issue reported that they did quite a bit of profile
switching. This lined up with the possible connection to the Bluetooth profile
management pull request.

### Pinpointing the cause

I had a hunch that this was related to the settings system. The settings system
is used by profile Bluetooth switching, and the settings system works directly
with the system flash. Based on this hunch, I tried spamming the RGB underglow
cycle behavior on my main keyboard. Sure enough after a couple minutes, I got
stuck in the bootloader. I was even able to reproduce it again.

This was an important discovery for two reasons. First, I was able to recreate
the issue consistently, which meant I could set up logging and more closely
monitor what the board was doing. Second, this more or less proved that it was
specifically the settings system at fault. Both Bluetooth profile switching and
RGB underglow cycling trigger it, and the one common piece is they save their
state to settings.

### Settings system overview

To understand what's going wrong, we first need to understand how the settings
system works. Here's a diagram to explain the flash space that the settings
system holds for our nRF52840 based boards (nice!nano, nRFMicro, BlueMicro).

![Settings Diagram](https://i.imgur.com/DF2t3Oq.png)

The settings flash space lives at the end of the flash of the chip. In this case
it starts at `0xF8000` and is `0x8000` bytes long, which is 32KB in more
comprehensible units. Then due to the chip's architecture, this flash space is
broken into pages, which are `0x1000` bytes in size (4KB).

The backend that carries out the settings save and read operation in ZMK is
called NVS. NVS calls these pages sectors. Due to how flash works, you can't
write to the same bytes multiple times without erasing them first, and to erase
bytes, you need to erase the entire sector of flash. This means when NVS writes
to the settings flash if there's no erased space available for the new value, it
will need to erase a sector.

### Logging discoveries

So first I enabled logging of the NVS module by adding
`CONFIG_NVS_LOG_LEVEL_DBG=y` to my `.conf` file. I repeated the same test of
spamming RGB underglow effect cycle and the resulting logs I got were this:

```log
[00:00:00.000,671] <inf> fs_nvs: 8 Sectors of 4096 bytes
[00:00:00.000,671] <inf> fs_nvs: alloc wra: 3, f70
[00:00:00.000,671] <inf> fs_nvs: data wra: 3, f40
// A bunch of effect cycle spam
[00:02:34.781,188] <dbg> fs_nvs: Erasing flash at fd000, len 4096
// A bunch more effect cycle spam
[00:06:42.219,970] <dbg> fs_nvs: Erasing flash at ff000, len 4096
// A bunch more effect cycle spam
// KABOOM - bootloader issue
```

So at start up, we can see that the 8 sectors of 4KB are found by NVS properly,
however, I wasn't sure what the second and third lines meant, but we'll get back
to that. Nonetheless the next two logs from NVS showed erasing the sector at
`0xFD000` and then erasing the `0xFF000` sector.

![Erased Sectors](https://i.imgur.com/DmLycMJ.png)

It's really odd that the third to last sector and the last sector are erased,
and then shortly after the bootloader issue is hit. I really had no explanation
for this behavior.

### Reaching out to Zephyr

At this point, I nor anyone else working on the ZMK project knew enough about
NVS to explain what was going on here. [Pete
Johanson](https://github.com/petejohanson), project founder, reached out on the
Zephyr Project's Slack (ZMK is built on top of Zephyr if you weren't aware).
Justin B and Laczen assisted by first explaining that those `alloc wra` and
`data wra` logs from earlier are showing what data NVS found at startup.

More specifically, `data wra` should be `0` when it first starts up on a clean
flash. As we can see from my earlier logging on a clean flash I was instead
getting `f40`. NVS is finding data in our settings sectors when they should be
blank! We were then given the advice to double check our bootloader.

### The Adafruit nRF52 Bootloader

Most of the boards the contributors of ZMK use have the [Adafruit nRF52
Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader), which allows
for extremely easy flashing by dragging and dropping `.uf2` files onto the board
as a USB drive. Every bootloader takes up a portion of the flash, and in the
README explains that the first `0x26000` is reserved for the bootloader with the
nRF52840, and we've properly allocated that.

However, there isn't a full explanation of the flash allocation of the
bootloader in the README. There's a possibility that the bootloader is using
part of the same flash area we're using. I reached out on the Adafruit Discord,
and [Dan Halbert](https://github.com/dhalbert) pointed me towards the [linker
map](https://github.com/adafruit/Adafruit_nRF52_Bootloader/blob/master/linker/nrf52840.ld)
of the nRF52840. Let's take a look.

```linker-script
FLASH (rx) : ORIGIN = 0xF4000, LENGTH = 0xFE000-0xF4000-2048 /* 38 KB */

BOOTLOADER_CONFIG (r): ORIGIN = 0xFE000 - 2048, LENGTH = 2048

/** Location of mbr params page in flash. */
MBR_PARAMS_PAGE (rw) : ORIGIN = 0xFE000, LENGTH = 0x1000

/** Location of bootloader setting in flash. */
BOOTLOADER_SETTINGS (rw) : ORIGIN = 0xFF000, LENGTH = 0x1000
```

Here's a diagram to show this a bit better.

![Adafruit Bootloader Diagram](https://i.imgur.com/TEOA31m.png)

We've found the issue! As you can see from the red bar (representing our
settings flash area), we've put the settings flash area _right on top_ of the
Adafruit bootloader's flash space. Oops!

This also shines some light on why NVS erased `0xFD000` and `0xFF000` sectors.
It's possible there was no flash written to `0xFD000` because the bootloader
didn't use up all of that space it has, and then there possibly weren't any
bootloader settings set yet, so `0xFF000` could be used and erased by NVS too.

After erasing `0xFF000`, NVS probably next erased a rather important part of the
bootloader that resulted in this issue at hand. In my opinion, we're pretty
lucky that it didn't delete an even more vital part of the bootloader. At least
we could still get to it, so that we could re-flash the bootloader easily!

## The solution

Now that we've found the issue, we can pretty easily fix this. We'll need to
move the settings flash area back so that it doesn't overlap with the
bootloader. First we calculate the size of the of flash area the bootloader is using.

```linker-script
0x100000 (end of flash) - 0x0F4000 (start of bootloader) = 0xC000 (48KB)
```

So the bootloader is using the last 48KB of the flash, this means all we need to
do is shift back the settings area and code space `0xC000` bytes. We'll apply
this to all of the `.dts` files for the boards that were affected by this issue.

```diff
        code_partition: partition@26000 {
-           reg = <0x00026000 0x000d2000>;
+           reg = <0x00026000 0x000c6000>;
        };


-       storage_partition: partition@f8000 {
+       storage_partition: partition@ec000 {
-           reg = <0x000f8000 0x00008000>;
+           reg = <0x000ec000 0x00008000>;
        };
```

And with those changes, we should no longer run into this issue! In the process
of these changes, we lost 48KB of space for application code, but we're only
using around 20% of it anyways. 🎉

## Article Updates

- 12/2023: Removed the deprecated `label` property from code snippets.
