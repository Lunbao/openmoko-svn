DESCRIPTION = "Standard sound theme for the OpenMoko distribution"
SECTION = "openmoko/base"
PV = "0.1+svn${SRCDATE}"
PR = "r0"

inherit openmoko-base autotools

SRC_URI = "${OPENMOKO_MIRROR}/src/target/${OPENMOKO_RELEASE}/artwork;module=sounds;proto=http"
S = "${WORKDIR}/sounds"

do_install() {
        find ${WORKDIR} -name ".svn" | xargs rm -rf
        install -d ${D}${datadir}/openmoko/sounds
        for i in *.mp3; do
                cp -fpPR ${S}/$i ${D}${datadir}/openmoko/sounds/
        done
        for i in touchscreen_click.wav notify_doorbell.wav startup_openmoko.wav; do
                cp -f ${S}/$i ${D}${datadir}/openmoko/sounds/
        done
}

FILES_${PN} = "${datadir}"

