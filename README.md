# axomavis
Video surveillance software

- Saving video streams on a file system split into files

### Example source streams json file 
```json 
[
  {
    "id":"camera-1",
    "url":"rtsp://admin:admin@192.168.0.1:554/stream"
  },
  {
    "id":"camera-2",
    "url":"rtsp://admin:admin@192.168.0.2:554/stream"
  }
]
```

### Build and run tests
```bash
mkdir build
cd build/
cmake -DTEST=on  ..
make
make test
```