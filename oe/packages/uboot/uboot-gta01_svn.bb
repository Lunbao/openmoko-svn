DESCRIPTION = "U-boot bootloader w/ Neo1973 (GTA01) support"
AUTHOR = "Harald Welte <laforge@openmoko.org>"
LICENSE = "GPL"
SECTION = "bootloader"
PRIORITY = "optional"
PV = "1.2.0+svn${SRCDATE}"
PR = "r11"

PROVIDES = "virtual/bootloader"
S = "${WORKDIR}/git"

SRC_URI = "git://www.denx.de/git/u-boot.git/;protocol=git \
           svn://svn.openmoko.org/trunk/src/target/u-boot;module=patches;proto=http \
	   file://uboot-20070311-tools_makefile_ln_sf.patch;patch=1 \
	  "

EXTRA_OEMAKE = "CROSS_COMPILE=${TARGET_PREFIX}"
TARGET_LDFLAGS = ""
UBOOT_MACHINES = "gta01bv2 gta01bv3 gta01bv4 smdk2440 hxd8 qt2410 gta02v1"

do_quilt() {
        mv ${WORKDIR}/patches ${S}/patches && cd ${S} && quilt push -av
        rm -Rf patches .pc
}

do_svnrev() {
	FILE=${S}/tools/setlocalversion
	OLDFILE=$FILE.old
	NEWFILE=$FILE.new
	cp $FILE $OLDFILE
	LINES=`cat $OLDFILE | wc -l`
	LINES_WE_WANT=$(($LINES-1))
	LASTLINE=`cat $OLDFILE | tail -n 1`
	cat $OLDFILE | head -n $LINES_WE_WANT > $NEWFILE
	echo ${LASTLINE}_${PR} >> $NEWFILE
	rm $FILE && mv $NEWFILE $FILE
}

do_compile () {
	chmod +x board/neo1973/gta*/split_by_variant.sh
	for mach in ${UBOOT_MACHINES}
	do
		oe_runmake ${mach}_config
		oe_runmake clean
		find board -name lowlevel_foo.bin -exec rm '{}' \;
		oe_runmake all
		oe_runmake u-boot.udfu
		if [ -f u-boot.udfu ]; then
			mv u-boot.udfu u-boot_${mach}.bin
		else
			mv u-boot.bin u-boot_${mach}.bin
		fi
		if [ -f board/${mach}/lowlevel_foo.bin ]; then
			mv board/${mach}/lowlevel_foo.bin \
			    lowlevel_foo_${mach}.bin
		else
			find board -name lowlevel_foo.bin \
			    -exec mv '{}' lowlevel_foo_${mach}.bin \;
		fi
	done
}

do_deploy () {
	install -d ${DEPLOY_DIR_IMAGE}
	for mach in ${UBOOT_MACHINES}
	do
		install ${S}/u-boot_${mach}.bin \
		    ${DEPLOY_DIR_IMAGE}/u-boot-${mach}-${PR}.bin
		if [ -f ${S}/lowlevel_foo_${mach}.bin ]; then
			install ${S}/lowlevel_foo_${mach}.bin \
			    ${DEPLOY_DIR_IMAGE}/lowlevel_foo-${mach}-${PR}.bin
		fi
	done
	install -m 0755 tools/mkimage ${STAGING_BINDIR_NATIVE}/uboot-mkimage
}

do_deploy[dirs] = "${S}"
addtask deploy before do_package after do_install
addtask quilt before do_patch after do_unpack
addtask svnrev before do_patch after do_quilt
