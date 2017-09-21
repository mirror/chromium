#!/usr/bin/python
# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from pyasn1.codec.der import decoder, encoder
from pyasn1_modules import rfc2560, rfc2459, pem
from pyasn1.type import univ, useful, tag
import sys, hashlib, time, datetime

from OpenSSL import crypto, SSL
from socket import gethostname
from pprint import pprint
from time import gmtime, mktime
from os.path import exists, join

import base64

NEXT_SERIAL = 0

sha1oid = univ.ObjectIdentifier('1.3.14.3.2.26')
sha1rsaoid = univ.ObjectIdentifier('1.2.840.113549.1.1.5')
sha256oid = univ.ObjectIdentifier('2.16.840.1.101.3.4.2.1')
sha256rsaoid = univ.ObjectIdentifier('1.2.840.113549.1.1.11')

def SigAlgOID(sigAlg):
  if sigAlg == 'sha1':
    return sha1rsaoid
  return sha256rsaoid

def create_cert(name, signer=None, ocsp=False):
  global NEXT_SERIAL
  pkey = crypto.PKey()
  pkey.generate_key(crypto.TYPE_RSA, 1024)
  cert = crypto.X509()
  cert.set_version(2)
  cert.get_subject().CN = name
  cert.set_pubkey(pkey)
  cert.set_serial_number(NEXT_SERIAL)
  NEXT_SERIAL += 1
  cert.gmtime_adj_notBefore(0)
  cert.gmtime_adj_notAfter(315360000)
  if ocsp:
    cert.add_extensions([crypto.X509Extension("extendedKeyUsage", False, "OCSPSigning")])
  if signer:
    cert.set_issuer(signer[1].get_subject())
    cert.sign(signer[2], 'sha1')
  else:
    cert.set_issuer(cert.get_subject())
    cert.sign(pkey, 'sha1')
  asn1cert = decoder.decode(crypto.dump_certificate(crypto.FILETYPE_ASN1, cert),
                            asn1Spec=rfc2459.Certificate())[0]
  if not signer:
    signer = [asn1cert]
  return (asn1cert, cert, pkey, signer[0])

def create_extension():
  ext = rfc2459.Extension()
  ext.setComponentByName('extnID', univ.ObjectIdentifier('1.2.3.4'))
  ext.setComponentByName('extnValue', 'DEADBEEF')

  return ext

ca = create_cert('Test CA', None)
ca_link = create_cert('Test OCSP Signer', ca, True)
ca_badlink = create_cert('Test False OCSP Signer', ca, False)
CERT = create_cert('Test Cert', ca)
JUNK_CERT = create_cert('Random Cert', None)
EXTENSION = create_extension()

def get_name(c):
  rid = rfc2560.ResponderID()
  subject = c[0].getComponentByName('tbsCertificate').getComponentByName('subject')
  rn = rid.componentType.getTypeByPosition(0).clone()
  for i in range(len(subject)):
    rn.setComponentByPosition(i, subject.getComponentByPosition(i))
  rid.setComponentByName('byName', rn)
  return rid

def get_key_hash(c):
  rid = rfc2560.ResponderID()
  spk = c[0].getComponentByName('tbsCertificate').getComponentByName('subjectPublicKeyInfo').getComponentByName('subjectPublicKey')
  keyHash = hashlib.sha1(encoder.encode(spk)[4:]).digest()
  rid.setComponentByName('byKey', keyHash)
  return rid

class RevokeTime(useful.GeneralizedTime):
  tagSet = useful.GeneralizedTime.tagSet.tagExplicitly(tag.Tag(
      tag.tagClassContext, tag.tagFormatSimple, 0))

class CertRevokeInfo(rfc2560.RevokedInfo):
  tagSet = rfc2560.RevokedInfo.tagSet.tagImplicitly(tag.Tag(
      tag.tagClassContext, tag.tagFormatSimple, 1))

def create_single_response(cert=CERT, status=0, next=None, revoke_time=None, reason=None, extensions=[]):
  sr = rfc2560.SingleResponse()
  cid = sr.setComponentByName('certID').getComponentByName('certID')

  issuer_tbs = cert[3].getComponentByName('tbsCertificate')
  tbs = cert[0].getComponentByName('tbsCertificate')
  nameHash = hashlib.sha1(encoder.encode(issuer_tbs.getComponentByName('subject'))).digest()
  keyHash = hashlib.sha1(encoder.encode(issuer_tbs.getComponentByName('subjectPublicKeyInfo').getComponentByName('subjectPublicKey'))[4:]).digest()
  sn = tbs.getComponentByName('serialNumber')

  ha = cid.setComponentByName('hashAlgorithm').getComponentByName('hashAlgorithm')
  ha.setComponentByName('algorithm', sha1oid)
  cid.setComponentByName('issuerNameHash', nameHash)
  cid.setComponentByName('issuerKeyHash', keyHash)
  cid.setComponentByName('serialNumber', sn)

  cs = rfc2560.CertStatus()
  if status == 0:
    cs.setComponentByName('good')
  elif status == 1:
    ri = CertRevokeInfo()
    if revoke_time == None:
      revoke_time = datetime.datetime.utcnow()
    ri.setComponentByName('revocationTime', useful.GeneralizedTime(revoke_time.strftime('%Y%m%d%H%M%SZ')))
    if reason:
      ri.setComponentByName('revocationReason', reason)
    cs.setComponentByName('revoked', ri)
  else:
    ui = rfc2560.UnknownInfo().subtype(implicitTag=tag.Tag(tag.tagClassContext, tag.tagFormatSimple, 2))
    cs.setComponentByName('unknown', ui)

  sr.setComponentByName('certStatus', cs)

  sr.setComponentByName('thisUpdate', useful.GeneralizedTime(datetime.datetime.utcnow().strftime('%Y%m%d%H%M%SZ')))
  if next:
    sr.setComponentByName('nextUpdate', RevokeTime(next.strftime('%Y%m%d%H%M%SZ')))
  if extensions:
    elist = sr.setComponentByName('singleExtensions').getComponentByName('singleExtensions')
    for i in range(len(extensions)):
      elist.setComponentByPosition(i, extensions[i])
  return sr

class OCSPResponseBytes(rfc2560.ResponseBytes):
  tagSet = rfc2560.ResponseBytes.tagSet.tagExplicitly(tag.Tag(
      tag.tagClassContext, tag.tagFormatSimple, 0))

def create(signer=None, response_status=0, response_type='1.3.6.1.5.5.7.48.1.1', signature=None, version=1, responder=None, responses=None, extensions=None, certs=None, sigAlg='sha1'):
  ocsp = rfc2560.OCSPResponse()
  ocsp.setComponentByName('responseStatus', response_status)

  if response_status != 0:
    return ocsp

  tbs = rfc2560.ResponseData()
  if version != 1:
    tbs.setComponentByName('version', version)

  if not signer:
    signer = ca
  if not responder:
    responder = get_name(signer)
  tbs.setComponentByName('responderID', responder)
  tbs.setComponentByName('producedAt', useful.GeneralizedTime(datetime.datetime.utcnow().strftime('%Y%m%d%H%M%SZ')))
  rlist = tbs.setComponentByName('responses').getComponentByName('responses')
  if responses == None:
    responses = [create_single_response(CERT, 0)]
  if responses:
    for i in range(len(responses)):
      rlist.setComponentByPosition(i, responses[i])

  if extensions:
    elist = tbs.setComponentByName('responseExtensions').getComponentByName('responseExtensions')
    for i in range(len(extensions)):
      elist.setComponentByPosition(i, extensions[i])

  sa = rfc2459.AlgorithmIdentifier()
  sa.setComponentByName('algorithm', SigAlgOID(sigAlg))
  sa.setComponentByName('parameters', univ.Null())

  basic = rfc2560.BasicOCSPResponse()
  basic.setComponentByName('tbsResponseData', tbs)
  basic.setComponentByName('signatureAlgorithm', sa)
  if not signature:
    signature = crypto.sign(signer[2], encoder.encode(tbs), sigAlg)
  basic.setComponentByName('signature', univ.BitString("'%s'H" % (signature.encode('hex'))))
  if certs:
    cs = basic.setComponentByName('certs').getComponentByName('certs')
    for i in range(len(certs)):
      cs.setComponentByPosition(i, certs[i][0])

  rbytes = OCSPResponseBytes()
  rbytes.setComponentByName('responseType', univ.ObjectIdentifier(response_type))
  rbytes.setComponentByName('response', encoder.encode(basic))

  ocsp.setComponentByName('responseBytes', rbytes)
  return ocsp

def wrap(s):
  return '\n'.join(s[pos:pos+64] for pos in xrange(0, len(s), 64))

def store(fname, ca, data):
  ca64 = crypto.dump_certificate(crypto.FILETYPE_PEM, ca[1]).replace('CERTIFICATE', 'CA CERTIFICATE')
  c64 = crypto.dump_certificate(crypto.FILETYPE_PEM, CERT[1])
  d64 = base64.b64encode(encoder.encode(data))
  out = '-----BEGIN OCSP RESPONSE-----\n%s\n-----END OCSP RESPONSE-----\n\n%s\n\n%s' % (wrap(d64), ca64, c64)
  open('%s.pem' % fname, 'w').write(out)

store('no_response', ca,                create(responses=[]))

store('bad_status', ca,                 create(response_status=17))
store('bad_ocsp_type', ca,              create(response_type='1.3.6.1.5.5.7.48.1.2'))
store('bad_signature', ca,              create(signature='\xde\xad\xbe\xef'))
store('ocsp_sign_direct', ca,           create(signer=ca, certs=[]))
store('ocsp_sign_indirect', ca,         create(signer=ca_link, certs=[ca_link]))
store('ocsp_sign_indirect_missing', ca, create(signer=ca_link, certs=[]))
store('ocsp_sign_bad_indirect', ca,     create(signer=ca_badlink, certs=[ca_badlink]))
store('ocsp_extra_certs', ca,           create(signer=ca, certs=[ca, ca_link]))
store('has_version', ca,                create(version=1))
store('responder_name', ca,             create(responder=get_name(ca)))
store('responder_id', ca,               create(responder=get_key_hash(ca)))
store('has_extension', ca,              create(extensions=[EXTENSION]))

store('good_response', ca,       create(responses=[create_single_response(CERT, 0)]))
store('good_response_sha256', ca,       create(responses=[create_single_response(CERT, 0)], sigAlg='sha256'))
store('good_response_next_update', ca,  create(responses=[create_single_response(CERT, 0, next=datetime.datetime.utcnow())]))
store('revoke_response', ca,            create(responses=[create_single_response(CERT, 1)]))
store('revoke_response_reason', ca,     create(responses=[create_single_response(CERT, 1, revoke_time=datetime.datetime.utcnow(), reason=1)]))
store('unknown_response', ca,           create(responses=[create_single_response(CERT, 2)]))
store('multiple_response', ca,          create(responses=[create_single_response(CERT, 0), create_single_response(CERT, 2)]))
store('other_response', ca,             create(responses=[create_single_response(JUNK_CERT, 0), create_single_response(JUNK_CERT, 1)]))
store('has_single_extension', ca,       create(responses=[create_single_response(CERT, 0, extensions=[create_extension()])]))

store('missing_response', ca,           create(response_status=0, responses=[]))
