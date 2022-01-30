#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u
echo "start with running manual-linux.sh"

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi
mkdir -p ${OUTDIR}
cd "$OUTDIR"

if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Kernel build starting here"
    echo "Step1"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    echo "Step2"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    echo "Step3"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    
    echo "Step4"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    
    echo "Step5"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    echo "Kernel build ending here"
fi 


echo "Adding the Image in outdir "
echo ${OUTDIR}
echo "Start adding the image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories

ROOTFS=rootfs
mkdir -p "$ROOTFS"
cd "$ROOTFS"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin 
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # TODO:  Configure busybox
    
    echo "Start configuring busybox"
    make distclean
	make defconfig
	echo "End configuring busybox"
    
else
    cd busybox
fi

# TODO: Make and insatll busybox

echo "Start installing busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="${OUTDIR}/${ROOTFS}" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
echo "End installing busybox"

cd "$OUTDIR/$ROOTFS"
echo "Library dependencies"
echo $PWD
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


# TODO: Add library dependencies to rootfs
echo "Start adding library dependencies to rootfs"
SYSROOT_PATH=$(${CROSS_COMPILE}gcc -print-sysroot)
cp $SYSROOT_PATH/lib/ld-linux-aarch64.so.1 lib
cp $SYSROOT_PATH/lib64/libm.so.6 lib64
cp $SYSROOT_PATH/lib64/libresolv.so.2 lib64
cp $SYSROOT_PATH/lib64/libc.so.6 lib64
echo "End adding library dependencies to rootfs"

# TODO: Make device nodes

echo "Start Making device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
echo "End making device nodes"

# TODO: Clean and build the writer utility

echo "Start clean and build the writer utility"
cd "$FINDER_APP_DIR"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
echo "End clean and build the writer utility"

# TODO: Copy the finder related scripts and  exceutables to the /home directory
# on the target rootfs

echo "Start Copy the scripts and executables to /home directory"
cp {writer,finder,finder-test}.sh "$OUTDIR/$ROOTFS/home"
cp writer "$OUTDIR/$ROOTFS/home"
echo $PWD
cp writer.c "$OUTDIR/$ROOTFS/home"
cp -r conf/ "$OUTDIR/$ROOTFS/home"
cp autorun-qemu.sh "$OUTDIR/$ROOTFS/home"
echo "End Copy the scripts and executables to /home directory"

# TODO: Chown the root directory
echo "Start chown the root directory"
cd "$OUTDIR/$ROOTFS"
sudo chown -R root:root * 
echo "End chown the root directory"

# TODO: Create initramfs.cpio.gz
echo "Start creating initramfs.cpio.gz"
cd "$OUTDIR/$ROOTFS"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio && cd ..
gzip -f initramfs.cpio
echo "End creating initramfs.cpio.gz"
