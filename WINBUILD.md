# My notes from trying to cross-compile for Windows

The vs_files need to be mounted via a case insensitive file system since the
paths in the Windows SDK mix case.

```sh
cd depot_tools/win_toolchain

# Get the digest of the current SDK.
HASH=$(basename $(dirname $(jq -r ".win_sdk" data.json)))

# Check how big the SDK is.
du -k vs_files/${HASH}

# Create a disk image. This assumes that the SDK is comfortably under 3G in
# size. Increase the count if this is not the case.
dd if=/dev/zero of=vs.dsk bs=1M count=3K

# Format the disk image using VFAT.
mkfs.vfat vs.dsk

# Now mount the image in place of the original SDK directory.
cd vs_files
mv ${HASH} ${HASH}.src
mkdir ${HASH}
sudo mount vs.dsk ${PWD}/${HASH} -t vfat -o loop,rw,users,uid=${UID}

# And copy the SDK files to the new one.
cp -r ${HASH}.src/* ${HASH}
```

In order to get code indexing working for Windows, we could do the following:

* Figure out a way to generate all Windows sources.

  - Generate a compilation database.
  - For each source file that's missing, invoke `ninja sourcefile` to generate
    it.

* Either figure out the canonical command line via invoking `clang-cl -###` or
  figure out a way initiaze the `cxx_extractor` using the `clang-cl` driver.

* Generate the .kindex files.

Now we'll have two sets of kindex files. One from Linux and the other from
Windows. Then we can figure out how to represent them in the UI. Perhaps make a
union of them?

Need to figure out how we generate ticket names for symbols and files, and then
make sure the tickets are stable across platforms. Then we can identify cross
platform stuff and separate them out.

