#!/bin/sh
# init - version 140523.1
echo ""
echo " --={ Load Enigma2 from HDD or USB }=--"
echo ""

# Mount things needed by this script
mount -t proc proc /proc
mount -t sysfs sysfs /sys

# Install busybox slinks
/bin/busybox --install -s

# Create device nodes
mknod /dev/tty c 5 0

sd="none"
echo "[init] Check if /dev/sdb exists"
for i in 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 X
do
  count=`cat /proc/partitions | grep -c sdb`
  if [ $count -eq 0 ]; then
    if [ ! "$i" == "X" ]; then
      echo -ne "       Waiting... [$i] \r"
      sleep 1
    fi
  else
    break
  fi
done

if [ "$i" == "X" ]; then
  echo "[init] /dev/sdb not found..."
else
  sd="sdb1"
fi

if [ ! "$sd"=="sdb1" ]; then
  echo "[init] Check if /dev/sda exists"
  for i in 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 X
  do
    count=`cat /proc/partitions | grep -c sda`
    if [ $count -eq 0 ]; then
      if [ ! "$i" == "X" ]; then
        echo -ne "       Waiting... [$i] \r"
        sleep 1
      fi
    else
      break
    fi
  done
fi

if [ "$i" == "%" ]; then
  echo "[init] Time out on wait for /dev/sda, exiting..."
  exec sh
else
  if [ ! "$sd"=="sdb1" ]; then
    sd="sda1"
  fi
fi

echo "[init] ...found, mounting /dev/$sd"
mount /dev/$sd /mnt -t vfat
if [ $? -ne 0 ]; then
  echo "[init] Mounting /dev/$sd failed, exiting..."
  exec sh
fi

echo "[init] Check root.img"
/sbin/fsck.ext3 -p /mnt/root.img
if [ $? -ne 0 ]; then
  echo "[init] Filesystem in root.img is corrupt, exiting..."
  exec sh
fi

echo "[init] Mount root.img"
mount /mnt/root.img /root2 -t ext3 -o loop
#umount /sys /proc

echo "[init] Start Enigma2"
exec switch_root /root2 /bin/devinit
echo "[init] Root file system failed!"
exec sh

