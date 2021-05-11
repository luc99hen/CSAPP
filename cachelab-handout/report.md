## Part A

> Task: A Cache Simulator use LRU strategy.

A Naive implementation, use `last_tick` in `CacheLine` to track this line's last access time, and iterate over the line list to find the oldest line in a group to evict when eviction is needed.

### Lessons

Confuse the input `asso_nums` into `asso_bits` and get lost when results is wrong.


## Part B

> Task: Matrix Transpose with least cache miss.

The basic idea behind all approach is to maximize the utlization of a `CacheLine` and avoid collision eviction. 


### Approaches

1. transpose by block, divide a matrix into several k x k blocks, then transpose by block. For example, transpose block $A_{1,2}$ into block $B_{2,1}$. Since the two blocks fit into different CacheLines, all the read and write operations are not supposed to lead to collision and eviction.
2. For the block on the diagonal, for example $A_{1,1}$ and $B_{1,1}$, they are mapped into the same CacheLine. Therefore, they should be handled in a different way.
   1. Put the diagonal block in a reversed order. For example, put $A_{1,1}$ not in $B_{1,1}$ but in $B_{k-1,k-1}$. Then we can avoid confict eviction.
   2. Use additional variables as cache (literally), then we can read a `CacheLine` continuously and put their values in temporal varaibles. Then move those values into their destinations. Then the collision would be avoided.
3. These strategies above are enough to deal with the 32x32 and 61x67 case, but fail to meet the criteria for 64x64. We are in an awkward situation when deciding how large the block size should be. If the block size is larger than 4, than we can't take advantage of the collision-free read&write property within a block, beacause the whole cache can accommodate at most 4 single rows. If the block size if too small, then we can't utlize the full campacity of a `CacheLine` which is 8, but we can only offer at most 4.  **In other words, the block size has both the limitation of 8 from the Cache Size and the limitation of 4 from the Cache utilizatino rate.**
Therefore, a hybrid approach is proposed for this. In this way, we divide the matrix into 8x8 block, but do the internal block transformation in a 4x4 way.
Sometimes we use the continuous 8 integer from a single line to maximize cache utilization. At the same time, we also conform to the 4-row limitation to avoid collision. The concrete implementation details are too complicate to put it in words and you can find more [here](https://www.cnblogs.com/liqiuhao/p/8026100.html) and [here](https://zhuanlan.zhihu.com/p/28585726).


This lab is the **most difficut** lab among all labs which I met in CSAPP. And the hardest part is not to come up with an idea but how to prove this idea is better than others. I find [this method for rough estimate](https://zhuanlan.zhihu.com/p/28585726) inspring and thoughtful. It's relatively rough but effective.


### Lessons

- Thinking
  - Visualization is very important and it will boost productivity espacially when my abstraction ability met the ceiling. By the way, Excel form is handy to visualize the whole Cache structure.
  - When thinking hits a bottleneck, it's better to seek help in time than to get stuck in place.
- Cache
  - process by rows (or by cols based on the internal store direction)
  - batch update (read a sequence into a Cache block, access within this block; then go for the next block)
