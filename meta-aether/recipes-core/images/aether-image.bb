SUMMARY = "Aether OS Initial Image"
DESCRIPTION = "Initial x86-64 development image for Aether OS"

LICENSE = "CLOSED"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh"

IMAGE_INSTALL:append = " \
    aether-shell \
    weston \
    packagegroup-qt6-essentials \
"

IMAGE_FSTYPES:append = " wic"
