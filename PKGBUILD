# This is an example PKGBUILD file. Use this as a start to creating your own,
# and remove these comments. For more information, see 'man PKGBUILD'.
# NOTE: Please fill out the license field for your package! If it is unknown,
# then please put 'unknown'.

# Maintainer: Your Name <youremail@domain.com>
pkgname=igal_qt
pkgver=1.5.2
pkgrel=1
epoch=
pkgdesc="igal_qt"
arch=('x86_64')
url="https://github.com/ien646/igal_qt"
license=('GPL')
groups=()
depends=('qt6-base' 'qt6-multimedia')
makedepends=('gcc' 'cmake' 'git')
checkdepends=()
optdepends=()
backup=()
options=()
source=("$pkgname-$pkgver::https://github.com/ien646/igal_qt/archive/refs/tags/$pkgver.tar.gz")
sha256sums=('f3da5d4751b33f46b180195539012d729d255b68d2c03358426ffb644bf1a0f9')

prepare() {
	cd "$pkgname-$pkgver"
}

build() {
	cd "$pkgname-$pkgver"
	mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
	cmake --build .
}

package() {
	cd "$pkgname-$pkgver"
	make install
}
