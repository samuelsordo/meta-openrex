SUMMARY = "SPI device demo application"
SECTION = "examples"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WORKDIR}/COPYRIGHT;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI  = "file://main.c"
SRC_URI += "file://readme.txt"
SRC_URI += "file://COPYRIGHT"

S = "${WORKDIR}"

inherit autotools gettext

FILES_${PN} = "${bindir}/*"
FILES_${PN}-dbg = "${bindir}/.debug/*"

do_compile() {
	${CC} -Wall ${S}/main.c -o ${B}/openrex_spi_slave
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${B}/openrex_spi_slave ${D}${bindir}
}
