echo "build win32 release"
rmdir /s /q build
mkdir build
cd build
cmake ../
cmake --build ./ --config Release

cd ..
rmdir /s /q build


echo "build win64 release"
mkdir build
cd build
cmake ../ -DCMAKE_GENERATOR_PLATFORM=x64
cmake --build ./ --config Release


