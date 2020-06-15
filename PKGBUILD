# Maintainer: Francisco Lopes <francisco@oblita.com>
pkgname=caps2esc
pkgver=1.0.4
pkgrel=3
pkgdesc="caps2esc: transforming the most useless key ever in the most useful one"
arch=('x86_64')
license=('GPL3')
url="https://github.com/oblitum/caps2esc"
depends=('libevdev')
makedepends=('gcc')
source=()
md5sums=()

build() {
    # gcc caps2esc.c -o caps2esc -I/usr/include/libevdev-1.0 -levdev -ludev
    make
}

package() {
    mkdir -p "${pkgdir}/usr/bin"
    install -m 755 caps2esc "${pkgdir}/usr/bin"

    mkdir -p "${pkgdir}/usr/lib/systemd/system"
    install -m 644 "${srcdir}/caps2esc.service" "${pkgdir}/usr/lib/systemd/system"
}
