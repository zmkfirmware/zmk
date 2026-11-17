---
title: Documentation
sidebar:
  label: Documentation
---

:::danger
Before reading this section, it is **vital** that you read through our [clean room policy](clean-room.md).
:::

This document outlines how to test your documentation changes locally and prepare the changes for a pull request.

The documentation is written with [Astro Starlight](https://starlight.astro.build/). The ZMK source code has all of the necessary Astro and Starlight dependencies included, but referencing their documentation can be helpful at times.

The website is built using the Node.js version pinned in `docs/.nvmrc`, which is available at <https://nodejs.org/en/download/>. The website uses `pnpm` as its package manager instead of `npm`, which is also available at <https://nodejs.org/en/download/>, or can be installed via <https://pnpm.io/installation>.

The general process for updating the ZMK documentation is:

1. Update the documentation
2. Test the changes locally
3. Ensure the sources are formatted properly and linted
4. Create a Pull Request for review and inclusion into the ZMK sources

## Testing Documentation Updates Locally

To verify documentation updates locally, follow the following procedure. The `pnpm` commands and first step will need to be run from a terminal.

1. Navigate to the `docs` folder
2. Run `pnpm install` to install the necessary dependencies
3. Run `pnpm dev` to start the local development server
4. Navigate to `http://localhost:4321` to view your changes
5. Verify the changes look good

## Formatting and Linting Your Changes

Prior to submitting a documentation pull request, you'll want to run the format and check commands. These commands are run as part of the verification process on pull requests so it's good to run them ahead of submitting documentation updates.

The format command can be run with the following procedure in a terminal that's inside the ZMK source directory.

1. Navigate to the `docs` folder
2. Run `pnpm prettier:format`

The check commands can be run with the following procedure in a terminal that's inside the ZMK source directory.

1. Navigate to the `docs` folder
2. Run `pnpm prettier:check`
3. Run `pnpm lint`
4. Run `pnpm build`
5. Run `pnpm check`

If any of the above steps throw an error, they need to be addressed and all of the checks re-run prior to submitting a pull request.

The documentation uses American English spelling and grammar conventions. Title case is used for the first three heading levels, with sentence case used beyond that. Please make sure your changes conform to these conventions.

## Submitting a Pull Request

Once the above sections are complete the documentation updates are ready to submit as a pull request.
