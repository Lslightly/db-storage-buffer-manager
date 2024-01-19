#import "TypstTemplates/templates/basic.typ": *

#show: project.with(
  title: "实验 存储与缓存管理器",
  authors: (
    (name: "李清伟 SA23011033", email: "qingweili2357@mail.ustc.edu.cn"),
  ),
)

#let dsmgr = "存储管理器"
#let bmgr = "缓冲管理器"
#let evaluator = "性能评估器"
#let driver = "驱动程序"
#let FixNewPage = ```cpp FixNewPage```
#let hitRatio = "缓冲命中率"
#let io = "磁盘IO次数"
#let time = "运行时间"
#let victimNum = "被替换的数量"
#let maintainTime = "数据结构维护时间"

= 摘要

本项目实现了数据库管理系统（DBMS）中不考虑并发情况下的#dsmgr、#bmgr。在缓冲替换策略上，本项目实现了naive版本、LRU、LRU-K、Clock算法，并在通过#FixNewPage 构建好的包含50000个页的堆文件上实现按照trace文件操作数据库堆文件进行了#io、#hitRatio、#time、#victimNum、#maintainTime 等性能数据的测量与对比。本项目有以下发现：1) LRU-2策略相比于其他版本的替换策略在#io、#hitRatio、#victimNum 指标上效果更好。2) LRU-k中k的设置不是越大越好。3) LRU-k(k=2,3,4)策略在#io、#hitRatio、#victimNum 上效果比LRU好。4) Clock算法在#time 和#maintainTime 上比LRU系算法好，但是在#io、#hitRatio、#victimNum 指标上效果较差。

= 管理器设计

本项目主要有4部分组成，包含#dsmgr、#bmgr、#evaluator、#driver。

== #dsmgr

#let ReadPage = ```cpp ReadPage```
#let WritePage = ```cpp WritePage```

#dsmgr 使用page ID作为索引方式。一个page的大小为FRAMESIZE = 4KB。一个page的从文件开始的偏移为[pageID \* pageSize, (pageID+1)\* pageSize)。#dsmgr 主要提供了#ReadPage 和#WritePage 接口供#bmgr 调用，分别对应读、写操作。每次读写操作的大小为1个page大小。其他接口功能比较简单直接，因此不再赘述。

== #bmgr

#let FixPage = ```cpp FixPage```
#let UnfixPage = ```cpp UnfixPage```
#let BUFSIZE = ```cpp BUFSIZE```
#let globalbuf = ```cpp GlobalBuf```
#let BCB = ```cpp BCB```

一个事务通过#FixPage 接口请求一个已经存在的page。当上层事务使用完这个page之后，会通过#UnfixPage 来提示#bmgr 对该page的使用结束，可以将记录使用该page的数量减少。当上层事务要求新page时，会通过#FixNewPage 请求获得新page。

#bmgr 主要是因为存在加速上层索引、文件、记录管理器请求响应的需求而出现的。为了达到这一目的，#bmgr 将频繁访问的page放到内存中（这里为#globalbuf），并且#BCB 来存储这个page的状态信息，包括是否dirty，当前使用该page的事务数量，frame和page的映射、是否加锁等。

由于内存资源有限，因此当缓冲被填满时，需要将一些替换代价低的页替换出去，并从#dsmgr 中获得请求所需要的页，并加入到缓冲中，维护相关信息。在选择受害者时，存在替换策略的问题，本项目实现了常见的替换策略LRU、LRU-K、Clock，并且还实现了一个非常简单的naive版本。

=== page和frame对应关系设计

#let ra = $->$
#let f2p = ```cpp f2p```
#let p2bcb = ```cpp p2bcb```

本项目中page和frame的关系如@f2p_p2bcb 所示。只要有一个frame是空闲的，就会将其分配给没有frame对应的新page。分配完成之后，会将frame #ra page的映射存入#f2p 中。而page #ra frame的关系则通过键为page，值为#BCB 链表的哈希表#p2bcb 进行存储。具体来说，要从一个page找到frame ID，需要：
+ 通过哈希函数得到 $"pageHash" = "Hash"("page ID")$
+ 通过pageHash和#p2bcb 得到#BCB 链表头指针
+ 在#BCB 链表中比对#BCB 中存储的page ID，得到#BCB 之后返回frame ID

#figure(
image("figures/f2p_p2bcb.png", width: 40%),
caption: "f2p和p2bcb结构"
) <f2p_p2bcb>

=== #BCB 链表

BCB结构定义如@BCBdef 所示。

#figure(
```cpp
class BCB {
public:
    BCB(int pageid, int frameid)
    : pageID(pageid), frameID(frameid), latch(0), count(0), dirty(0), next(nullptr) {}
    int pageID;
    int frameID;
    int latch;
    int count;
    int dirty;
    BCB* next;
};
```,
caption: "BCB结构定义",
) <BCBdef>

=== #FixPage 操作流程

操作流程如@FixPageFlow 所示。

=== 替换流程

#let SelectVictim = ```cpp SelectVictim```

在@FixPageFlow 的操作流程中，需要通过#SelectVictim 选择合适的受害者并进行替换，同时还需要维护#BCB, #f2p, #p2bcb 以及替换策略实现者内部的数据结构。#SelectVictim 的流程如@SelectVictimFlow 所示。

#grid(
  columns: 2,
  [
    #figure(
    image("figures/FixPage.png", width: 45%),
    caption: [#FixPage 操作流程图]
    ) <FixPageFlow>
  ],
  [
    #figure(
    image("figures/SelectVictim.png", width: 80%),
    caption: [#SelectVictim 操作流程图]
    ) <SelectVictimFlow>
  ]
)

=== 替换策略

==== naive

每次只选择0号frame进行替换。见#link("../src/BMgr.cc")[../src/BMgr.cc]

==== LRU

每次选择队尾作为victim。每次访问都放到队头（如果在LRU链表中存在，则先删除，后放到队头），见#link("../src/LRUBMgr.cc")[../src/LRUBMgr.cc] 50行以上。

==== LRU-K

维护两个列表，访问次数 $>=$ K的放到一个热表，否则放到另一个冷表。优先替换冷表。见#link("../src/LRUBMgr.cc")[../src/LRUBMgr.cc] 50行以下。

==== Clock

#let cpp(t) = raw(t, lang: "cpp")
#let current = cpp("current")
#let referenced = cpp("referenced")

当clock未满的时候，不断增加#current 并填充新访问的frameID，设置#referenced 为true。当clock满的时候，如果要访问的frame 在clock中存在，则将相应项中的#referenced 设置为true。否则表示已经替换了受害者页，需要设置新的frame ID。此时直接在#current 指针下设置frame ID并设置#referenced 为true。

在选择受害者时，不断将#referenced == true的项设置为#referenced = false。一旦找到一个#referenced 为false的项，则作为受害者返回frame ID。

== #evaluator <eval>

性能评估器主要用于评估#io、#hitRatio、#time、#victimNum、#maintainTime。其中：
+ #io 次数通过#dsmgr 的#ReadPage 和#WritePage 接口的调用次数确定。
+ #hitRatio 通过#FixPage 中是否找到#BCB 确定。如果找到，则为hit，否则为miss。
+ #time 为所有Read, Write以及最后的WriteDirtys操作的时间求和组成。
+ #victimNum 为#SelectVictim 对被替换页的数量，包括被替换的干净页、脏页。干净和脏通过#BCB 的dirty位进行区分。
+ #maintainTime 主要通过插桩得到，是不同替换策略维护数据结构所花费的时间累计和。

源代码见#link("../include/Evaluator.h")[../include/Evaluator.h]

== 驱动程序

驱动器主要是用于解析待测负载文件并转换为对#bmgr 的操作序列。同时驱动器还用于驱动生成性能测试数据。源代码见#link("../main.cpp")[../main.cpp]

= 运行结果

运行方式见#link("../README.md")[../README.md]。

统计指标见@eval 的说明。

== 不同替换策略的性能对比

不同策略的性能对比如@io12288 - @maintain12288 所示。可以发现LRU-2策略相比于其他版本的替换策略在#io、#hitRatio、#victimNum 指标上效果更好。LRU-k(k=2,3,4)策略在#io、#hitRatio、#victimNum 上效果比LRU好。LRU算法的#io 甚至比不过naive版本的实现。

在LRU-k系列算法中，随着K增大，#io 并没有减少，#hitRatio 也有所降低，#victimNum 也有所减少。说明k不是越大越好。

同时Clock算法相比于LRU系算法在#time 和#maintainTime 上效果较好，但是#io、#hitRatio、#victimNum 指标上效果甚至比naive算法还要差。

#grid(
  rows: 2,
  column-gutter: 0.5em,
  columns: 2,
  [
      #figure(
        image("figures/io12288.png"),
        caption: [BUFSIZE=12288时不同策略的#io],
      ) <io12288>
      #figure(
        image("figures/victim12288.png"),
        caption: [BUFSIZE=12288时不同策略的#victimNum],
      ) <victim12288>
  ],
  [
      #figure(
        image("figures/hitRatio12288.png", width: 90%),
        caption: [BUFSIZE=12288时不同策略的#hitRatio],
      ) <hitRatio12288>
      #figure(
        image("figures/totalT12288.png"),
        caption: [BUFSIZE=12288时不同策略的#time],
      ) <time12288>
  ]
)
#figure(
  image("figures/maintainT12288.png", width: 50%),
  caption: [BUFSIZE=12288时不同策略的#maintainTime]
) <maintain12288>

== LRU-2算法在不同BUFSIZE下的性能变化

不同BUFSIZE对于替换策略的效果也有影响。对于LRU-2算法来说，其#io、#hitRatio、#time 的变化如@lrubegin - @lruend 所示。当BUFSIZE增大时，#io、#hitRatio、#time 效果都显著变好。但是当BUFSIZE进一步扩大时，#time 并没有进一步减少，主要是因为BUFSIZE太大导致维护代价变高，如@lrumaintain 所示。

#grid(
  rows: 2,
  columns: 2,
  column-gutter: 1em,
  [
    #figure(
      image("figures/lru-io.png"),
      caption: [LRU-2#io 随BUFSIZE变化折线图]
    ) <lrubegin>
  ],
  [
    #figure(
      image("figures/lru-hitRatio.png"),
      caption: [LRU-2#hitRatio 随BUFSIZE变化折线图]
    )
  ],
  [
    #figure(
      image("figures/lru-time.png"),
      caption: [LRU-2#time 随BUFSIZE变化折线图]
    ) <lruend>
  ],
  [
    #figure(
      image("figures/lru-maintain.png"),
      caption: [LRU-2#maintainTime 随BUFSIZE变化折线图]
    ) <lrumaintain>
  ]
)

= 总结

本项目实现了#dsmgr、#bmgr 以及多种替换策略。在替换策略的对比中发现LRU-2的效果较好。LRU-k中k的设置不是越大越好。Clock算法在#time 和#maintainTime 上比LRU系算法好，但是在#io、#hitRatio、#victimNum 指标上效果较差。
