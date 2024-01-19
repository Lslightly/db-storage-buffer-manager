# db-storage-buffer-manager

Storage and Buffer Manager. Advanced Database Course Lab in 2023Fall.

实验报告见[./doc/report.pdf](./doc/report.pdf)

## 运行说明

运行平台ubuntu22.04, C++17标准。

```bash
./dependency.sh # 安装依赖
./all.sh # 运行性能评估
```

## 目录结构说明

```bash
[0] .
├── [1] .gitignore
├── [2] CMakeLists.txt
├── [3] README.md                   本文件
├── [4] all.sh                      性能测试脚本
├── [5] build.sh                    项目构建脚本
├── [6] dependency.sh               依赖脚本
├── [7] doc
│   ├── [8] TypstTemplates
│   ├── [9] adbs-lab.pdf
│   ├── [10] figures
│   ├── [11] report.pdf
│   └── [12] report.typ
├── [13] eval                       性能测试结果
│   ├── [14] Clock
│   ├── [15] LRU-2
│   ├── [16] LRU-3
│   ├── [17] LRU-4
│   ├── [18] LRUBMgr
│   └── [19] naiveBMgr
├── [20] include
│   ├── [21] BMgr.h                 缓存管理器
│   ├── [22] ClockBMgr.h            Clock替换策略
│   ├── [23] DSMgr.h                存储管理器
│   ├── [24] Evaluator.h            性能评估器
│   ├── [25] LRUBMgr.h              LRU替换策略
│   ├── [26] argparse
│   ├── [27] defer
│   └── [28] spdlog
├── [29] main.cc                    驱动
├── [30] src
│   ├── [31] BMgr.cc
│   ├── [32] CMakeLists.txt
│   ├── [33] ClockBMgr.cc
│   ├── [34] DSMgr.cc
│   └── [35] LRUBMgr.cc
├── [36] test.sh                    单元测试脚本
├── [37] test
│   ├── [38] data-5w-50w-zipf.txt
│   ├── [39] info.py
│   └── [40] mini.txt
└── [41] unittest                   单元测试
    ├── [42] CMakeLists.txt
    ├── [43] SameHashReplaceTest.cc
    ├── [44] hello_test.cc
    └── [45] logtest.cc
```


