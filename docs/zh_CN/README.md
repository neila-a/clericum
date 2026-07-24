# clericum

[English](/docs/en_US/README.md) | 大陆简体中文

一个基于 FUSE 的文件备份工具。

## 什么是 clericum

*clericum* 在拉丁语中意为“书记”。

## 编译

```bash
nix build
```

## 已知问题

- 在一些命令中必须使用绝对路径。
- 在 FileItemAction 中加载仓库可能会导致 Dolphin 冻结。  
  不过仓库仍会正确加载。
