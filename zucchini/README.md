Zucchini: A New patching algorithm inspired by Courgette and BSDiff

# Usage

Zucchini uses BUILD or BUILD.gn and has dependancies on Chromium base.

## Using Blaze

I did not manage to make a build configuration that works with Chromium base
yet. Therefore, you won't be able to have zucchini as a command line utility.
You can build zucchini_lib and experiment with it though.

'blaze build experimental/users/etiennep/zucchini:zucchini_lib'

## Using gn

One quick way to setup Zucchini is to copy the entire directory to chromium/src/
and modify chromium/src/BUILD.gn to add Zucchini as a dependancy.

Example :
```
group("gn_only") {
  testonly = true

  deps = ["//zucchini:zucchini",]
```

To Build (release)
'gn gen out/Release'
'ninja -C out/Release zucchini'

To generate patch:
'zucchini -gen old new patch'

To apply patch:
'zucchini -apply old patch new'
