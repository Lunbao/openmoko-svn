DESCRIPTION = "The OpenMoko address book"
LICENSE = "GPL"
SECTION = "openmoko/pim"
DEPENDS += "glib-2.0 gtk+ libglade eds-dbus gnome-vfs openmoko-libs"
RDEPENDS = "gnome-vfs-plugin-file"
RRECOMMENDS = "gnome-vfs-plugin-http"
PV = "0.1+svn${SRCDATE}"
PR = "r0"

inherit openmoko

SRC_URI += "\
  file://stock_contact.png \
  file://stock_person.png"

EXTRA_OECONF = "--enable-gnome-vfs"

do_install_append () {
	install -d ${D}/${datadir}/pixmaps
	install -m 0644 ${WORKDIR}/stock_contact.png ${D}/${datadir}/pixmaps
	install -m 0644 ${WORKDIR}/stock_person.png ${D}/${datadir}/pixmaps
}

FILES_${PN} += "${datadir}/pixmaps/stock_contact.png \
		${datadir}/pixmaps/stock_person.png"

