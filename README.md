# Luby transform codes

对本地文件使用卢比变换码进行编解码

度分布函数：鲁棒孤子分布

解码算法：置信度传播

校验和：CRC-8 (x^8^ + x^2^ + x + 1)

针对编码使用的随机种子，以其定长版本为基础，设计了变长版本，又分为使用/未使用汉明码保护种子长度字段的两个版本

变长版本在使用汉明码的情况下最多支持 2^27^ 个编码包，否则可支持 2^30^ 个

## 使用方法

以定长种子版本为例：

```bash
# 编码
./encoder-static <待编码文件路径> <编码块大小(B)> <编码包数量>
# 解码
./decoder-static <待解码文件路径> <编码块大小(B)> <原始文件大小(B)>
```

编解码分别会按文件名生成 `.enc` 和 `.dec` 后缀的二进制文件
