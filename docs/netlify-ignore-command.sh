#!/bin/sh

# We deploy main, version branches and deploy previews only
if echo "$HEAD" | grep -qE '^(main|v[[:digit:]]+\.[[:digit:]]+-branch)$' \
   || echo "$CONTEXT" | grep -q '^deploy-preview$'
then
  echo "Head '$HEAD' matches a deployed branch, or context is deploy preview, checking for git changes."

  if [ "$CACHED_COMMIT_REF" = "$COMMIT_REF" ]; then
    echo "No cached build to diff against (cache miss or rebuild), proceeding with build."
    exit 1
  fi

  git diff --quiet "$CACHED_COMMIT_REF" "$COMMIT_REF" . ../app/boards/
  exit $?
else
  echo "Head '$HEAD' does not match a deployed branch and is not a deploy preview"
  exit 0
fi
