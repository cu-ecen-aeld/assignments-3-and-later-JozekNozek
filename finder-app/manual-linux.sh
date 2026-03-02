#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: $SCRIPT_DIR"
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

sudo apt update && sudo apt install -y flex bison libncurses-dev libssl-dev bc

sudo mkdir -p -m 755 ${OUTDIR}
sudo chown jouza:jouza ${OUTDIR}

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

    # deep clean the kernel build tree -> removing the .config file with any existing configurations
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper

    # Use default config files as config files
    # configure for our “virt” arm dev board we will simulate in QEMU
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig

    # build a kernel image for booting with QEMU (-j4 says use 4 processor cores I think)
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all

    # This command builds Linux kernel modules for the ARM64 architecture using a specified cross-compiler toolchain.
    # It compiles loadable kernel modules (.ko files) without building the full kernel image.
    # Skip modules for now
    # make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules

    # Build the devicetree
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories of root filesystem
echo "Creating rootfs!"
sudo mkdir -p -m 755 rootfs
sudo chown jouza:jouza rootfs
cd rootfs
sudo mkdir -p -m 755 bin dev etc home lib lib64 proc sbin sys tmp usr var
sudo chown jouza:jouza bin dev etc home lib lib64 proc sbin sys tmp usr var
sudo mkdir -p -m 755 usr/bin usr/lib usr/sbin
sudo chown jouza:jouza usr/bin usr/lib usr/sbin
sudo mkdir -p -m 755 var/log
sudo chown jouza:jouza var/log

cd "$OUTDIR"
echo "Busybox clone"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
echo "Busybox clean!"
make clean
echo "Busybox make!"
sudo PATH=$PATH make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
echo "Busybox install!"
sudo PATH=$PATH make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

ROOTFS="${OUTDIR}/rootfs"
cd "$ROOTFS"
echo "Library dependencies checking in: "
pwd
# Use the readelf utility to check which library dependencies are needed and store those outputs into variables
INTERPRETER=$( ${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | cut -d: -f2 | sed 's/ \[//g' | tr -d ' ]' | xargs )
NEEDED_LIBS=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | sed 's/.*\[\(.*\)\].*/\1/' | tr '\n' ' ')


# TODO: Add library dependencies to rootfs
# Copy interpreter
# Should be copied to "$ROOTFS/lib"
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)  # Find the sysroot of the toolchain to extract the needed dependencies
TARGET_DIR="${ROOTFS}/lib"
INTERPRETER_SRC=$(find "${SYSROOT}" -wholename "*${INTERPRETER}" | head -1)
echo "SYSROOT dir is $SYSROOT, copying $INTERPRETER_SRC to ${TARGET_DIR}/"
[ -n "${INTERPRETER_SRC}" ] && cp -a "${INTERPRETER_SRC}" "${TARGET_DIR}/"

# Copy each needed lib
# Should be copied to "$ROOTFS/lib64"
echo "Copying shared libraires:"
for lib in ${NEEDED_LIBS}; do
    LIB_SRC=$(find "${SYSROOT}" -name "${lib}" | head -1)
    if [ -n "${LIB_SRC}" ]; then
        cp -a "${LIB_SRC}" "${ROOTFS}/lib64/"
        echo "Copied: ${lib}"
    else
        echo "Missing: ${lib}"
    fi
done


# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3    # For some reason you should always have a dev/null device?
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
cd "$SCRIPT_DIR"
echo "Should now be in the scripts directory that is: $SCRIPT_DIR?"
pwd
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
cp writer* "$ROOTFS/home/"

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp find*.sh "$ROOTFS/home/"
cp -r ${SCRIPT_DIR}/conf/ "$ROOTFS/home/"
cp autorun-qemu.sh "$ROOTFS/home/"
echo "All needed scripts and files copied to the /rfs/home/ directory so that qemu can execute them"

# TODO: Chown the root directory
cd "$ROOTFS"
sudo chown -R root:root *
echo "root directory chowned"

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
echo "ran first cpio cmd"
cd $OUTDIR
gzip -f initramfs.cpio
echo "finitoooo"