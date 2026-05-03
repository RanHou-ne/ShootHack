#!/usr/bin/env python3
"""
将图片文件转换为C++数组格式
用法: python image_to_cpp.py your_logo.png
"""

import sys
import os

def image_to_cpp_array(image_path, array_name=None):
    """
    将图片文件转换为C++数组格式
    
    参数:
        image_path: 图片文件路径
        array_name: 数组名称（默认使用文件名）
    """
    if not os.path.exists(image_path):
        print(f"错误: 文件不存在: {image_path}")
        return None
    
    # 如果没有指定数组名，使用文件名（去掉扩展名）
    if array_name is None:
        array_name = os.path.splitext(os.path.basename(image_path))[0]
    
    # 读取文件内容
    with open(image_path, 'rb') as f:
        data = f.read()
    
    # 生成C++数组代码
    cpp_code = f"unsigned char {array_name}[] = {{"
    
    # 每行16个字节
    bytes_per_line = 16
    for i, byte in enumerate(data):
        if i % bytes_per_line == 0:
            cpp_code += "\n    "
        cpp_code += f"0x{byte:02X}"
        if i < len(data) - 1:
            cpp_code += ", "
    
    cpp_code += "\n};\n"
    
    # 添加大小常量
    cpp_code += f"unsigned int {array_name}_size = {len(data)};\n"
    
    return cpp_code

def main():
    if len(sys.argv) < 2:
        print("用法: python image_to_cpp.py <图片文件路径> [数组名称]")
        print("\n示例:")
        print("  python image_to_cpp.py my_logo.png")
        print("  python image_to_cpp.py my_logo.png custom_logo")
        print("\n支持的格式: PNG, JPG, BMP, GIF 等")
        return
    
    image_path = sys.argv[1]
    array_name = sys.argv[2] if len(sys.argv) > 2 else None
    
    print(f"正在转换: {image_path}")
    
    cpp_code = image_to_cpp_array(image_path, array_name)
    
    if cpp_code:
        # 输出到控制台
        print("\n" + "="*60)
        print("C++ 数组代码:")
        print("="*60)
        print(cpp_code)
        
        # 保存到文件
        output_file = os.path.splitext(image_path)[0] + "_array.h"
        with open(output_file, 'w') as f:
            f.write("// 自动生成的图片数组\n")
            f.write(f"// 源文件: {image_path}\n\n")
            f.write(cpp_code)
        
        print(f"\n✓ 已保存到: {output_file}")
        print(f"\n使用方法:")
        print(f"  1. 将生成的代码复制到 src/Assets/image.h 中")
        print(f"  2. 在 DX11Hook.cpp 中使用:")
        print(f"     LoadTextureFromMemory(g_pd3dDevice, {array_name or 'your_array'}, sizeof({array_name or 'your_array'}), &image::your_texture);")

if __name__ == "__main__":
    main()
