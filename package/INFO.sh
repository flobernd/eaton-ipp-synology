#!/bin/sh

. /pkgscripts/include/pkg_util.sh

package="eaton-ipp"
version="1.75.183-1"
os_min_ver="7.0-40761"
displayname="Eaton Intelligent Power Protector"
arch="$(pkg_get_platform_family)"
start_dep_services="network.target"
maintainer="Florian Bernd"
description="Eaton's Intelligent Power Protector (IPP) software provides graceful, automatic shutdown of network devices during a prolonged power disruption, preventing data loss and saving work-in-progress."
support_url="https://www.eaton.com/us/en-us/catalog/backup-power-ups-surge-it-power-distribution/eaton-intelligent-power-protector.html"
adminprotocol="https"
adminport="4680"
checkport="no"
ctl_stop="yes"
thirdparty="yes"
install_type="system"
silent_install="yes"
silent_upgrade="yes"
silent_uninstall="yes"
dsmuidir="ui"
dsmappname="com.eaton.IPP"
support_center="no"

[ "$(caller)" != "0 NULL" ] && return 0

pkg_dump_info
