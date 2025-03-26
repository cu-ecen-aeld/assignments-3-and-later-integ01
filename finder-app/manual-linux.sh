#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
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
    OLD_PWD=${OLDPWD}    
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make -j10 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    #make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
else
    OLD_PWD=${OLDPWD}    
fi
#echo ">>>>>>>> ${OLD_PWD} <<<<<<<<"
echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var 
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
#git clone git://busybox.net/busybox.git
git clone https://github.com/mirror/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} 
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
GCC_PATH="${OLD_PWD}/gcc_resource"
#unzip "${GCC_PATH}.zip" -d ${OLD_PWD}
cp ${GCC_PATH}/libm.so.6 lib64/
cp ${GCC_PATH}/libresolv.so.2 lib64/
cp ${GCC_PATH}/libc.so.6 lib64/
cp ${GCC_PATH}/ld-linux-aarch64.so.1 lib/


#cp ${GCC_PATH}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 lib64/
#cp ${GCC_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 lib64/
#cp ${GCC_PATH}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 lib64/
#cp ${GCC_PATH}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 lib/

# TODO: Make device nodes
sudo mknod -m 666 dev/null  c 1 3
sudo mknod -m 666 dev/console  c 5 1
sudo mknod -m 666 dev/tty  c 5 0
#nod /dev/console 0600 0 0 c 5 1

# TODO: Clean and build the writer utility
echo "Changing to old dir: ${OLD_PWD}"
cd ${OLD_PWD}
make clean
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp writer ${OUTDIR}/rootfs/home/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -r ../conf ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
#sudo chown root:root ../Image


# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
#sudo chown root:root initramfs.cpio.gz

#mv initramfs.cpio.gz ${OUTDIR}


