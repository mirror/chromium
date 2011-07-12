# Run this script in the mozilla/security/nss/lib directory in a NSS source
# tree.
#
# Point patches_dir to the src/net/third_party/nss/patches directory in a
# chromium source tree.
patches_dir=/Users/wtc/chrome1/src/net/third_party/nss/patches

patch -p6 < $patches_dir/nextproto.patch

patch -p6 < $patches_dir/versionskew.patch

patch -p6 < $patches_dir/renegoscsv.patch

patch -p6 < $patches_dir/cachecerts.patch

patch -p6 < $patches_dir/peercertchain.patch

patch -p6 < $patches_dir/ocspstapling.patch

patch -p6 < $patches_dir/clientauth.patch

patch -p6 < $patches_dir/cachedinfo.patch

patch -p6 < $patches_dir/didhandshakeresume.patch

patch -p6 < $patches_dir/cbcrandomiv.patch
