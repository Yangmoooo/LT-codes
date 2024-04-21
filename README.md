# Luby transform codes

对本地文件使用卢比变换码进行编解码。

针对编码使用的伪随机数种子，以其定长版本为基础，设计了长度可变的版本。

动态种子长度的版本最多支持 2^27^ 个编码包，但建议不要使用超过 1,000,000 个。

度分布函数：鲁棒孤子分布

解码算法：置信传播

校验和：CRC-8 ($x^8 + x^2 + x + 1$)

## 使用方法

以静态种子长度的版本为例：

```bash
# 编码
./encoder-static <待编码文件路径> <编码块大小(B)> <编码包数量>
# 解码
./decoder-static <待解码文件路径> <编码块大小(B)> <原始文件大小(B)>
```

编解码分别会按文件名生成 `.enc` 和 `.dec` 后缀的二进制文件

## 注意

使用汉明码是为了确保能够（在发生 1 位错的情况下）读出正确的种子长度，但实际上，如果编码了足够数量的冗余包，且某个包的种子长度字段出错发生在编码文件靠前的位置，也会有一定的几率完成解码。这是因为虽然出错位置后续读取的数据发生错位，但累加的错位偏移最终达到了包本身的长度，重新将数据对齐，使得这之后的包又能够被正确读取。
