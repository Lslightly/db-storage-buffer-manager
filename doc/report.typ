#import "TypstTemplates/templates/basic.typ": *

#show: project.with(
  title: "实验 存储与缓存管理器",
  authors: (
    (name: "李清伟 SA23011033", email: "qingweili2357@mail.ustc.edu.cn"),
  ),
)

#let dsmgr = "存储管理器"
#let bmgr = "缓冲管理器"
#let ReadPage = ```c++ ReadPage```
#let WritePage = ```c++ WritePage```
#let FixPage = ```c++ FixPage```


= 管理器设计

= 运行结果

== 统计指标

根据实验要求，需要统计磁盘IO、Buffer命中率、运行时间等数据。
+ 磁盘IO次数通过统计#dsmgr 的#ReadPage 和#WritePage 次数来表示IO开销。
+ Buffer命中率通过#bmgr 的#FixPage 操作中是否找到对应的BCB块来进行判断。如果没有找到，说明当前page并不在buffer中，miss了。否则为命中。
+ 运行时间会分别统计读操作和写操作的时间。合起来为整体运行时间。

