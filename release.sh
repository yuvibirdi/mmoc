#!/bin/bash

# Release script for MMOC
set -e

VERSION=${1:-"0.1.0"}

if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Usage: $0 [version]"
    echo "Example: $0 0.1.0"
    echo "Version must be in format X.Y.Z"
    exit 1
fi

TAG="v$VERSION"

echo "Creating release $TAG"

# Check if tag already exists
if git rev-parse "$TAG" >/dev/null 2>&1; then
    echo "Tag $TAG already exists!"
    exit 1
fi

# Make sure we're on main branch
CURRENT_BRANCH=$(git branch --show-current)
if [[ "$CURRENT_BRANCH" != "main" ]]; then
    echo "Warning: You're on branch '$CURRENT_BRANCH', not 'main'"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Make sure working directory is clean
if [[ -n $(git status --porcelain) ]]; then
    echo "Working directory is not clean. Please commit or stash changes."
    exit 1
fi

# Run tests to make sure everything works
echo "Running tests..."
if [[ ! -d build ]]; then
    echo "Build directory not found. Running setup and build..."
    ./setup.sh
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
fi

python3 tests/test_runner.py

echo "All tests passed!"

# Create and push tag
git tag -a "$TAG" -m "Release $TAG"
git push origin "$TAG"

echo "Tag $TAG created and pushed!"
echo "GitHub Actions will now build and create the release."
echo "Check: https://github.com/$(git config --get remote.origin.url | sed 's/.*github.com[:/]\([^/]*\/[^/]*\).*/\1/' | sed 's/\.git$//')/actions"