# 自定义Logo快速入门

## 🚀 5分钟添加你的Logo

### 方法一：使用现有图片（最简单）

如果你已经有一个logo图片（PNG/JPG）：

```bash
# 1. 转换图片为C++数组
python image_to_cpp.py your_logo.png my_logo

# 2. 打开生成的 your_logo_array.h，复制内容到 src/Assets/image.h

# 3. 完成！重新编译即可
```

### 方法二：生成带文字的Logo（推荐）

如果你想快速生成一个带文字的logo：

```bash
# 运行脚本（交互式）
python create_custom_logo.py

# 或直接指定参数
python create_custom_logo.py "我的" my_logo 192
```

脚本会询问你：
1. **Logo文字**: 输入1-3个字符（支持中文），例如 "SH"、"我的"
2. **数组名称**: 例如 "my_logo"（用于C++代码中）
3. **图片尺寸**: 例如 192（生成192x192的图片）

然后自动生成：
- ✅ `my_logo.png` - 可预览的图片文件
- ✅ `my_logo_array.h` - 可直接使用的C++代码

### 方法三：在线转换

1. 访问 https://notisrac.github.io/FileToCArray/
2. 上传你的logo图片
3. 复制生成的C++数组代码
4. 粘贴到 `src/Assets/image.h`

## 📝 完整示例

假设你想添加一个显示"SH"的logo：

### 步骤1：生成Logo

```bash
python create_custom_logo.py SH sh_logo 128
```

输出：
```
✓ Logo图片已保存到: sh_logo.png
✓ C++数组已保存到: sh_logo_array.h
```

### 步骤2：添加到项目

打开 `sh_logo_array.h`，内容类似：

```cpp
// 自动生成的Logo数组
// 文字: SH
// 尺寸: 128x128

unsigned char sh_logo[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    // ... 更多数据
};
unsigned int sh_logo_size = 5432;
```

复制这段代码，粘贴到 `src/Assets/image.h` 文件末尾。

### 步骤3：声明纹理指针

在 `src/Hook/DX11Hook.cpp` 中找到 `namespace image` 部分，添加：

```cpp
namespace image
{
    ID3D11ShaderResourceView* bg           = nullptr;
    ID3D11ShaderResourceView* logo         = nullptr;
    ID3D11ShaderResourceView* sh_logo      = nullptr;  // 👈 添加这一行
    // ...
}
```

### 步骤4：加载纹理

在 `DX11Hook.cpp` 的 `InitImGui` 函数中，找到logo加载部分，添加：

```cpp
// 加载自定义logo
#ifdef USE_EMBEDDED_IMAGES
bool shLogoLoaded = LoadTextureFromMemory(
    g_pd3dDevice, 
    sh_logo, 
    sizeof(sh_logo), 
    &image::sh_logo
);
if (shLogoLoaded)
{
    printf("[+] SH Logo加载成功\n");
}
#endif
```

### 步骤5：在UI中使用

在 `src/UI/Menu/Menu.cpp` 中使用你的logo：

```cpp
// 替换原来的logo绘制代码
if (image::sh_logo)
{
    ImGui::GetWindowDrawList()->AddImage(
        image::sh_logo,
        pos + ImVec2(14, 14),
        pos + ImVec2(78, 78),  // 64x64大小
        ImVec2(0, 0),
        ImVec2(1, 1),
        ImColor(255, 255, 255, 255)
    );
}
```

### 步骤6：编译测试

```bash
# 使用VSCode任务
Ctrl+Shift+B

# 或使用PowerShell
.\.vscode\Invoke-MSBuild.ps1 -Target Build -Configuration Debug -Platform x64
```

## 🎨 自定义颜色

编辑 `create_custom_logo.py` 中的颜色设置：

```python
# 找到这一行（约第35行）
fill=(71, 226, 67, 255)  # 绿色

# 修改为你喜欢的颜色：
fill=(255, 100, 100, 255)  # 红色
fill=(100, 150, 255, 255)  # 蓝色
fill=(255, 200, 50, 255)   # 橙色
```

颜色格式：`(R, G, B, Alpha)`，每个值范围 0-255

## 🔧 常见问题

### Q: 生成的图片太大，编译很慢？
**A**: 减小尺寸，例如使用 64 或 128 而不是 192

### Q: 中文显示为方块？
**A**: 确保系统安装了中文字体（Windows通常自带微软雅黑）

### Q: Logo不显示？
**A**: 检查：
1. 是否定义了 `USE_EMBEDDED_IMAGES` 宏
2. 数组名称是否匹配
3. 查看控制台输出确认加载成功

### Q: 想要透明背景？
**A**: 使用PNG格式，脚本默认生成透明背景

## 📚 更多资源

- **详细教程**: `docs/如何添加自定义Logo.md`
- **问题排查**: `LOGO_FIX_GUIDE.md`
- **嵌入图片说明**: `EMBEDDED_IMAGES_GUIDE.md`

## 💡 提示

1. **Logo尺寸建议**:
   - 主Logo: 128x128 或 192x192
   - 小图标: 32x32 或 64x64
   - 按钮图标: 24x24 或 32x32

2. **文字建议**:
   - 1-2个字符最清晰
   - 使用大写字母效果更好
   - 中文建议使用简短的词

3. **性能优化**:
   - 使用PNG格式（比BMP小很多）
   - 尺寸不要超过256x256
   - 使用在线工具压缩PNG

4. **多个Logo**:
   - 可以添加多个不同的logo
   - 根据场景切换使用
   - 例如：主Logo、加载Logo、小图标等

---

**快速开始**: 运行 `python create_custom_logo.py` 并按提示操作！
