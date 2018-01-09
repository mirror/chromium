# meta/config ref
This ref contains chromium project-wide configurations
for gerrit and gerrit-related things.

## Updating refs/meta/config from a checkout
Follow these steps to create a git checkout which is dedicated to manipulating refs/meta/config of src.

```bash
export REPO=chromium/src
mkdir -p meta/$REPO
cd meta/$REPO
git init
git remote add origin https://chromium.googlesource.com/$REPO
git config --unset-all remote.origin.fetch
git config --add remote.origin.fetch refs/meta/config:refs/remotes/origin/refs/meta/config
git config --add remote.origin.fetch refs/meta/config:refs/meta/config
git fetch origin
git checkout -t origin/refs/meta/config -b my-edit
```

Now you can treat it just like a normal repo. You can use rebase-update to update and clean up your branches, new-branch to create new ones, map-branches to view your local branches and their review states, and upload to upload for review.
