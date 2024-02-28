# 喷泉码

对本地文件使用喷泉码进行编解码

针对编码使用的随机种子，以其定长版本为基础，设计了可变长度版本

在 Arch 下使用 clang++ 16 和 g++ 编译通过

在 Windows 11 下使用 clang++ 16 和 MSYS2 的 UCRT64 g++ 编译通过

## 使用方法

以定长种子版本为例：

```powershell
# 编码
static_encoder.exe <待编码文件路径> <编码块大小(B)> <编码包数量>
# 解码
static_decoder.exe <待解码文件路径> <编码块大小(B)> <原始文件大小(B)>
```

编码得到的 `encode.bin` 和解码得到的 `decode.bin` 均存放在 `./data` 目录下
