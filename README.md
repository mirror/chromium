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
git config --add remote.origin.fetch '+refs/heads/infra/config:refs/remotes/origin/master'
git fetch
```

Now you can create a new branch (or more than one) to make changes:

```
git new-branch some_feature
# edit cr-buildbucket.cfg
git commit -a
git cl upload
```

To rebase your local branches on top of upstream:

```
git rebase-update
```
