#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Takes a series of tbsCertificate files (in der-ascii format), signs and
generates certificates for each, and dumps the resulting chain.

HOWTO hack up a cert chain:
1. dump the existing chain with der2ascii (eg using print_certificates.py)
2. copy out the tbsCertificate SEQUENCE from each cert into its own file: ee.tbs, int.tbs, root.tbs
3. in each .tbs file, replace the SPKI modulus with the matching modulus from below.
4. make any other desired edits to the .tbs files
5. run this script from the same directory as the .tbs files

Note: this script currently assumes the cert chain is using RSA keys and sha256WithRSAEncryption signatures.
"""

# TODO(mattm): clean this up and actually land it?

import os
import sys
import subprocess
import errno

sys.path += [os.path.join(os.path.dirname(os.path.abspath(__file__)),'testserver')]

import asn1
from minica import RSA, SHA256_WITH_RSA_ENCRYPTION
import print_certificates
from print_certificates import read_file_to_string, process_data_with_command

root_key = RSA(0x00e7733738eee44ef2714fa26ca36bc08137b2675373b2d6bb125af476a43eaa1cb39c42ac4f938f24fa0c7c80e79d4dd57e28c62e1dae665e7b7366fd5b1c8b9e68854a40deec31768dab81352c9eaad3a51a8352b4231b8217089a70b514dc7ea8d628784e0624ca8ab8f95a0db79361fc79c7d4bb98b35d83b205c1117c6255499a9874e3da9437c8b6f9a5ee97aa779acaebb16c05ec2bd3e5410c402d7e771c11f4994a50554c271451ea6738303d2fee8e6a7378a03a0fd55b87417c27b757e7b83fa46e4b9cada41be32bfb252d2d673fecd492fa1999922b52bae0bdf93e324648f0e69fce9ed9d3311f8ccc4d193a4470f6497c7cf77d3148f674553d,
              65537,
              0x00aa2e31608f67181573f395fe437b2b24d80b39a646e02cbb88979040dc2ba7714b2f8e669c6c67484d0379585ef56f11979deb7a6520a2ecbde0f0cd417d0fbbfe26639ea74b0c639b6f8ba5c995310e45cbe4db9ed0619d99ca1da4d9c0e90fc89d0f4cc6320643571503638fca777808e892627d3d9e45185c8196a080d0db76d28d67b6c2e4fbe46785e9a1f6d6b83d92ff2b445da0b9fdc8b39424dbd9de225b27f82fb49780ca841fe585a5ca5b444c066f6fe0e5a93970b581ea82d276f4b5e414a252e5eea4823d9b06b0241bd873c563ac377b2c8e2fb9dbde0671f88fbe6151ccd90bf46654ac43401800e542929cd3262f09412d4e11bc03611491)

int_key = RSA(0x00c71c61a4f8214f6a0c5f020290dced1195cf2e56892e9ca5ad71e2eb60c1a2d16e0a56e826fc1cf98bd66429a850bbe1330283f5a0270c57b6842bbb326ac5c7851d405f0a83c7b9995b5f194a1118cf7082905b6a36a671ee2d275c924829dbd693918b1e3fa2dd61b9528680964fc3d01d8a3334c3ed88479b9faa3fb22d2dd30a8ad9ef80eae70c337f5c42b89e12ae26ac86bb86ba83435fbfc051b003ce0f264579de9c3bcb9b52b653513e1b44b05b606914476a1123be04478ec33a8ef75bc0f32e4d361f755a04247e8f49ecab634c3ea2293a5bb29ff0696364b49677a2820f6cccdb37fa7017eb662bc370f6f16d77c3a0cda6b5d1a3772f321917,
              65537,
              0x02d66eb5d12b785c448cde1477480ec0df67b27b5d3d22d261a46bdd7ae587928084f93419ca2207946c9bc4d43742563549e5e38c42c23445cfa067ec9204fb90f417c4c2fb99cf15566e06883db222d2b1a9a903dc7f86c424349967244c246bec72788f17cd14669e2384f3343132d336788fbe42c491daa367251fff44118aa36c1863e00d917b9335be889d6aa47959aa2b5d293303cef9453f085c35113420d73e5c5fe438c8b01bed71780546fecd8f55b3a288249a311542bc69e351dc28384ef2dc71025e5f05b45fcac6e8de36af802a715b1b6bdc0213afdce6aa2c46138cf80a84d9efb2c0f051bdabf2bf890c71c500690d834baf9c048ee341)

ee_key = RSA(0x00b30e5e5428ea9124f170f523c00ae7ea1c68736f388f263ad5dd2969343498289488b3322bfaf2f205e170e2a97ccdfe1ea1a066b2f6042d0d911e19e89675e3b1d91b93de9c854a5fe8fee22bf155a1265064478d0f6eb896993c27c39bd8dcce26a48377075ccf1cfdbfa5d19730a629d039e487490dc261c063b8b110cf68f32ff43c006e5eb2a6d02eb88b7f7e573e9d9b2e318e491c7fac3b8d3007da14f93abd299a189a7998ad2c9d57021555012077324e9a333f5857be7c74aeacf5f618d188e2a0598f84a1cd2ba1f094945cb5f19b3f08062b496766f1dbb21fca3743f37313f559b6a73c075d757f0b61159e895e0f4e9b89ebf1b4d1fa4356fb,
             65537,
             0x0a265f5c7ff144070714b320b2ab2b984ec1f10136008f5738764ff9a1b6f5851f5e5c6214b226016829ee5f3ae2533efb7788032a53ddcabc0124e6ad13925d34e0acba861cf34553087f224d01622c3f62c13c79178ddc32d53edeb62ce86f23d476f0e8d767006a914bc2d78dad794dd77ae0a47694ac172473c2ad6ed982ee2dd700cbaa2f6a814b4fb0e53f624012cb7e2d38820f048cf725f019ac8351671ab1bfee49c8dcf6cb8a2aee6eaf985c3cb0cd6584ba079fff0842c5713c9db24f5795953ef3b9056673790e4a470d08cf8c410baa7d9f5416f4a1b7f0b19280e393d28b227080ea45e36160fc2485f76d20988f2dd6b1ebde6237454000e1)



def SignCert(tbs_file,
             privkey):

  tbsCert = process_data_with_command(['ascii2der'], read_file_to_string(tbs_file))

  return asn1.ToDER(asn1.SEQUENCE([
    asn1.Raw(tbsCert),
    asn1.SEQUENCE([
      SHA256_WITH_RSA_ENCRYPTION,
      None,
    ]),
    asn1.BitString(privkey.Sign(tbsCert)),
  ]))


def write_file(path, s):
  with open(path, 'w') as f:
    f.write(s)

def main():
  ee_cert = SignCert('ee.tbs', int_key)
  int_cert = SignCert('int.tbs', root_key)
  root_cert = SignCert('root.tbs', root_key)
  pretty_printers = print_certificates.parse_outputs('header,der2ascii,openssl_text,pem')

  write_file('full-chain.pem', print_certificates.pretty_print_certificates([ee_cert, int_cert, root_cert], pretty_printers))
  write_file('chain.pem', print_certificates.pretty_print_certificates([ee_cert, int_cert], pretty_printers))
  write_file('root.pem', print_certificates.pretty_print_certificates([root_cert], pretty_printers))


if __name__ == "__main__":
  main()
