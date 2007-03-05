DESCRIPTION = "Standard Gtk+ theme for the OpenMoko distribution"
SECTION = "openmoko/base"
PV = "0.0+svn${SRCDATE}"
PR = "r3"

inherit openmoko-base

SRC_URI = "${OPENMOKO_MIRROR}/src/target/${OPENMOKO_RELEASE}/artwork;module=themes;proto=http"
S = "${WORKDIR}"

dirs = "themes/openmoko-standard"

do_install() {
	find ${WORKDIR} -name ".svn" | xargs rm -rf
	install -d ${D}${datadir}/themes/
	for i in ${dirs}; do
		cp -fpPR ${WORKDIR}/$i ${D}${datadir}/themes/
	done
	
	install -d ${D}${sysconfdir}/gtk-2.0
	echo 'include "${datadir}/themes/openmoko-standard/gtk-2.0/gtkrc"' >> ${D}${sysconfdir}/gtk-2.0/gtkrc
}

FILES_${PN} = "${datadir} ${sysconfdir}"
