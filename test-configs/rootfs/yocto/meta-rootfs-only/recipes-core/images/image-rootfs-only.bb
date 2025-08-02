DESCRIPTION = "Minimal rootfs-only image for riscv64 (no kernel)"
LICENSE = "MIT"

inherit core-image

VIRTUAL-RUNTIME_dev_manager ?= "busybox-mdev"
INIT_MANAGER = "mdev-busybox"

IMAGE_LINGUAS = " "
IMAGE_INSTALL = "packagegroup-core-boot \
   dropbear \
   openssh-sftp-server \
   gdbserver \
   init-ifupdown \
   i2c-tools \
   procps \
   readline \
   wget \
   sqlite3 \
   mc \
   htop \
   ${VIRTUAL-RUNTIME_dev_manager} \
   ${CORE_IMAGE_EXTRA_INSTALL} \
"

#   mariadb-client
#   libmariadb


# Avoid installing the kernel and kernel modules
#IMAGE_INSTALL:remove = "kernel kernel-image kernel-modules"

# Optional: reduce size, allow login without password
#IMAGE_FEATURES += "empty-root-password"

# to include RPM database into the target
EXTRA_IMAGE_FEATURES += "package-management"

# Rootfs output formats
#IMAGE_FSTYPES += "tar.gz ext4"

