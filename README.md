# axomavis
Video surveillance software

### Add a logging library
```bash
git clone https://github.com/SergiusTheBest/plog
sudo cp -r plog/include/plog /usr/local/include
```

### Build and run tests
```bash
mkdir build
cd build/
cmake -DTEST=on  ..
make
make test
```