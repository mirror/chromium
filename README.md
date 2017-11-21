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
git remote add -t infra/config origin https://chromium.googlesource.com/chromium/src
git fetch origin
git config depot-tools.upstream origin/infra/config
# Using "master" is a misnomer here -- it's tracking the remote infra/config branch,
# not master -- but "git cl upload" special-cases the "master" branch.
git checkout -b master origin/infra/config
```

Now you can create a new branch to make changes:

```
git checkout master
# The --track option is needed to make CL uploads work.
# Alternatively, you can use "git branch --set-upstream-to" later.
git checkout -b your_branch_name --track
# You could also do:
# git checkout origin/infra/config
# git new-branch --upstream_current your_branch_name

# edit cr-buildbucket.cfg
git commit -a
git cl upload
```

To update your branches:

```
git checkout master
git pull
git rebase master your_branch_name
```
