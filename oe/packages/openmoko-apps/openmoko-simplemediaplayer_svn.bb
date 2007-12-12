DESCRIPTION = "The OpenMoko Media Player"
SECTION = "openmoko/applications"
DEPENDS += "alsa-lib dbus-glib id3lib libvorbis"
PV = "0.0.1+svn${SRCDATE}"
PR = "r3"

inherit openmoko

FILES_${PN} = "${bindir}/* \
               ${libdir}/* \
               ${datadir}/*"