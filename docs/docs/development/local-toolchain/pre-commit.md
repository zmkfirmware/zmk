---
title: Pre-commit
---

ZMK uses [pre-commit](https://pre-commit.com/) to check for common errors and make sure the codebase is formatted consistently.

Pre-commit is run on every pull request. You can also install it locally to get the same checks run on every commit you make _before_ you submit a pull request.

## Installing pre-commit

Open a terminal and run:

```bash
pip3 install pre-commit
```

If this doesn't work, make sure [Python](https://www.python.org/) is installed and try again.

## Enabling Commit Hooks

Now that pre-commit is installed on your PC, you need to install it into the ZMK repo to enable it. Open a terminal to the ZMK repo directory and run:

```bash
pre-commit install
```

This should print a message such as

```
pre-commit installed at .git\hooks\pre-commit
```

Pre-commit will now automatically check your changes whenever you run `git commit`. If it detects a problem, it will describe the problem and cancel the commit. For simple problems such as incorrect formatting, it will also automatically fix the files so you can just `git add` them and try again.

## Automatically Enabling pre-commit

Pre-commit can be configured to automatically install itself into any newly cloned repository, so you don't have to remember to run `pre-commit install`. See the [pre-commit documentation](https://pre-commit.com/#automatically-enabling-pre-commit-on-repositories) for instructions.
