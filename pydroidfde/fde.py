#!/usr/bin/env python
#
# Android FDE Decryption
#
# Authors:  Thomas Cannon <tcannon@viaforensics.com>
#           Andrey Belenko <abelenko@viaforensics.com>
#           Cedric Halbronn <cedric.halbronn@sogeti.com>
# Requires: Python, M2Crypto (sudo apt-get install python-m2crypto)
#
# --
# Revision 0.1
# ------------
# Code merged from bruteforce_stdcrypto.py and decrypt.py
#

# Imports
from os import path
from M2Crypto import EVP
import hashlib
import struct, sys, itertools, time
#from pbkdf2 import pbkdf2_bin
from hexdump import hexdump
import argparse

# Constants
HEADER_FORMAT = "=LHHLLLLLLL64s"  # Taken from cryptfs.h in crespo source.
HASH_COUNT = 2000
IV_LEN_BYTES = 16
SECTOR_SIZE = 512
BLOCK_SIZE = 16
ENCRYPT = 1
DECRYPT = 0
header = ''

def parse_header(header_file):
  global header
  # Check header file is bigger than 0x100
  fileSize = path.getsize(header_file)
  assert(fileSize >= 0x100)

  # Read header file
  header = open(header_file, 'rb').read()

  # Unpack header
  ftrMagic, \
  majorVersion, \
  minorVersion, \
  ftrSize, \
  flags, \
  keySize, \
  spare1, \
  fsSize1, \
  fsSize2, \
  failedDecrypt, \
  cryptoType = \
  struct.unpack(HEADER_FORMAT, header[0:100])

  if minorVersion != 0: # TODO: This is a dirty fix for 1.2 header. Need to do something more generic
    ftrSize = 0x68

  encrypted_key = header[ftrSize:ftrSize + keySize]
  salt = header[ftrSize + keySize + 32:ftrSize + keySize + 32 + 16]

  # Display parsed header
  print 'Magic          :', "0x%0.8X" % ftrMagic
  print 'Major Version  :', majorVersion
  print 'Minor Version  :', minorVersion
  print 'Footer Size    :', ftrSize, "bytes"
  print 'Flags          :', "0x%0.8X" % flags
  print 'Key Size       :', keySize * 8, "bits"
  print 'Failed Decrypts:', failedDecrypt
  print 'Crypto Type    :', cryptoType.rstrip("\0")
  print 'Encrypted Key  :', "0x" + encrypted_key.encode("hex").upper()
  print 'Salt           :', "0x" + salt.encode("hex").upper()
  print '----------------'

  return encrypted_key, salt


# This function assume the PIN/password is correct since we do not check this here
def get_decrypted_key(encrypted_key, salt, password, debug=True):

  keySize = len(encrypted_key)
  assert(keySize == 16 or keySize == 32) # Other cases should be double-checked
  if keySize == 16:
    algorithm='aes_128_cbc'
  elif keySize == 32:
    algorithm='aes_256_cbc'
  else:
    print 'Error: unsupported keySize'
    return

  # Calculate the key decryption key and IV from the password
  # We encountered problems with EVP.pbkdf2 on some Windows platforms with M2Crypto-0.21.1
  # In such case, use pbkdf2.py from https://github.com/mitsuhiko/python-pbkdf2
  # For scrypt case, use py-script from https://bitbucket.org/mhallin/py-scrypt/src
  deriv_result = None
  dkLen = keySize+IV_LEN_BYTES
  if header[0xbc] == '\x02': # if the header specify a dkType == 2, the partition uses scrypt
    import scrypt
    print '[+] This partition uses scrypt'
    factors = (ord(fact) for fact in header[0xbd:0xc0])
    N = factors.next()
    r = factors.next()
    p = factors.next()
    print "[+] scrypt parameters are: N=%s, r=%s, p=%s" % (hex(N), hex(r), hex(p))
    deriv_result = scrypt.hash(password, salt, 1 << N, 1 << r, 1 << p)[:dkLen]
  else:
    print '[+] This partition uses pbkdf2'
    deriv_result = EVP.pbkdf2(password, salt, iter=HASH_COUNT, keylen=dkLen)
  key = deriv_result[:keySize]
  iv = deriv_result[keySize:]

  # Decrypt the encryption key
  cipher = EVP.Cipher(alg=algorithm, key=key, iv=iv, padding=0, op=DECRYPT)
  decrypted_key = cipher.update(encrypted_key)
  decrypted_key = decrypted_key + cipher.final()
  
  # Display the decrypted key
  if debug:
    print 'Password       :', password
    print 'Derived Key    :', "0x" + key.encode("hex").upper()
    print 'Derived IV     :', "0x" + iv.encode("hex").upper()
    print 'Decrypted Key  :', "0x" + decrypted_key.encode("hex").upper()
    print '----------------'

  return decrypted_key

def decrypt_data(decrypted_key, essiv, data):

  keySize = len(decrypted_key)
  assert(keySize == 16 or keySize == 32) # Other cases should be double checked
  if keySize == 16:
    algorithm='aes_128_cbc'
  elif keySize == 32:
    algorithm='aes_256_cbc'
  else:
    print 'Error: unsupported keySize'
    return

  # try to decrypt the actual data 
  cipher = EVP.Cipher(alg=algorithm, key=decrypted_key, iv=essiv, op=0) # 0 is DEC
  cipher.set_padding(padding=0)
  decData = cipher.update(data)
  decData = decData + cipher.final()
  return decData
