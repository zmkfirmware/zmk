# Contributing To ZMK

Thanks for taking an interest in contributing to ZMK! After reading through the documentation, if
you have any questions, please come join us on the
[ZMK Discord Server][discord-invite].

## Code of Conduct

All community members are expected to abide by the [Code of Conduct][code-of-conduct].
For any and all conduct inquiries or concerns, please contact conduct@zmkfirmware.dev.

[code-of-conduct]: https://github.com/zmkfirmware/zmk/blob/main/CODE_OF_CONDUCT.md

## How Can I Contribute

There are many different ways that you can contribute to ZMK, several of which require no coding
abilities. These include:

- Chat Support
- Issue Reporting/Commenting
- Testing
- Documentation
- Code Contributions

## Chat Support

Providing user support on the [ZMK Discord Server][discord-invite] is a great way to help the
project. In particular, answering questions in the [#help](https://discord.com/channels/719497620560543766/719909884769992755) channel is incredibly appreciated.

## Issue Reporting/Commenting

Often, you might encounter unexpected behavior when building, flashing, or running the ZMK
firmware. Submitting or commenting on issues on GitHub is a great way to contribute to the
ZMK project.

### Before Submitting a Report

- Review the [Frequently Asked Questions](https://zmkfirmware.dev/docs/faq).
- Check the [Troubleshooting Guide](https://zmkfirmware.dev/docs/troubleshooting) for answers.
- Search the [open issues](https://github.com/zmkfirmware/zmk/issues) for an existing report that
  matches your problem.

### Opening A Report

To open a report:

- Head to https://github.com/zmkfirmware/zmk/issues/new
- Provide an accurate summary of the issue in the title.
- Provide as much detail as you can about the issue including:
  - What [board/shield](https://zmkfirmware.dev/docs/faq#what-is-a-board) you are using.
  - A link to the user repository, if you used it to build your firmware.
  - Exact steps to reproduce the problem.
  - Any relevant screenshots or [logs](https://zmkfirmware.dev/docs/dev-guide-usb-logging)

## Testing

The `help wanted` label will be added to any [pull requests](https://github.com/zmkfirmware/zmk/pulls?q=is%3Aopen+is%3Apr+label%3A%22help+wanted%22)
or [issues](https://github.com/zmkfirmware/zmk/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22)
where user testing can assist the ZMK contributors to verify fixes, confirm
bugs, etc.

When providing testing feedback, please provide:

- Exact steps used to test
- Any hardware details relevant to testing
- Pass/fail summary for testing.
- Full details of any failures, including:
  - Logs
  - Screenshots

## Documentation

Quality documentation is a huge part of what makes a successful project. Contributions to add
documentation to areas not currently covered are greatly appreciated.

### Contributing

- The documentation site can be found in the main ZMK repo, in the
  [docs/](https://github.com/zmkfirmware/zmk/tree/main/docs) subdirectory.
- The documentation is maintained using [Docusaurus V2](https://v2.docusaurus.io/docs/).
- To get started, from the `docs/` directory, run `npm ci` and then `npm start`.
- Enhancements should be submitted as pull requests to the `main` branch of ZMK.

### Formatting

ZMK uses `prettier` to format documentation files. You can run prettier with `npm run prettier:format`.
You can setup git to run prettier automatically when you commit by installing the pre-commit hooks: `pip3 install pre-commit`, `pre-commit install`.

## Code Contributions

### Development Setup

To get your development environment setup going, start at the
[basic setup](https://zmkfirmware.dev/docs/dev-setup) docs, and make sure you can build and flash
your own locally built firmware.

### Formatting

ZMK uses `clang-format` to ensure consist formatting for our source code. Before submitting your
changes, make sure you've manually run `clang-format`, or have your IDE configured to auto-format
on save.

You can setup git to run `clang-format` automatically when you commit by installing the pre-commit hooks: `pip3 install pre-commit`, `pre-commit install`.

### Commit Messages

The ZMK project is working towards, but not yet enforcing, the use of
[conventional commits](https://www.conventionalcommits.org/en/v1.0.0/) for commit messages.

Further documentation and details will be provided here soon.

### Pull Requests

When opening a pull request with your changes, please:

- Submit the PR to the `main` branch of the
  [`zmkfirmware/zmk`](https://github.com/zmkfirmware/zmk) repository.
- Use a descriptive title that summarizes the change.
- In the description, include:
  - References to any open issues fixed by the PR.
  - Feature added by the PR
  - Bugs fixed by the PR.
  - Testing you've performed locally.
  - Requested testing by reviewers or testers.
  - Screenshots or logs that support understanding the change.

[discord-invite]: https://zmkfirmware.dev/community/discord/invite
