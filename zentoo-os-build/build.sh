#!/bin/bash

PROCESSORS=`cat /proc/cpuinfo | grep -c processor`

echo Using $PROCESSORS cpus

if [ -e ./prebuild/zensysgen-glibc.tgz ]; then 
  echo Using pre-built OS
  for i in `find ./prebuild -name "*.tgz"`
  do
    echo Extracting $i
    tar xpzf $i -C /
  done

  exit 0
fi

# kernel link files
mkdir -p /zensysgen-glibc/usr/src
ln -sf /usr/src/linux /zensysgen-glibc/usr/src/linux

echo Building Base:
ROOT="/zensysgen-glibc" \
CFLAGS="-Os -march=pentium3 -mtune=pentium3 -pipe -fomit-frame-pointer -fexpensive-optimizations -ffast-math" \
CXXFLAGS="${CFLAGS}" \
USE="minimal make-symlinks glibc-omitfp -cracklib -python -berkdb -db -openmp -gpm -ipv6 -pam -X -perl -webdav -git iconv mmx sse" \
MAKEOPTS="-j${PROCESSORS}" \
INSTALL_MASK="*.h *.a *.o /usr/include /usr/man /usr/share/man /usr/share/doc /usr/share/info /usr/share/locale /etc/conf.d /etc/init.d" \
emerge glibc busybox libcap dhcpcd bash lilo bzip2 pciutils openssh e2fsprogs mount-cifs nfs-utils -v

if [ ! X$? == X0 ]; then
  echo "Error ?"
  read 
fi

echo Building media support:
ROOT="/zensysgen-glibc" \
CFLAGS="-O3 -march=pentium3 -mtune=pentium3 -pipe -fomit-frame-pointer -fexpensive-optimizations" \
CXXFLAGS="${CFLAGS}" \
USE="-python -berkdb -db -openmp -gpm -ipv6 -pam -X -perl -webdav -git -xv -xvmc -v4l directfb fb fbcon alsa oss mad mp3 aac iconv -kde -gnome -cups -opengl -mesa mmx sse" \
MAKEOPTS="-j${PROCESSORS}" \
INSTALL_MASK="*.h *.a *.o /usr/include /usr/man /usr/share/man /usr/share/doc /usr/share/info /usr/share/locale /etc/conf.d /etc/init.d" \
VIDEO_CARDS="intel fbdev fb" \
LIRC_DEVICES="" \
KERNEL_DIR="/usr/src/linux" \
emerge jpeg DirectFB fontconfig freetype giflib libpng xine-lib libid3tag mp3info libsndfile -v

if [ ! X$? == X0 ]; then
  echo "Error ?"
  read
fi

echo "Installing Font(s):"
ROOT="/zensysgen-glibc" \
USE="" \
MAKEOPTS="-j${PROCESSORS}" \
INSTALL_MASK="*.h *.a *.o /usr/include /usr/man /usr/share/man /usr/share/doc /usr/share/info /usr/share/locale /etc/conf.d /etc/init.d" \
LIRC_DEVICES="serial" \
KERNEL_DIR="/usr/src/linux" \
emerge ttf-bitstream-vera vdrsymbols-ttf -v

if [ ! X$? == X0 ]; then
  echo "Error ?"  read
fi

# ----------------------------------------------------------------------------
#
#    TEMP-builds (packages where we only need parts of)
#

PACKAGE="alsa-utils"
echo "Building temp package:"${PACKAGE}
[ ! -e /zensysgen-glibc-temp-${PACKAGE} ] && mkdir /zensysgen-glibc-temp-${PACKAGE}
rm -rf /zensysgen-glibc-temp-${PACKAGE}/var
mkdir  /zensysgen-glibc-temp-${PACKAGE}/var
cp -rp /zensysgen-glibc/var/db /zensysgen-glibc-temp-${PACKAGE}/var
mkdir  /zensysgen-glibc-temp-${PACKAGE}/var/cache
cp -rp /zensysgen-glibc/var/cache/edb /zensysgen-glibc-temp-${PACKAGE}/var/cache

ROOT="/zensysgen-glibc-temp-"${PACKAGE} \
USE="minimal make-symlinks -python -berkdb -db -openmp -gpm -ipv6 -pam -X iconv mmx sse" \
MAKEOPTS="-j${PROCESSORS}" \
INSTALL_MASK="*.h *.a *.o /usr/include /usr/man /usr/share/man /usr/share/doc /usr/share/info /usr/share/locale /etc/conf.d /etc/init.d" \
emerge ${PACKAGE} -v

cp -a /zensysgen-glibc-temp-alsa-utils/usr/sbin/alsactl     /zensysgen-glibc/usr/sbin

PACKAGE="lirc"
echo "Building temp package:"${PACKAGE}
[ ! -e /zensysgen-glibc-temp-${PACKAGE} ] && mkdir /zensysgen-glibc-temp-${PACKAGE}
rm -rf /zensysgen-glibc-temp-${PACKAGE}/var
mkdir  /zensysgen-glibc-temp-${PACKAGE}/var
cp -rp /zensysgen-glibc/var/db /zensysgen-glibc-temp-${PACKAGE}/var
mkdir  /zensysgen-glibc-temp-${PACKAGE}/var/cache
cp -rp /zensysgen-glibc/var/cache/edb /zensysgen-glibc-temp-${PACKAGE}/var/cache

# kernel link files
mkdir -p /zensysgen-glibc-temp-${PACKAGE}/usr/src
ln -sf /usr/src/linux /zensysgen-glibc-temp-${PACKAGE}/usr/src/linux

ROOT="/zensysgen-glibc-temp-"${PACKAGE} \
USE="minimal make-symlinks -python -berkdb -db -openmp -gpm -ipv6 -pam -X iconv mmx sse" \
MAKEOPTS="-j${PROCESSORS}" \
INSTALL_MASK="*.h *.a *.o /usr/include /usr/man /usr/share/man /usr/share/doc /usr/share/info /usr/share/locale /etc/conf.d /etc/init.d" \
emerge ${PACKAGE} -v

cp -a /zensysgen-glibc-temp-lirc/usr/lib/*.so*  /zensysgen-glibc/usr/lib/
cp -a /zensysgen-glibc-temp-lirc/usr/bin/irw    /zensysgen-glibc/usr/bin/
cp -a /zensysgen-glibc-temp-lirc/usr/bin/irexec /zensysgen-glibc/usr/bin/

# ----------------------------------------------------------------------------
#
#    Features !!!!!!
#

echo "Building addon components"

for i in ./features/*.build; do
  [ -e $i ] || continue
  . $i
done

# Copy libstdc++ from the host compiler as well as libgcc_c so files

mkdir -p /zensysgen-glibc/usr/lib/gcc/i686-pc-linux-gnu/4.4.4/
cp -a /usr/lib/gcc/i686-pc-linux-gnu/4.4.4/libstdc++.so* /zensysgen-glibc/usr/lib/gcc/i686-pc-linux-gnu/4.4.4/
cp -a /lib/libgcc_s.so.1 /zensysgen-glibc/lib

echo "Making extra directories"
mkdir /zensysgen-glibc/proc
mkdir /zensysgen-glibc/sys
mkdir /zensysgen-glibc/mnt
mkdir /zensysgen-glibc/root
mkdir /zensysgen-glibc/var/run

echo "Creating irexec lircd link"
mkdir /zensysgen-glibc/var/run/lirc
ln -sf /dev/lircd /zensysgen-glibc/var/run/lirc/lircd

echo "Making devices."
mkdir x
mount -o bind / x
cd x
tar cf - dev/tty* dev/hda* dev/hdb* dev/console dev/sda* dev/null dev/zero dev/mem | tar xvpf - -C /zensysgen-glibc
cd ..
umount x
rm -rf x
# Make loop devices
mkdir /zensysgen-glibc/dev/loop
mknod /zensysgen-glibc/dev/loop/0 b 7 0 -m 660
chown root:disk /zensysgen-glibc/dev/loop/0 
mknod /zensysgen-glibc/dev/loop/1 b 7 1 -m 660
chown root:disk /zensysgen-glibc/dev/loop/1 
mknod /zensysgen-glibc/dev/loop/2 b 7 2 -m 660
chown root:disk /zensysgen-glibc/dev/loop/2 
mknod /zensysgen-glibc/dev/loop/3 b 7 3 -m 660
chown root:disk /zensysgen-glibc/dev/loop/3 
mknod /zensysgen-glibc/dev/loop/4 b 7 4 -m 660
chown root:disk /zensysgen-glibc/dev/loop/4 
mknod /zensysgen-glibc/dev/loop/5 b 7 5 -m 660
chown root:disk /zensysgen-glibc/dev/loop/5 
mknod /zensysgen-glibc/dev/loop/6 b 7 6 -m 660
chown root:disk /zensysgen-glibc/dev/loop/6 
mknod /zensysgen-glibc/dev/loop/7 b 7 7 -m 660
chown root:disk /zensysgen-glibc/dev/loop/7 

echo "Making /storage"
mkdir /zensysgen-glibc/storage

echo "Preparing modules"
#ln -sf /tmp/config/modules/lib/modules /zensysgen-glibc/lib/modules
mkdir -p /zensysgen-glibc/lib/modules
#tar cf - /lib/modules/2.6.36 | tar xvpf - -C /zensysgen-glibc/modules

echo "Setting timezone."
ln -sf /usr/share/zoneinfo/Europe/Berlin /zensysgen-glibc/etc/localtime

echo "Copying locale.alias"
mkdir /zensysgen-glibc/usr/share/locale
cp  /usr/share/locale/locale.alias /zensysgen-glibc/usr/share/locale/

echo "Updating ld.so.cache"
chroot /zensysgen-glibc /sbin/ldconfig

echo "Saving prebuilds"
for i in /zensysgen-glibc* ; do
  echo $i
  tar czf ./prebuild/$i.tgz $i
done
