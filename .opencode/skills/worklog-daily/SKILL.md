---
name: worklog-daily
description: 自动化研发日志管理系统。整合 Git 提交与本地笔记，自动提取“硬件 Bug”与“性能参数”，实现本地 Markdown 与 Notion 云端数据库的双重同步。
---

# Worklog Daily (R&D Edition)

## 概览
这是一个闭环的研发日志管理工具，旨在减少嵌入式开发中的文档负担。它能将开发过程中的代码改动（Git）和随手笔记（Markdown）自动转化为结构化的技术文档，并采用**三层架构**智能同步至 Notion 云端知识库：

- **本地层**：自动生成日志文件 (`Worklog.md` 和 `daily/YYYY-MM-DD.md`)
- **汇总层**：在项目页面下创建"📋 工作日志汇总"子页面，记录每日开发简报
- **经验层**：在项目页面下创建"💡 技术经验库"数据库，重点沉淀问题解决方案和性能优化记录

## 核心工作流 (Workflow)

### 1. 数据采集 (Data Collection)
- **触发脚本**：运行 `~/.codex/skills/local/worklog-daily/scripts/collect_worklog_sources.py`。
- **Git 采集**：提取今日（或指定日期）的 Commit Messages 和变更文件列表。
- **笔记采集**：扫描 `log/*.md` 和 `README.md`，通过正则识别 `# M.D` 或 `## YYYY-MM-DD` 格式的内容块。
- **环境感知**：通过脚本输出获取当前 `project_name`（如 `rk3588_project`）。

### 2. 智能分类与总结 (Synthesis & AI Analysis)
Agent 必须对采集的数据进行语义分析，并提取出以下特定字段：

#### 2.1 基础字段（用于日志汇总）
- **项目名称 (Project)**：从上下文或脚本中提取。若用户未指定，默认使用文件夹名。
- **今日进展 (Progress)**：总结实现的功能、修复的代码逻辑（中文描述）。
- **分类标签 (Category)**：根据关键词自动映射为"硬件/算法/驱动/系统/环境"。

#### 2.2 经验库专用字段（触发条件：有问题或优化）
- **硬件 Bug (Hardware Bugs)**：**重点识别**
  - 检测关键词：驱动报错、内核崩溃（Panic）、段错误（Segfault）、电压异常、物理损坏、超时、死锁
  - 提取内容：错误码、报错日志片段、触发条件
  - 若无则填"无"（不写入经验库）
  
- **性能指标 (Parameters)**：**重点提取**
  - 识别所有包含数值和单位的内容：
    - 延迟类：`推理延迟: 45ms`、`响应时间: 120ms`
    - 吞吐类：`帧率: 30fps`、`QPS: 500`
    - 资源类：`内存占用: 400MB`、`CPU 使用率: 65%`
    - 硬件类：`电压: 1.1V`、`温度: 78°C`
  - **参数对比**：若同一指标出现多次，自动识别为优化记录（如：延迟从 120ms → 45ms）
  
- **根因分析 (Root Cause)**：**需深度挖掘**
  - 从 Git Commit Message 和笔记中提取技术原因
  - 识别关键词："原因是"、"由于"、"因为"、"发现是"
  - 优先提取：代码逻辑错误、配置不当、硬件限制、依赖版本冲突
  
- **解决方案 (Solution)**：**核心字段**
  - 提取修复步骤的关键动作：修改配置、调整参数、重构代码、更换硬件
  - 若包含代码片段，保留完整语法（带文件名和行号）
  - 识别关键词："修复方法"、"解决步骤"、"通过"、"改为"

### 3. 本地文档更新 (Local File Ops)
- **单日日志**：创建/更新 `daily/YYYY-MM-DD.md`。
  - **格式**：包含“摘要”、“详细技术路径（含 Git Hash）”、“参数记录”和“遗留问题”。
- **主日志索引**：在根目录 `Worklog.md` 中以列表形式追加今日简报，并保留指向单日日志的链接。

### 4. Notion 三层结构同步 (Hierarchical Sync)

调用 `notion` MCP 服务，构建项目 → 日志 + 经验库的三层架构：

#### 4.1 项目页面层 (Project Root)
- **页面搜索/创建**：
  - 在 Notion workspace 根目录搜索名为 `{{项目名称}}` 的页面（如 `RK3588 考勤系统`）
  - 若不存在，创建新页面，标题格式：`🚀 {{项目名称}}`
  - 在页面顶部添加项目概述区域，包含：项目启动日期、技术栈、核心目标

#### 4.2 日志汇总子页面 (Daily Log Summary)
- **位置**：作为项目页面的**第一个子页面**
- **页面标题**：`📋 工作日志汇总`
- **内容结构**：
  - 使用 **Callout 区块**显示每日简报
  - 每条日志条目格式：
    ```
    > 📅 YYYY-MM-DD | {{分类标签}}
    > **今日成果**：一句话总结（从 Progress 提取）
    > **关键指标**：若有参数变化，用徽章样式高亮显示
    > **遗留问题**：未解决的 Bug 数量 + 链接到经验库对应条目
    ```
  - **追加逻辑**：每次运行在页面**顶部**追加最新日志（保持时间倒序）
  - **去重检查**：若当前日期已存在，更新该日期对应的 Callout 内容

#### 4.3 经验库子页面 (Knowledge Base - 重点)
- **位置**：作为项目页面的**第二个子页面**
- **页面标题**：`💡 技术经验库 | {{项目名称}}`
- **内容组织**：创建一个 **Database（数据库视图）**，包含以下属性：
  
  | 属性名 | 类型 | 说明 |
  |--------|------|------|
  | `标题` | Title | 格式：`🔴 [问题] {{简短描述}}` 或 `🟢 [优化] {{简短描述}}` |
  | `日期` | Date | 问题发生/解决的日期 |
  | `分类` | Select | 硬件 / 算法 / 驱动 / 系统 / 环境 |
  | `优先级` | Select | 🔴 Critical / 🟡 High / 🟢 Normal |
  | `状态` | Status | 已解决 / 进行中 / 待排查 |
  | `关键参数` | Rich Text | 记录数值变化（如：优化前 120ms → 优化后 45ms） |
  | `根因分析` | Rich Text | 技术原因（如：DMA 缓冲区溢出） |
  | `解决方案` | Rich Text | **重点标记**：具体修复步骤 + 代码片段 |
  | `关联文件` | Files & Media | 上传相关日志截图、寄存器 dump 文件 |
  | `Git Hash` | Text | 关联的提交哈希（便于回溯） |

- **数据写入规则**：
  - **仅当满足以下任一条件时创建新条目**：
    1. 识别到 `Hardware Bug` 非空且非"无"
    2. 识别到关键参数有**显著变化**（数值差异 >20% 或状态从 Fail → Pass）
    3. 手动笔记中包含"解决方案"、"原因分析"、"踩坑"等关键词
  
  - **重点标记策略**：
    - 标题前缀使用 emoji：🔴（问题）、🟢（优化）、⚡（性能）
    - `解决方案` 字段使用 **Toggle Heading** 格式，便于折叠阅读
    - 若有代码片段，使用 **Code Block** 嵌入，并标注编程语言
    - 参数对比使用 **表格** 或 **双栏布局**（Before / After）

- **去重与更新逻辑**：
  - 通过 `日期 + 标题关键词` 判断是否为同一问题
  - 若已存在，在该条目的 `解决方案` 字段末尾追加"更新记录"
  - 若状态从"待排查"变为"已解决"，自动更新 `状态` 属性

#### 4.4 执行顺序（关键）
1. **先生成本地总日志** (`Worklog.md` 和 `daily/YYYY-MM-DD.md`)
2. **检索/创建项目页面**
3. **更新日志汇总子页面**（轻量级，每日必写）
4. **条件性写入经验库**（仅在有价值信息时触发）
5. **交叉链接**：在日志汇总中为遗留问题添加跳转链接到经验库对应条目

#### 4.5 Notion API 调用示例
```python
# 伪代码示例（实际使用 MCP notion 工具）

# 1. 搜索项目页面
project_page = notion.search(query=project_name, filter="page")
if not project_page:
    project_page = notion.create_page(title=f"🚀 {project_name}")

# 2. 查找/创建子页面
log_summary_page = find_child_page(project_page, "📋 工作日志汇总")
knowledge_base_page = find_child_page(project_page, "💡 技术经验库")

# 3. 更新日志汇总（追加模式）
notion.append_blocks(log_summary_page, callout_content)

# 4. 条件性写入经验库
if has_significant_issue_or_optimization:
    notion.create_database_entry(
        knowledge_base_page,
        properties={
            "标题": f"🔴 [问题] {issue_title}",
            "解决方案": solution_with_code,
            "关键参数": parameter_comparison
        }
    )
```

### 5. Git 版本控制与远程同步 (Version Control)

在完成本地文档生成和 Notion 同步后，必须执行 Git 提交和推送操作，确保工作日志版本化并同步至远程仓库。

#### 5.1 检查与暂存 (Stage Changes)
- **检查状态**：运行 `git status` 查看新生成的文件
- **暂存工作日志相关文件**：
  ```bash
  git add .opencode/skills/worklog-daily/
  git add daily/
  git add Worklog.md
  ```

#### 5.2 提交规范 (Commit Convention)
- **提交信息格式**：参考项目现有提交风格，采用清晰的多行描述
- **标准模板**：
  ```bash
  git commit -m "Add worklog-daily skill with automated daily log generation and Notion sync
  
  - Implement three-layer Notion architecture (project/daily logs/knowledge base)
  - Add automated log extraction from [project_names]
  - Generate structured daily logs with key parameters, bugs, and solutions
  - Create knowledge base entries for critical technical issues
  - Add Python scripts for log analysis and generation (UTF-8 encoding support)"
  ```

#### 5.3 推送到远程仓库 (Push to Remote)
- **执行推送**：
  ```bash
  git push origin main
  ```
  或根据实际分支名称调整（如 `master`、`develop`）

#### 5.4 执行顺序（完整闭环）
1. ✅ **数据采集** → 运行脚本收集 Git 提交和笔记
2. ✅ **智能分析** → AI 提取关键字段和技术细节
3. ✅ **本地生成** → 创建/更新 `daily/*.md` 和 `Worklog.md`
4. ✅ **Notion 同步** → 更新三层架构（项目页/日志汇总/经验库）
5. ✅ **Git 提交** → 暂存文件 + 规范化提交信息
6. ✅ **推送远程** → 同步至 GitHub/GitLab 等远程仓库

#### 5.5 自动化脚本示例（可选）
可在 `scripts/` 目录下创建 `commit_and_push.sh`（Linux/macOS）或 `commit_and_push.ps1`（Windows）：

```bash
#!/bin/bash
# scripts/commit_and_push.sh

# 暂存工作日志文件
git add .opencode/skills/worklog-daily/ daily/ Worklog.md

# 生成提交信息（包含日期和项目名称）
DATE=$(date +%Y-%m-%d)
PROJECT_NAME=$(basename $(pwd))

git commit -m "Update worklog for ${PROJECT_NAME} - ${DATE}

- Add daily log entries with technical details
- Sync critical issues to Notion knowledge base
- Update performance metrics and bug resolutions"

# 推送到远程仓库
git push origin main

echo "✅ Worklog committed and pushed successfully!"
```

#### 5.6 注意事项
- **推送前确认**：确保 Notion 同步已成功完成，避免不完整数据进入版本历史
- **分支策略**：若项目采用 feature 分支，应推送至对应分支而非直接推送 main
- **冲突处理**：若出现推送冲突，先执行 `git pull --rebase` 解决冲突后再推送
- **敏感信息**：确保工作日志中不包含 API 密钥、密码等敏感信息

## 编码与终端注意事项 (Encoding Guardrails)
- **必须使用 UTF-8 写入文件**：本地日志与 `Worklog.md` 统一使用 UTF-8。优先使用 `Set-Content -Encoding UTF8` 或在 Python 中显式 `encoding='utf-8'`。
- **避免在命令行内联中文生成内容**：PowerShell/管道/终端编码不一致时会把中文变成 `?`。如果需要生成大量中文，先写一个 UTF-8 的 `.py` 脚本文件（存放在 `scripts/`），再运行该脚本。
- **若必须在命令行内联中文**：先执行 `chcp 65001`，并设置 `$OutputEncoding = [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)`；同时设置环境变量 `PYTHONUTF8=1`。

## 提示词指令 (Prompt Constraints)


1. **项目自适应**：Agent 需根据当前环境判断项目属性。若是 RK3588，重点关注 NPU、交叉编译、SDK 相关术语；若是纯软件，侧重逻辑描述。

2. **专业术语**：强制使用专业中文（如：模型量化、算子融合、寄存器偏移、中断处理）。

3. **数据敏感度**：
   - 任何带单位的数值必须出现在 `Parameters` 属性中。
   - 任何描述故障的词汇（如 `fail`, `error`, `溢出`, `挂死`）必须出现在 `Hardware Bug` 属性中。

4. **经验库写入阈值**：**不是所有日志都需要写入经验库**。仅在以下情况触发：
   - 识别到明确的错误/异常/崩溃（有错误码或堆栈信息）
   - 性能参数有显著变化（数值差异 >20% 或状态改变）
   - 笔记中明确包含"解决方案"、"踩坑"、"经验"等关键词
   - 涉及硬件调试、底层驱动、系统配置等高价值技术细节

5. **重点标记规则**：
   - 标题必须使用 emoji 前缀：🔴（问题/Bug）、🟢（优化/改进）、⚡（性能提升）
   - 解决方案字段必须结构化：问题现象 → 排查过程 → 解决步骤 → 验证结果
   - 参数对比必须使用表格或明确的前后对比格式

6. **Git 版本控制要求**：
   - 每次完成工作日志生成和 Notion 同步后，必须执行 Git 提交和推送
   - 提交信息应清晰描述本次日志更新的内容和关键技术点
   - 确保远程仓库与本地工作日志保持同步

7. **无活动处理**：若今日无 Git 提交且无笔记记录，本地文档记录为"今日无开发活动"，不触发 Notion 写入和 Git 提交。

## 资源与工具要求

### 必须运行的 MCP 服务器
- `filesystem`: 读写本地 `daily/` 和 `Worklog.md`。
- `notion`: 负责云端同步（需确保 `codex mcp login notion` 状态为 Logged in）。

### 必要脚本
- `scripts/collect_worklog_sources.py`: 负责 Git 提取与 Markdown 笔记的正则清洗。
- `scripts/render_worklog_sources.py`: (可选) 用于生成原始数据的摘要预览。
