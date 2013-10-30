#!/bin/bash

#Script that delete partition on disk and create a calaos system

ROOT_UID=0     # Only users with $UID 0 have root privileges.
MBR="/usr/lib/syslinux/mbr.bin"
tmpdir="/tmp/create_cf_tmp"

function clean_temp()
{
    set +e
    #clean tempdir if needed
    mkdir -p $tmpdir/mountfs
    umount $tmpdir/mountfs > /dev/null 2>&1
    set -e
}

#Usage: ./create_cf.sh <disk> <firmware> <local_config.xml>
set -e #stop if any error occurs
set -o nounset #stop if variable are uninitialised

# Run as root, of course.
if [ "$UID" -ne "$ROOT_UID" ]
then
    echo "Must be root to run this script."
    exit 1
fi

clean_temp

if [ $# -ne 3 ]
then
    echo "Usage: `basename $0` </dev/disk> <firmware.img> <local_config.xml>"
    exit 1
fi

DISK=$1
FIRMWARE=$2
LCONFIG=$3

#some checks
test -b $DISK || { echo "$DISK is not a block device"; exit 1; }
test -f $FIRMWARE || { echo "$FIRMWARE does not exist"; exit 1; }
test -f $LCONFIG || { echo "$LCONFIG does not exist"; exit 1; }
test -f $MBR || { echo "$MBR file is not found. Please install syslinux."; exit 1; }
hash syslinux 2>&- || { echo "syslinux needed. Please install syslinux."; exit 1; }
hash parted 2>&- || { echo "parted needed. Please install parted."; exit 1; }
hash mkdosfs 2>&- || { echo "mkdosfs needed. Please install mkdosfs."; exit 1; }
hash mkfs.ext3 2>&- || { echo "mkfs.ext3 needed. Please install mkfs.ext3."; exit 1; }

#for progression
echo 5

#get all partition number for that device
part_list=`parted -s $DISK print | awk '/^ / {print $1}'`

#delete all existing partitions from disk
set +e
for p in $part_list
do
    umount ${DISK}${p} > /dev/null 2>&1
    parted -s $DISK rm $p > /dev/null
done
set -e

echo 15

#create the new VFAT and ext3 partitions for system/config
parted -s $DISK unit "%" mkpart primary fat32 0 50 > /dev/null
parted -s $DISK unit "%" mkpart primary ext3 50 100 > /dev/null

echo 20

#format partitions
mkdosfs -F32 -v -n "System" ${DISK}1 > /dev/null
echo 35
mkfs.ext3 -L "Config" ${DISK}2 > /dev/null 2>&1
echo 45

#make it boot
parted -s $DISK set 1 boot on > /dev/null
echo 50

cat $MBR > $DISK
sync
syslinux ${DISK}1 > /dev/null
echo 55

#mount system partition and extract firmware
mount ${DISK}1 $tmpdir/mountfs
set +e
tar -C $tmpdir/mountfs -xjf $FIRMWARE 2> /dev/null
set -e
sync
umount $tmpdir/mountfs
echo 80

#mount config partition and create config dirs/files
mount ${DISK}2 $tmpdir/mountfs
mkdir -p $tmpdir/mountfs/apache
mkdir -p $tmpdir/mountfs/calaos
echo 85

#generate https certificate
cat > $tmpdir/cert.cnf << "EOF"
RANDFILE = /dev/urandom
[ req ]
default_bits = 1024
encrypt_key = yes 
distinguished_name = req_dn
x509_extensions = cert_type
prompt = no

[ req_dn ]
C=FR
ST=FRANCE
L=Hesingue
O=Calaos
OU=Calaos
CN=calaos.fr
emailAddress=contact@calaos.fr

[ cert_type ]
basicConstraints                = critical,CA:FALSE
nsCertType                      = server
nsComment                       = "Calaos SSL Certificate"
subjectKeyIdentifier            = hash
authorityKeyIdentifier          = keyid,issuer:always
subjectAltName                  = DNS:home.calaos.fr,DNS:mobile.calaos.fr,DNS:webmail.calaos.fr,DNS:www.calaos.fr,DNS:dev.calaos.fr,DNS:update.calaos.fr,DNS:support.calaos.fr,DNS:imap.calaos.fr,DNS:blog.calaos.fr,DNS:priv.calaos.fr,DNS:internal.calaos.fr,DNS:intern.calaos.fr,DNS:compta.calaos.fr
issuerAltName                   = issuer:copy
keyUsage                        = keyEncipherment, digitalSignature
extendedKeyUsage                = serverAuth
EOF

openssl req -new -outform PEM -config $tmpdir/cert.cnf -out $tmpdir/mountfs/apache/server.pem -newkey rsa:2048 -nodes -keyout $tmpdir/mountfs/apache/server.key -keyform PEM -days 9999 -x509 > /dev/null 2>&1

echo 90
cp $LCONFIG $tmpdir/mountfs/calaos/local_config.xml

echo 95

sync
umount $tmpdir/mountfs

rm -fr $tmpdir >/dev/null 2>&1

echo 100
exit 0

