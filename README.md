# fibril

## Install

```
./bootstrap
./configure
make
make install
```

## Test
```
make check
```

## Benchmark
By default, `make check` will run standard tests AND benchmarks. To run benchmarks only,

```
cd benchmark
make check
```

To run the benchmarks with serial version, do
```
cd benchmark/serial
make check
```

You can also compare the performance of **fibril** with **Intel CilkPlus**, or **Intel Threading Building Blocks**. To run these versions, you have to have a compiler that supports these frameworks. GCC 5+ supports Intel CilkPlus natively. To run these benchmarks, do
```
cd benchmark/[cilkplus or tbb]
make check
```


