SUMMARY = "SPI device demo application"
SECTION = "examples"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WORKDIR}/COPYRIGHT;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI  = "file://main.c"
SRC_URI += "file://canvascmd.c"
SRC_URI += "file://canvascmd.h"
SRC_URI += "file://fbscreen.c"
SRC_URI += "file://fbscreen.h"
SRC_URI += "file://spidevice.c"
SRC_URI += "file://spidevice.h"
SRC_URI += "file://config.h"
SRC_URI += "file://canvas_common.h"
SRC_URI += "file://readme.txt"
SRC_URI += "file://COPYRIGHT"

S = "${WORKDIR}"

inherit autotools gettext

FILES_${PN} = "${bindir}/*"
FILES_${PN}-dbg = "${bindir}/.debug/*"

do_compile() {
	${CC} -Wall -lm \
		${S}/main.c \
		${S}/canvascmd.c \
		${S}/fbscreen.c \
		${S}/spidevice.c \
		-o ${B}/openrex_spi_canvas
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${B}/openrex_spi_canvas ${D}${bindir}
}
