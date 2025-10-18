#!/bin/bash

rm -r dependencies
mkdir dependencies

pushd dependencies > /dev/null

echo "Downloading and unzipping gpg error"
wget https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.56.tar.bz2
tar -xvf "libgpg-error-1.56.tar.bz2"
rm "libgpg-error-1.56.tar.bz2"
echo "Compiling gpg error"
pushd "libgpg-error-1.56" > /dev/null
./autogen.sh
./configure --prefix=/workspace/dependencies/gpgerror-ins --enable-install-gpg-error-config
make -j`nproc` && make install
popd > /dev/null

echo "Downloading and unzipping gcrypt"
wget https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.11.2.tar.bz2
tar -xvf "libgcrypt-1.11.2.tar.bz2"
rm "libgcrypt-1.11.2.tar.bz2"
pushd "libgcrypt-1.11.2" > /dev/null
./configure --prefix=/workspace/dependencies/gcrypt-ins --with-libgpg-error-prefix=/workspace/dependencies/gpgerror-ins/
make -j`nproc` && make install
popd > /dev/null

echo "Downlaind and unzipping nettle"
wget https://ftp.gnu.org/gnu/nettle/nettle-3.10.2.tar.gz
tar -xvf "nettle-3.10.2.tar.gz"
rm "nettle-3.10.2.tar.gz"
pushd "nettle-3.10.2" > /dev/null
./configure --prefix=/workspace/dependencies/nettle-ins
make -j`nproc` && make install
popd > /dev/null

echo "Pulling and installing mpich"
git clone https://github.com/pmodels/mpich.git --recursive
pushd mpich > /dev/null
./autogen.sh 
./configure --prefix=/workspace/dependencies/mpich-ins --disable-fortran
make -j`nproc` && make install
popd > /dev/null

popd > /dev/null
