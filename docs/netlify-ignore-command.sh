#!/bin/sh

# We deploy main, version branches and deploy previews only
if echo $HEAD | grep -E '^(main|v[[:digit:]]+\.([[:digit:]])+-branch)$' || echo $CONTEXT | grep '^deploy-preview$'
then
  echo "Head '$HEAD' matches a deployed branch, or context is deploy preview, checking for git changes."
  git diff --quiet $CACHED_COMMIT_REF $COMMIT_REF . ../app/boards/
else
  echo "Head '$HEAD' does not match a deployed branch and is not a deploy preview"
  exit 0
fi