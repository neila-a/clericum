license="GPL-3.0-or-later"
version="0.1.0"
authors=("Neila")

# 生成的补全脚本输出路径（相对 completor.bash 所在目录解析）。
# completor.bash 位于 src/cli/，故 "../../share/..." 解析到仓库根目录下的 ./share。
# 与 flake.nix 中的 "mv ./share $out" 配合，最终安装到
# $out/share/bash-completion/completions/clericum。
output="../../share/bash-completion/completions/clericum_cli"
cmd="clericum_cli"
subcmds=(backup store)

# 每个子命令必须存在一个同名的 opts 数组（可为空），
# 否则 bash-completor 不会生成该子命令的补全函数。
subcmd_opts_store=()
subcmd_opts_backup=()

# ---------------------------------------------------------------------------
# 自定义 reply 函数：处理「子命令 动作」两级结构，
# 动作之后根据语义补全文件/目录。
#
# 变量命名约定（来自 bash-completor）：
#   - 以 reply_ 开头的函数会被复制到生成的脚本中（重命名为 _clericum_comp_reply_*）。
#   - 以 reply_ 开头的数组变量会把其首个元素当作函数名，生成一个转发包装。
# 因此这里用 reply_store_impl / reply_backup_impl 作为真正实现，
# 再用 reply_store / reply_backup 数组指向它们，最后通过 @store / @backup 引用。
# ---------------------------------------------------------------------------

# store: create <path> | load <store> <path> | unload <path>
reply_store_impl() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local action="${COMP_WORDS[2]:-}"
  case "$action" in
    load)
      # 仅补全包含 .clericum-store 标记的 store 目录
      compopt -o nospace -o filenames
      local d
      while IFS= read -r d; do
        [[ -f "$d/.clericum-store" ]] && COMPREPLY+=("$d")
      done < <(compgen -A directory -- "$cur")
      ;;
    unload)
      # 仅补全包含 .clericum-mount 标记的挂载点目录
      compopt -o nospace -o filenames
      local d
      while IFS= read -r d; do
        [[ -f "$d/.clericum-mount" ]] && COMPREPLY+=("$d")
      done < <(compgen -A directory -- "$cur")
      ;;
    create)
      # 在指定目录创建 store：补全目录
      compopt -o nospace -o filenames
      COMPREPLY=( $(compgen -A directory -- "$cur") )
      ;;
    *)
      COMPREPLY=( $(compgen -W "create load unload" -- "$cur") )
      ;;
  esac
}

# backup: create <path> <name> | load <backup path> | remove <backup path>
reply_backup_impl() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local action="${COMP_WORDS[2]:-}"
  case "$action" in
    create|load|remove)
      # 这些动作后面接文件参数
      compopt -o nospace -o filenames
      COMPREPLY=( $(compgen -A file -- "$cur") )
      ;;
    *)
      COMPREPLY=( $(compgen -W "create load remove" -- "$cur") )
      ;;
  esac
}

reply_store=(reply_store_impl)
reply_backup=(reply_backup_impl)

subcmd_args_store="@store"
subcmd_args_backup="@backup"
