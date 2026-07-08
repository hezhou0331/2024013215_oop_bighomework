#!/usr/bin/env python3
import sys

def fix_long_lines(filepath, max_len=80):
    """Fix lines longer than max_len by splitting inline comments"""
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    fixed_lines = []
    for line in lines:
        # Remove trailing newline for processing
        content = line.rstrip('\n\r')
        
        # Skip if already short
        if len(content) <= max_len:
            fixed_lines.append(line)
            continue
        
        # Check if this is a comment-only line
        stripped = content.lstrip()
        if stripped.startswith('//'):
            # Comment line - try to split intelligently
            if '【' in content:
                # Chinese comment - can't split easily, keep as is
                fixed_lines.append(line)
            else:
                # Try to split
                fixed_lines.append(line)
            continue
        
        # Check if there's an inline comment
        if '//' in content:
            # Find the code and comment parts
            parts = content.split('//', 1)
            code_part = parts[0].rstrip()
            comment_part = '//' + parts[1]
            
            if len(code_part) <= max_len:
                # Code part is OK, just move comment to next line
                indent = len(content) - len(content.lstrip())
                fixed_lines.append(code_part + '\n')
                fixed_lines.append(' ' * indent + comment_part + '\n')
            else:
                # Code part itself is too long - keep as is
                fixed_lines.append(line)
        else:
            # No inline comment, keep as is
            fixed_lines.append(line)
    
    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        f.writelines(fixed_lines)
    
    print(f"Fixed {filepath}")

if __name__ == '__main__':
    files = [
        'src/controller/ProjectController.cpp',
        'src/view/ConsoleUI.cpp',
    ]
    for f in files:
        fix_long_lines(f)
