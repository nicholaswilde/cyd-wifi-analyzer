#!/usr/bin/env bash
set -e

# Pull latest changes and tags
echo "Fetching latest tags and commits from origin..."
git fetch --tags origin
git pull --rebase origin main

# Extract and calculate next version
LATEST_TAG=$(git tag --sort=-v:refname 2>/dev/null | head -n 1)

if [ -z "$LATEST_TAG" ]; then
    if [ -f version.txt ]; then
        BASE_VER=$(tr -d '[:space:]' < version.txt)
        BASE_VER=${BASE_VER#v}
        if [ -n "$BASE_VER" ]; then
            CURRENT_VER="$BASE_VER"
        else
            CURRENT_VER="0.1.0"
        fi
    else
        CURRENT_VER="0.1.0"
    fi
    NEW_TAG="v$CURRENT_VER"
    echo "No existing git tags found. Initializing first release: $NEW_TAG"
else
    CURRENT_VER=${LATEST_TAG#v}
    IFS='.' read -ra VER_PARTS <<< "$CURRENT_VER"
    PATCH=${VER_PARTS[2]:-0}
    NEW_PATCH=$((PATCH + 1))
    NEW_VER="${VER_PARTS[0]}.${VER_PARTS[1]}.$NEW_PATCH"
    NEW_TAG="v$NEW_VER"
    echo "Current version: $LATEST_TAG"
    echo "Target version: $NEW_TAG"
fi

echo "Running pre-release validation (check & test)..."
task check
task test

echo "Validation passed. Bumping version.txt..."
echo "$NEW_TAG" > version.txt
git add version.txt
git commit -m "chore(release): bump version to $NEW_TAG"

echo "Checking git status..."
if [ -n "$(git status --porcelain)" ]; then
    echo "Error: Working directory not clean after bumping version."
    exit 1
fi

echo "Tagging release..."
git tag -a "$NEW_TAG" -m "Release $NEW_TAG"

echo "Pushing atomically..."
git push --atomic origin main "$NEW_TAG"

echo "Release $NEW_TAG published successfully!"
