#!/usr/bin/env python
#
# Android FDE Decryption
#
# Authors:  Thomas Cannon <tcannon@viaforensics.com>
#           Andrey Belenko <abelenko@viaforensics.com>
#           Cedric Halbronn <cedric.halbronn@sogeti.com>
# Requires: Python, M2Crypto (sudo apt-get install python-m2crypto)
#
# Parses the header for the encrypted userdata partition
# Decrypts the master key found in the header using a supplied password
# Decrypts the sectors of an encrypted userdata partition using the decrypted key
#
# --
# Revision 0.1 (released by Thomas)
# ------------
# Written for Nexus S (crespo) running Android 4.0.4
# Decrypts a given sector of an encrypted userdata partition using the decrypted key
# Header is located in file userdata_footer on the efs partition
#
# --
# Revision 0.2 (released by Cedric)
# ------------
# Rewritten to loop on all the sectors of the whole userdata partition
# Adapted to support HTC One running Android 4.2.2
# Header is located in "extra" partition located in mmcblk0p27
#

from fde import *

def decrypt(encrypted_partition, sector_start, decrypted_key, outfile, debug=True):

  # Check encrypted partition size is a multiple of sector size and open file
  fileSize = path.getsize(encrypted_partition)
  assert(fileSize % SECTOR_SIZE == 0)
  nb_sectors = fileSize / SECTOR_SIZE
  fd = open(encrypted_partition, 'rb')
  outfd = open(outfile, 'wb')

  keySize = len(decrypted_key)
  assert(keySize == 16 or keySize == 32) # Other cases should be double checked
  if keySize == 16:
    algorithm='aes_128_cbc'
  elif keySize == 32:
    algorithm='aes_256_cbc'
  else:
    print 'Error: unsupported keySize'
    return

  # Decrypt one sector at a time
  for i in range(0, nb_sectors):

    # Read encrypted sector
    sector_offset = sector_start + i
    encrypted_data = fd.read(SECTOR_SIZE)
  
    # Calculate ESSIV
    # ESSIV mode is defined by:
    # SALT=Hash(KEY)
    # IV=E(SALT,sector_number)
    salt = hashlib.sha256(decrypted_key).digest()
    sector_number = struct.pack("<I", sector_offset) + "\x00" * (BLOCK_SIZE - 4)
    
    # Since our ESSIV hash is SHA-256 we should use AES-256
    # We use ECB mode here (instead of CBC with IV of all zeroes) due to crypto lib weirdness
    # EVP engine PKCS7-pads data by default so we explicitly disable that
    cipher = EVP.Cipher(alg='aes_256_ecb', key=salt, iv='', padding=0, op=ENCRYPT)
    essiv = cipher.update(sector_number)
    essiv += cipher.final()
    
    if debug:
      print 'SECTOR NUMBER  :', "0x" + sector_number.encode("hex").upper()
      print 'ESSIV SALT     :', "0x" + salt.encode("hex").upper()
      print 'ESSIV IV       :', "0x" + essiv.encode("hex").upper()
      print '----------------'
    
    # Decrypt sector of userdata image
    decrypted_data = decrypt_data(decrypted_key, essiv, encrypted_data)
    
    # Print the decrypted data
    if debug:
      print 'Decrypted Data :', "0x" + decrypted_data.encode("hex").upper()
    outfd.write(decrypted_data)

  fd.close()
  outfd.close()

def main():

  parser = argparse.ArgumentParser(description='FDE for Android')

  parser.add_argument('encrypted_partition', help='The encrypted /data partition')
  parser.add_argument('header_file', help='The header file containing the encrypted key')
  parser.add_argument('outfile', help='The filename to save the decrypted partition')
  parser.add_argument('-p', "--password", help='Provided password. Default is "0000"', default='0000')
  parser.add_argument('-s', "--sector", help='Sector number for the first bytes in the encrypted_partition file. It allows to decrypt data dumped at a particular offset. Default is 0', default=0, type=int)
  args = parser.parse_args()

  outfile = args.outfile
  header_file = args.header_file
  encrypted_partition = args.encrypted_partition
  assert path.isfile(header_file), "Header file '%s' not found." % header_file
  assert path.isfile(encrypted_partition), "Encrypted partition '%s' not found." % encrypted_partition
  password = args.password
  sector_start = args.sector

  # Parse header
  encrypted_key, salt = parse_header(header_file)

  # Get decrypted key
  decrypted_key = get_decrypted_key(encrypted_key, salt, password)

  # Loop on sectors
  decrypt(encrypted_partition, sector_start, decrypted_key, outfile, debug=False)

  print 'Decrypted partition written to: %s' % outfile
  print 'Done.'

if __name__ == "__main__":

  main()
