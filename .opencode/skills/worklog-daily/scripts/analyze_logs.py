#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
分析日志文件并提取结构化信息
"""

import re
from datetime import datetime
from pathlib import Path
import json

def parse_git_log(git_log_text):
    """解析Git日志"""
    commits = []
    lines = git_log_text.strip().split('\n')
    
    current_commit = None
    for line in lines:
        if '|' in line and len(line.split('|')) >= 4:
            # 新的提交记录
            parts = line.split('|')
            if current_commit:
                commits.append(current_commit)
            
            current_commit = {
                'hash': parts[0],
                'date': parts[1].split()[0],  # 只取日期部分
                'author': parts[2],
                'message': parts[3],
                'files': []
            }
        elif current_commit and line.strip():
            # 文件列表
            current_commit['files'].append(line.strip())
    
    if current_commit:
        commits.append(current_commit)
    
    return commits

def extract_performance_params(text):
    """提取性能参数"""
    params = []
    
    # 匹配FPS
    fps_pattern = r'FPS[:\s]*([0-9.]+)\s*fps?|帧率[:\s]*([0-9.]+)'
    for match in re.finditer(fps_pattern, text, re.IGNORECASE):
        value = match.group(1) or match.group(2)
        params.append(f"FPS: {value}")
    
    # 匹配时间(ms, s)
    time_pattern = r'(\d+\.?\d*)\s*(ms|s|秒|毫秒)'
    for match in re.finditer(time_pattern, text):
        params.append(f"耗时: {match.group(1)}{match.group(2)}")
    
    # 匹配分辨率
    resolution_pattern = r'(\d+)\s*[x×]\s*(\d+)'
    for match in re.finditer(resolution_pattern, text):
        params.append(f"分辨率: {match.group(1)}×{match.group(2)}")
    
    # 匹配内存/文件大小
    size_pattern = r'(\d+\.?\d*)\s*(MB|GB|KB)'
    for match in re.finditer(size_pattern, text, re.IGNORECASE):
        params.append(f"大小: {match.group(1)}{match.group(2)}")
    
    return list(set(params))  # 去重

def extract_bugs_and_issues(text):
    """提取Bug和问题"""
    issues = []
    
    # 关键词匹配
    bug_keywords = [
        '问题', 'Bug', 'bug', '错误', '失败', 'error', 'Error', 
        '崩溃', 'crash', '异常', '冲突', 'fail', 'failed'
    ]
    
    lines = text.split('\n')
    for i, line in enumerate(lines):
        if any(keyword in line for keyword in bug_keywords):
            # 收集上下文
            context_start = max(0, i - 1)
            context_end = min(len(lines), i + 3)
            context = '\n'.join(lines[context_start:context_end])
            issues.append(context.strip())
    
    return issues

def extract_solutions(text):
    """提取解决方案"""
    solutions = []
    
    solution_keywords = ['解决', '修复', '改为', '通过', '方案', '方法']
    
    lines = text.split('\n')
    for i, line in enumerate(lines):
        if any(keyword in line for keyword in solution_keywords):
            context_start = max(0, i - 1)
            context_end = min(len(lines), i + 4)
            context = '\n'.join(lines[context_start:context_end])
            solutions.append(context.strip())
    
    return solutions

def analyze_capture_log(log_path):
    """分析 capture 日志"""
    with open(log_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    return {
        'project': 'RDK双摄像头实时显示',
        'category': '硬件',
        'date': '2026-02-01',  # 估算日期
        'progress': [
            'VIO摄像头配置与图像采集(1920×1080)',
            'OpenCV集成与SSH X11转发实时显示',
            'VPS硬件编码尝试(失败,改用Sensor降分辨率)',
            '双摄像头640×480实时显示实现'
        ],
        'parameters': extract_performance_params(content),
        'issues': extract_bugs_and_issues(content),
        'solutions': extract_solutions(content)
    }

def analyze_msnet_log(log_path):
    """分析 msnet 日志"""
    with open(log_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    return {
        'project': 'MSNet立体视觉深度估计',
        'category': '算法',
        'date_range': '2026-01-11 ~ 2026-01-26',
        'progress': [
            'Windows ↔ ARM NFS双向共享配置',
            'BPU模型加载流程实现(LoadModel→Inference→Postprocess)',
            '量化校准数据冲突问题定位与修复',
            '成功实现板载推理(640×352官方模型, 384×192自定义模型)'
        ],
        'parameters': extract_performance_params(content),
        'issues': extract_bugs_and_issues(content),
        'solutions': extract_solutions(content),
        'critical_findings': [
            '校准数据格式冲突(三重致命冲突):数值量程/内存排布/颜色通道',
            'NFS文件传输中断导致模型损坏(33.75MB异常,正常304MB)',
            'Swap内存占满导致NTP服务崩溃'
        ]
    }

def main():
    # 分析两个日志
    capture_result = analyze_capture_log(
        Path(r'E:\embedded AI\RDK_board_pro\capture\log\README.md')
    )
    
    msnet_result = analyze_msnet_log(
        Path(r'E:\embedded AI\RDK_board_pro\msnet\README.md')
    )
    
    # 输出JSON格式便于后续处理
    results = {
        'capture': capture_result,
        'msnet': msnet_result
    }
    
    output_path = Path(r'E:\embedded AI\RDK_board_pro\.opencode\skills\worklog-daily\scripts\analysis_result.json')
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(results, f, ensure_ascii=False, indent=2)
    
    print(f"✅ 分析完成! 结果保存至: {output_path}")
    print(f"\n📊 发现:")
    print(f"  - Capture项目: {len(capture_result['progress'])}项进展, {len(capture_result['parameters'])}个参数")
    print(f"  - MSNet项目: {len(msnet_result['progress'])}项进展, {len(msnet_result['parameters'])}个参数")

if __name__ == '__main__':
    main()
