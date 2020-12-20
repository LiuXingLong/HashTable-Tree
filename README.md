# BST

### Hash Map (基于 Binary Search Tree 实现)

哈希表 + 二叉搜索树 实现 key => value 数据存储与修改。

特点：千万级 key => value 数据，插入、查询、修改、删除 毫秒级实现。



### 红黑树实现

插入节点初始都为红色

1、节点必须是红色或者黑色。
2、根节点必须是黑色。
3、叶节点(NIL)是黑色的。（NIL节点无数据，是空节点）
4、红色节点必须有两个黑色儿子节点。
5、从任一节点出发到其每个叶子节点的路径，黑色节点的数量是相等的。


#### 插入操作

1、情况一：插入节点为根节点，将插入节点修改为黑色。

2、情况二：插入节点的父亲节点为黑色，直接插入即可。

3、情况三：插入N节点的叔父(U)都为红色；将父亲(P)和叔父(U)修改为黑色祖父修改为红色,然后对其祖父做递归进行调整。

                 G(黑)                              G(红)
               /       \                          /       \
             P(红)     U(红)      ===》          P(黑)     U(黑) 
            /                                  /
          N(红)                               N(红)

4、情况四：插入N节点的叔父(U)为黑色,且插入N为右节点,N的父亲同为左节点。对N的父亲节点(P)执行左旋操作，进入情况五状态。
（对称情况:插入N节点的叔父(U)为黑色,且插入N为左节点,N的父亲同为右节点,类似反向处理）

                  G(黑)                              G(黑)
                /       \                         /       \
             P(红)     U(黑)      ===》          N(红)     U(黑) 
            /   \                              /   \
           1     N(红)                       P(红)    3
         /   \                             /   \
        2     3                           1     2

5、情况五：插入N节点的叔父(U)为黑色,且插入N和父亲同为左节点。对N的祖父节点(G)进行右旋，将祖父节点G修改为红色，父亲节点P修改为黑色。
（对称情况:插入N节点的叔父(U)为黑色,且插入N和父亲同为右节点,类似反向处理）

                  G(黑)                              P(红)                            P(黑)
                /       \                          /      \                        /       \
              P(红)     U(黑)      ===》          N(红)    G(黑)        ===》      N(红)     G(红)     
            /      \                            /    \    /    \                /   \     /   \
           N(红)    3                          1      2   3     U(黑)           1    2    3     U(黑)
          /  \
         1    2

