# Notepatch-CN（Notepatch 汉化增强版）

基于开源项目 [Notepatch](https://github.com/59de44955ebd/Notepatch) v0.3 修改的 Windows 记事本替代品，保留原生记事本的极简体验，完成了**全界面中文汉化**，并补齐/增强了原生记事本的快捷键。

![深色主题](screenshots/notepatch-dark.png)

## 功能特性

- 全中文界面：菜单、对话框、状态栏、提示信息全部汉化（EOL 与编码名称保留英文，与中文版 Windows 自带记事本显示习惯一致）
- 补齐原生记事本快捷键：
  - **缩放**：Ctrl + 滚轮 / Ctrl + "+" / Ctrl + "-" / Ctrl + 0 重置（10%–500% 限幅）
  - **横向滚动**：Shift + 滚轮 或 Ctrl + Shift + 滚轮
  - **F5** 插入时间日期、**Ctrl + A** 全选、**Esc** 退出全屏
- 原项目已有功能完整保留：行号、深色/浅色主题、窗口置顶、半透明、查找替换（支持 `^t` / `^n` 占位符）等
- 界面设置持久化：主题、半透明、窗口置顶、缩放比例自动保存，下次启动自动恢复

## 快捷键一览

| 类别 | 快捷键 |
| --- | --- |
| 缩放 | Ctrl+滚轮 / Ctrl+"+" / Ctrl+"-" / Ctrl+0 重置 |
| 横向滚动 | Shift+滚轮 或 Ctrl+Shift+滚轮 |
| 全屏 | F11 进入，Esc 或 F11 退出 |
| 编辑 | Ctrl+A 全选、F5 插入时间日期 |
| 查找 | Ctrl+F 查找、Ctrl+H 替换、Ctrl+G 转到行、F3 / Shift+F3 下一个/上一个 |
| 文件 | Ctrl+N 新建、Ctrl+O 打开、Ctrl+S 保存、Ctrl+Shift+S 另存为、Ctrl+Shift+N 新窗口、Ctrl+P 打印、Ctrl+W 关闭 |
| 视图 | Alt+N 行号、Alt+T 半透明、Alt+A 窗口置顶、Alt+D 深色主题、Alt+L 浅色主题 |

## 安装

1. 在 [Releases](../../releases) 页面下载最新安装包并解压（**Windows 11 用户请下载 `-win11` 后缀的安装包**：Win11 的商店版新记事本会拦截 notepad.exe 启动，该版本的安装脚本会自动处理）
2. 右键 `安装.bat`，选择「以管理员身份运行」
3. 程序将复制到 `C:\Program Files\Notepatch`，并通过注册表 Image File Execution Options 重定向替换系统记事本（不修改系统文件 notepad.exe，随时可逆）

卸载 / 恢复系统记事本：右键 `卸载.bat`，以管理员身份运行即可。

## 自行编译

- Visual Studio 2019（v142 工具集）+ Windows 10 SDK（10.0.19041.0）
- 打开 `Notepatch.sln`，选择 Release / x64 编译即可

## 更新日志

见 [CHANGELOG.md](CHANGELOG.md)。

## 致谢

原始项目：[59de44955ebd/Notepatch](https://github.com/59de44955ebd/Notepatch)，感谢原作者的出色工作。
