---
title: "Zephyr 3.0 Update Preparation"
authors: petejohanson
tags: [firmware, zephyr, core]
---

As preparation for completing the [work](https://github.com/zmkfirmware/zmk/pull/1143) to upgrade ZMK to [Zephyr 3.0](https://docs.zephyrproject.org/3.0.0/releases/release-notes-3.0.html), users with user config repositories who wish to avoid future build failures with their GitHub Actions workflows can take steps to adjust
their repositories now.

<!-- truncate -->

GitHub Actions needs to use our latest Docker image to ensure continued compatibility with the ZMK codebase on Zephyr 3.0 (and beyond). You should:

- Open `.github/workflows/build.yml` in your editor/IDE
- Change `zmkfirmware/zmk-build-arm:2.5` to `zmkfirmware/zmk-build-arm:stable` wherever it is found

Once the changes are committed and pushed, the build will run as expected.

A future blog post will outline the complete Zephyr 3.0 changes once that work is finalized.

:::note

If you created your user config repository a while ago, you may find that your `build.yml` file instead references
a `zephyr-west-action-arm` custom GitHub Action instead. In this case, the upgrade is not as direct. We suggest that
instead you [re-create your config repository](/docs/user-setup) to get an updated setup using the new automation
approach.

:::
