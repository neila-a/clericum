# clericum

English | [大陆简体中文](/docs/zh_CN/README.md)

A FUSE-based file backup tool.

## What's clericum

*clericum* means "cleric" in Latin.

## Compilation

```bash
nix build
```

## Known Issues

- Absolute paths must be used in some commands.
- In FileItemAction, loading the store can cause Dolphin to freeze.  
  But the store still loads correctly.
