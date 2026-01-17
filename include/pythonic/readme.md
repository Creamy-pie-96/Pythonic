# step 1
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/home/DATA/CODE/code/usingpythonic/install
cmake --build . --target install -j2
ls -l /home/DATA/CODE/code/usingpythonic/install