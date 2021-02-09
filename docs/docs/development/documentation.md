---
title: Documentation
sidebar_label: Documentation
---

This document outlines how to test your documentation changes locally and prepare the changes for a pull request.

The documentation is written with [Docusaurus](https://docusaurus.io/). The ZMK source code has all of the necessary Docusaurus dependencies included but referencing their documentation can be helpful at times.

The general process for updating the ZMK documentation is:

1. Update the documentation
2. Test the changes locally
3. Ensure the sources are formatted properly and linted
4. Create a Pull Request for review and inclusion into the ZMK sources

:::note
If you are working with the documentation from within VS Code+Docker please be aware the documentation will not be auto-generated when making changes while the server is running. You'll need to restart the server when saving changes to the documentation.
:::

:::note
You will need `Node.js` and `npm` installed to update the documentation. If you're using the ZMK dev container (Docker) the necessary dependencies are already installed.
:::

## Testing Documentation Updates Locally

To verify documentation updates locally, follow the following procedure. The `npm` commands and first step will need to be run from a terminal.

1. Navigate to the `docs` folder
2. Run `npm ci` to build the necessary tools if you haven't run it before or the ZMK sources were updated
3. Run `npm start` to start the local server that will let you see your documentation updates via a web browser
4. If a web browser doesn't open automatically: you'll need to open a browser window or tab and navigate to `http://localhost:3000` to view your changes
5. Verify the changes look good

## Formatting and Linting Your Changes

Prior to submitting a documentation pull request, you'll want to run the format and check commands. These commands are run as part of the verification process on pull requests so it's good to run them ahead of submitting documentation updates.

The format command can be run with the following procedure in a terminal that's inside the ZMK source directory.

1. Navigate to the `docs` folder
2. Run `npm run prettier:format`

The check commands can be run with the following procedure in a terminal that's inside the ZMK source directory.

1. Navigate to the `docs` folder
2. Run `npm run prettier:check`
3. Run `npm run lint`
4. Run `npm run build`

:::warning
If any of the above steps throw an error, they need to be addressed and all of the checks re-run prior to submitting a pull request.
:::

## Submitting a Pull Request

Once the above sections are complete the documentation updates are ready to submit as a pull request.
