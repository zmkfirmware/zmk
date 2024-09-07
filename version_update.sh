#!/bin/bash

# Source the VERSION file
source VERSION

# Check the latest commit message
COMMIT_MSG=$(git log -1 --pretty=%B)

# Initialize variables for version update
UPDATE_REQUIRED=false

# Determine if a version bump is needed
if [[ "$COMMIT_MSG" =~ SKIP\ VERSIONING ]]; then
    echo "RUN_TAG_RELEASE=false" >> $GITHUB_ENV
    exit 0
fi

if [[ "$COMMIT_MSG" =~ ^!|!:|BREAKING\ CHANGE ]]; then
    ((MAJOR+=1))
    MINOR=0
    PATCH=0
    UPDATE_REQUIRED=true
elif [[ "$COMMIT_MSG" == feat* ]]; then
    ((MINOR+=1))
    PATCH=0
    UPDATE_REQUIRED=true
elif [[ "$COMMIT_MSG" == fix* ]]; then
    ((PATCH+=1))
    UPDATE_REQUIRED=true
fi

if [ "$UPDATE_REQUIRED" = true ]; then
    # Update VERSION file
    cat <<EOL > VERSION
MAJOR=$MAJOR
MINOR=$MINOR
PATCH=$PATCH
EOL

    # commit and push changes
    git add VERSION
    git commit -m "Update version to $MAJOR.$MINOR.$PATCH"
    git push origin main

    echo "Version updated to $MAJOR.$MINOR.$PATCH"
    echo "RUN_TAG_RELEASE=true" >> $GITHUB_ENV
    exit 0
fi

echo "RUN_TAG_RELEASE=false" >> $GITHUB_ENV

