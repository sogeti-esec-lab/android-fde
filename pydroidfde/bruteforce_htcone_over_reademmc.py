#!/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, Decryption, HTC One, Bruteforce
#
# Uses the read_emmc command available on the HTC One to dump the "extra" partition and the first sector of "data" partition
# Uses the bruteforce.py to bruteforce the PIN/password

from dump import read_emmc
from bruteforce import do_job
import os, sys

OUTDIR='output'

if os.path.isdir(OUTDIR):
  print 'ERROR: outdir already exists!'
  sys.exit(0)

os.mkdir(OUTDIR)
print 'Output directory: %s' % OUTDIR

DATAFILE=os.path.join(OUTDIR, "data")
EXTRAFILE=os.path.join(OUTDIR, "extra")
KEYFILE=os.path.join(OUTDIR, "keyfile")

OFFSET_DATA=6422528
OFFSET_EXTRA=586799

read_emmc(start=OFFSET_DATA, num=1, outfile=DATAFILE)
read_emmc(start=OFFSET_EXTRA, num=1, outfile=EXTRAFILE)
do_job(EXTRAFILE, DATAFILE, KEYFILE, 4)
