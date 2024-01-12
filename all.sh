sudo apt install ninja-build
cmake -S . -G Ninja -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
./build.sh
