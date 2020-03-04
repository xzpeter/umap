#!/bin/bash

ROOT="$(pwd)"

mkdir -p deps
mkdir -p build
INSTALL_DIR=$ROOT/build

cd deps

repos=(
    https://github.com/pmodels/argobots.git
    https://github.com/mercury-hpc/mercury.git
    https://xgitlab.cels.anl.gov/sds/margo.git
)

for i in "${repos[@]}" ; do
	# Get just the name of the project (like "mercury")
	name=$(basename $i | sed 's/\.git//g')
	if [ -d $name ] ; then
		echo "$name already exists, skipping it"
	else
		if [ "$name" == "mercury" ] ; then
			git clone --recurse-submodules $i
		else
			git clone $i
		fi
	fi
done


echo "### building argobots ###"
cd argobots
git checkout v1.0rc2
./autogen.sh && CC=gcc ./configure --prefix="$INSTALL_DIR"
make -j $(nproc) && make install
cd ..



echo "### building mercury ###"
cd mercury
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
      -DMERCURY_USE_BOOST_PP=ON \
      -DMERCURY_USE_CHECKSUMS=ON \
      -DMERCURY_USE_EAGER_BULK=ON \
      -DMERCURY_USE_SYSTEM_MCHECKSUM=OFF \
      -DNA_USE_OFI=ON \
      -DMERCURY_USE_XDR=OFF \
      -DBUILD_SHARED_LIBS=on ..
make -j $(nproc) && make install
cd ..
cd ..


echo "### building margo ###"
cd margo
git checkout v0.4.3
export PKG_CONFIG_PATH="$INSTALL_DIR/lib/pkgconfig"
./prepare.sh
./configure --prefix="$INSTALL_DIR"
make -j $(nproc) && make install

cd "$ROOT"

echo "*************************************************************************"
echo "Dependencies are all built.  You can now build with:"
echo ""
echo "  export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig "
echo "  cd build && cmake3 -DCMAKE_INSTALL_PREFIX=. -DCMAKE_BUILD_TYPE=Release .. "
echo "  make"
echo "  make install"
echo "*************************************************************************"
