/**
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */
import assert from "node:assert/strict";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { test } from "node:test";

import {
  compareReleaseVersions,
  createVersionInfo,
  normalizeBranchName,
  normalizeReleaseVersions,
  parseReleaseBranch,
  resolveVersionInfo,
} from "./version-info.ts";

test("normalizeBranchName accepts the forms hosts and git produce", () => {
  assert.equal(normalizeBranchName("v0.3-branch"), "v0.3-branch");
  assert.equal(normalizeBranchName(" v0.3-branch "), "v0.3-branch");
  assert.equal(normalizeBranchName("refs/heads/v0.3-branch"), "v0.3-branch");
  assert.equal(
    normalizeBranchName("refs/remotes/origin/v0.4-branch"),
    "v0.4-branch"
  );
  assert.equal(
    normalizeBranchName("remotes/origin/v0.4-branch"),
    "v0.4-branch"
  );
  assert.equal(normalizeBranchName("origin/v0.4-branch"), "v0.4-branch");
  assert.equal(normalizeBranchName("HEAD"), "");
  assert.equal(normalizeBranchName(""), "");
  assert.equal(normalizeBranchName(undefined), "");
});

test("parseReleaseBranch recognizes only vX.Y-branch", () => {
  assert.equal(parseReleaseBranch("v0.3-branch"), "0.3");
  assert.equal(parseReleaseBranch("v1.10-branch"), "1.10");
  assert.equal(parseReleaseBranch("main"), null);
  assert.equal(parseReleaseBranch("v0.3"), null);
  assert.equal(parseReleaseBranch("v0.03-branch"), null);
});

test("normalizeReleaseVersions sorts, de-duplicates, and validates", () => {
  assert.deepEqual(normalizeReleaseVersions(["0.3", "0.4", "0.3"]), [
    "0.4",
    "0.3",
  ]);
  assert.deepEqual(normalizeReleaseVersions(["0.9", "0.10", "1.0"]), [
    "1.0",
    "0.10",
    "0.9",
  ]);
  assert.deepEqual(normalizeReleaseVersions([]), []);
  assert.throws(() => normalizeReleaseVersions("0.3"), /must be an array/);
  assert.throws(
    () => normalizeReleaseVersions(["0.3.1"]),
    /expected an "X.Y" string/
  );
  assert.throws(
    () => normalizeReleaseVersions([0.3]),
    /expected an "X.Y" string/
  );
});

test("compareReleaseVersions compares major and minor numerically", () => {
  assert.deepEqual(["0.9", "0.10", "1.0"].sort(compareReleaseVersions), [
    "1.0",
    "0.10",
    "0.9",
  ]);
});

test("development builds select Pre-Release", () => {
  const info = createVersionInfo({ branch: "main", releaseVersions: ["0.3"] });

  assert.equal(info.branch, "main");
  assert.equal(info.isDevelopmentVersion, true);
  assert.equal(info.versionDropdownLabel, "Pre-Release");
  assert.deepEqual(info.releaseVersions, ["0.3"]);
  assert.deepEqual(
    info.versionNavbarItems.map(({ label, current }) => ({ label, current })),
    [
      { label: "Pre-Release", current: true },
      { label: "v0.3", current: false },
    ]
  );
});

test("a configured release branch selects its own version", () => {
  const info = createVersionInfo({
    branch: "v0.3-branch",
    releaseVersions: ["0.3"],
  });

  assert.equal(info.isDevelopmentVersion, false);
  assert.equal(info.versionDropdownLabel, "v0.3");
  assert.equal(info.currentVersion.current, true);
  assert.equal(info.currentVersion.href, "https://v0-3-branch.zmk.dev/docs");
});

test("a release branch missing from the data file is still selected", () => {
  const info = createVersionInfo({
    branch: "v0.4-branch",
    releaseVersions: ["0.3"],
  });

  assert.equal(info.isDevelopmentVersion, false);
  assert.equal(info.versionDropdownLabel, "v0.4");
  assert.deepEqual(info.releaseVersions, ["0.4", "0.3"]);
  assert.deepEqual(
    info.versionNavbarItems.map(({ label, current }) => ({ label, current })),
    [
      { label: "Pre-Release", current: false },
      { label: "v0.4", current: true },
      { label: "v0.3", current: false },
    ]
  );
  assert.equal(
    info.versionNavbarItems.find((item) => item.label === "v0.4")?.href,
    "https://v0-4-branch.zmk.dev/docs"
  );
});

test("an older release branch keeps its own selection in a newer list", () => {
  const info = createVersionInfo({
    branch: "v0.3-branch",
    releaseVersions: ["0.4", "0.3"],
  });

  assert.equal(info.versionDropdownLabel, "v0.3");
  assert.deepEqual(info.releaseVersions, ["0.4", "0.3"]);
  assert.equal(
    info.versionNavbarItems.find((item) => item.label === "v0.3")?.current,
    true
  );
});

test("a version branch is normalized before it is parsed", () => {
  const info = createVersionInfo({
    branch: "refs/heads/v0.3-branch",
    releaseVersions: ["0.3"],
  });

  assert.equal(info.branch, "v0.3-branch");
  assert.equal(info.versionDropdownLabel, "v0.3");
});

test("major versions and empty release lists are supported", () => {
  const major = createVersionInfo({
    branch: "v1.0-branch",
    releaseVersions: [],
  });
  assert.equal(
    major.versionNavbarItems.find((item) => item.label === "v1.0")?.href,
    "https://v1-0-branch.zmk.dev/docs"
  );

  const empty = createVersionInfo({ branch: "main", releaseVersions: [] });
  assert.deepEqual(
    empty.versionNavbarItems.map((item) => item.label),
    ["Pre-Release"]
  );
});

test("resolveVersionInfo prefers the build environment and reads the data file", () => {
  const dir = mkdtempSync(join(tmpdir(), "zmk-version-info-"));
  const file = join(dir, "release-versions.json");
  writeFileSync(file, JSON.stringify({ releaseVersions: ["0.3"] }));

  try {
    const info = resolveVersionInfo({
      env: { BRANCH: "refs/heads/v0.4-branch", GITHUB_REF_NAME: "main" },
      cwd: join(dir, "missing"),
      releaseVersionsFile: file,
    });

    assert.equal(info.versionDropdownLabel, "v0.4");
    assert.deepEqual(info.releaseVersions, ["0.4", "0.3"]);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("resolveVersionInfo falls back to development when no branch source exists", () => {
  const dir = mkdtempSync(join(tmpdir(), "zmk-version-info-"));
  const file = join(dir, "release-versions.json");
  writeFileSync(file, JSON.stringify({ releaseVersions: ["0.3"] }));

  try {
    const info = resolveVersionInfo({
      env: {},
      cwd: join(dir, "missing"),
      releaseVersionsFile: file,
    });

    assert.equal(info.branch, "main");
    assert.equal(info.versionDropdownLabel, "Pre-Release");
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
