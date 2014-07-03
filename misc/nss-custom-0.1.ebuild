# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5
inherit cmake-utils

DESCRIPTION="Name Service Switch (NSS) custom lists."
HOMEPAGE="https://github.com/deadskif/nss_custom "
SRC_URI="https://github.com/deadskif/nss_custom/archive/master.zip"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~arm ~x86"
IUSE="+syslog test"

DEPEND=""
RDEPEND="${DEPEND}"

S="${WORKDIR}/nss_custom-master"

src_configure() {
    local mycmakeargs=(
        $(cmake-utils_use_enable test TEST)
        $(cmake-utils_use_enable syslog SYSLOG)
    )
    cmake-utils_src_configure
}

src_compile() {
    cmake-utils_src_compile
}

src_install() {
    cmake-utils_src_install
}

