---
title: Pull Requests
---

Changes to ZMK's `main` branch are all done through pull requests, even for core committers. The following will help ensure an easier PR/review process for all involved.

## Clean Commit History

Before opening a PR, ensure your branch has a clean commit history, which allows our release automation to generate clean changelogs, and allows easier understanding of our commit history when investigating regressions, etc.

A "clean" commit history satisfies the following:

- Commit messages follow our [commit message conventions](#commit-messages).
- The sequence of commits should be well organized, with discrete commits used to break any combined work into smaller, cohesive changes if the scope of a given PR/change is larger. This allows for easier code review. You can usually accomplish this by amending (e.g. `git add foo && git commit --amend`) or by interactive rebasing (e.g. `git rebase -i upstream/main`) and squashing/rewording commits.
- Do not use merge commits to catch up with ZMK `main`, instead using rebasing to address any issues with divergence (e.g. `git rebase upstream/main`).

### Commit Messages

The ZMK project uses [conventional commits](https://www.conventionalcommits.org/) for their commit messages. This not only provides consistency for our commits, but also allows for release/versioning automation to determine the next version to release, generating changelogs, etc.

Commit messages will be checked as part of our CI process by GitHub Actions.

#### Guidelines

Commits should have the following:

- A first line prefix that includes a [type](#types), as well as appropriate [scope](#scopes) in parentheses as needed.
- Following the prefix, a concise summary of the change, which documents the new behavior/feature/functionality in the positive (e.g. "wake from sleep now works with charlieplex kscan", not "fixed waked from sleep bug with charlieplex kscan driver").
- A blank line following the first line.
- A body that provides more detail of the changes. This _may_ be a bulleted list or paragraph prose.
- An optional set of [git trailers](https://git-scm.com/docs/git-interpret-trailers#_description) for things like [GitHub keywords](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/using-keywords-in-issues-and-pull-requests) following a blank line.

#### Example

Here is an example of a good commit message:

```
feat(boards): Add numpad layouts

Added physical layouts for the following variants of numpads:

- With and without extra top row
- 2U plus key or 1U plus and backspace keys
- 2U 0 key or 1U 0 and 00 keys
- Full 1U grid/macropad layout

Other layouts exist, such as "southpaw" horizontally mirrored layouts,
and layouts with a fifth column, but those seem to be much less common.
```

#### Pre-Commit

To help make sure you don't need to wait for GitHub Actions to check your commits, you can [set up pre-commit](../local-toolchain/pre-commit.md) to check your commits as you create them.

#### Types

The following commit types are used by ZMK:

- `blog:` -- changes to our documentation found in the `docs/blog` directory
- `docs:` -- changes to our documentation found in the `docs/` directory, except blogs
- `feat:` -- changes that add a new feature
- `fix:` -- changes that fix existing functionality
- `refactor:` -- changes that refactor existing functionality without adding any new features
- `feat!:`/`refactor!:`/`fix!:` -- same as above, but indicates a breaking change. Examples would be changes to the public C API, renaming a board/shield, editing a board or shield to rename devicetree labels that may be used in keymaps, etc.
- `ci:` -- changes to our continuous integration setup with GitHub Actions, usually only for the files in `.github/workflows/`
- `chore:` -- grab bag type for small changes that don't fall into any of the above categories, including dependency updates for development tools and docs.

#### Scopes

The following scopes are frequently used to further clarify the scope of the change:

- `hid` -- changes to our general HID code
- `usb` -- changes specific to USB
- `ble` -- changes specific to BLE
- `power` -- changes to our power management code
- `split` -- changes to our split keyboard support
- `studio` -- changes to our ZMK Studio code
- `display` -- changes to to our display code
- `underglow` -- changes to to our RGB underglow support
- `backlight` -- changes to to our simple LED backlight support
- `behaviors` -- changes to to our core behavior code
- `core` -- changes to any other area of our core code
- `boards` -- changes to the in-tree boards
- `shields` -- changes to the in-tree shields

## Opening a PR

Create a PR by visiting https://github.com/zmkfirmware/zmk/pulls and clicking "New pull request" and selecting your branch. GitHub should auto-populate a start of a description/body to your PR, but please make sure to supplement that with additional details about the change and make sure the check-list is complete in the PR template.

Once created, the PR should automatically have reviewers assigned.

## Review

Depending on the area of change, different ZMK team members will review the PR. The ZMK project is a small team, so please be patient with the review timeline. You are welcome to send a polite request/nudge on our Discord server to draw attention to your PR if it has not gotten a response after a reasonable amount of time.

## Merging

Maintainers merging PRs will perform the following steps:

1. Verify that the expected tests have all run, and are passing
1. Inspect the commits in the PR to confirm the commit messages not only are proper conventional commits, but have accurate descriptions and are using the correct type for the set of files changed by the commit(s).
1. Merge the PR using a squash-merge. If the PR includes multiple commits, then the squash message will be fixed up manually to remove the `* ` prefixes from each message section to ensure [release-please](https://github.com/googleapis/release-please) properly will parse the compound commit message.
1. Confirm the new/existing release PR is updated properly with the expected version number and changelog.
