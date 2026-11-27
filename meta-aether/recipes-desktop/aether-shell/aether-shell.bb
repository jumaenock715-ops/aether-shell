SUMMARY = "Aether OS Graphical Shell"
DESCRIPTION = "The native Qt Quick desktop shell for Aether OS"

# Private-development setting only. Select a distribution licence before release.
LICENSE = "CLOSED"

SRC_URI = "git://github.com/jumaenock715-ops/aether-shell.git;protocol=https;branch=main"
SRCREV = "bcf00e88cb67591cfdf011ad3f8f635fdb2afd77"

S = "${WORKDIR}/git"

inherit qt6-cmake

DEPENDS += "qtbase qtdeclarative"

RDEPENDS:${PN} += " \
    qml6-module-qtquick \
    qml6-module-qtquick-window \
"

EXTRA_OECMAKE += "-DCMAKE_BUILD_TYPE=Release"

FILES:${PN} += "${bindir}/aether-shell"
