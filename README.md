# infra/config branch

This branch contains chromium project-wide configurations
for chrome-infra services.
For example, [cr-buildbucket.cfg](cr-buildbucket.cfg) defines builders.

## Making changes

It is recommended to have a separate checkout for this branch, so switching
to/from this branch does not populate/delete all files in the master branch.

Initial setup:

```bash
mkdir config
cd config
git init
git remote add origin https://chromium.googlesource.com/chromium/src
git fetch origin infra/config
git reset --hard origin/infra/config
git config depot-tools.upstream origin/infra/config
```

Now you can create a new branch to make changes:

```
git new-branch add-new-builder
# edit cr-buildbucket.cfg
git commit -A
git cl upload
```
