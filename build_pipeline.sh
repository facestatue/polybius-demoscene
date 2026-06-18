rm main.xz
echo "Compiling..."
gcc -Oz -fno-plt -fno-ident -flto -fuse-linker-plugin -nostdlib -lX11 main.c -o main -lX11
echo "Stripping pre-packaged binary..."
strip --strip-all -v main
echo "Compressing binary and gluing..."
raw_size=$(wc -c < main)
xz -9 --extreme -C none main

cat stub.sh main.xz > SPCSHP
chmod +x SPCSHP

size=$(wc -c < SPCSHP)
echo -e "Initial raw binary build size (no xz): \x1b[1m${raw_size} bytes\x1b[0m" 
echo -e "Final compressed and stripped build size: \x1b[1m${size} bytes\x1b[0m" 

./SPCSHP
exit 0
