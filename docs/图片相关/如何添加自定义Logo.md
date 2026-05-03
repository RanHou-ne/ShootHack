# 如何添加自定义Logo到项目中

本教程将教你如何将自己的logo图片嵌入到C++代码中，实现无需外部文件的logo显示。

## 方法概述

项目使用了**图片嵌入技术**，将图片数据以字节数组的形式直接编译到DLL中。这样做的好处：
- ✅ 不需要额外的图片文件
- ✅ 防止文件丢失或路径错误
- ✅ 更安全，不易被修改
- ✅ 加载速度更快

## 步骤一：准备你的Logo图片

### 1. 图片要求
- **格式**: PNG（推荐）、JPG、BMP
- **尺寸**: 
  - 主Logo: 192x192 像素（或任意正方形尺寸）
  - 小图标: 32x32 或 64x64 像素
- **背景**: PNG格式支持透明背景（推荐）

### 2. 创建Logo（可选）

如果你想快速生成一个简单的logo，可以使用项目中的Python脚本：

```bash
python create_logo.py
```

这会生成一个基础的logo.png文件。

## 步骤二：将图片转换为C++数组

### 方法A：使用Python脚本（推荐）

1. 使用项目提供的转换脚本：

```bash
python image_to_cpp.py your_logo.png my_logo
```

参数说明：
- `your_logo.png`: 你的图片文件路径
- `my_logo`: 生成的数组名称（可选，默认使用文件名）

2. 脚本会生成一个 `your_logo_array.h` 文件，包含类似这样的代码：

```cpp
unsigned char my_logo[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    // ... 更多字节数据
};
unsigned int my_logo_size = 12345;
```

### 方法B：使用在线工具

1. 访问在线转换工具（例如：https://notisrac.github.io/FileToCArray/）
2. 上传你的图片文件
3. 选择输出格式为 C/C++ Array
4. 复制生成的代码

### 方法C：使用xxd命令（Linux/Mac）

```bash
xxd -i your_logo.png > logo_array.h
```

## 步骤三：添加到项目中

### 1. 编辑 `src/Assets/image.h`

打开 `src/Assets/image.h` 文件，在文件末尾添加你的数组：

```cpp
// 你的自定义Logo
unsigned char my_custom_logo[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    // ... 粘贴转换后的字节数据
};
```

### 2. 在 `src/Assets/image.h` 中声明纹理指针

找到 `namespace image` 部分（通常在 `DX11Hook.cpp` 中），添加你的纹理指针：

```cpp
namespace image
{
    ID3D11ShaderResourceView* bg           = nullptr;
    ID3D11ShaderResourceView* logo         = nullptr;
    ID3D11ShaderResourceView* my_custom_logo = nullptr;  // 添加这一行
    // ... 其他纹理
}
```

### 3. 在 `src/Hook/DX11Hook.cpp` 中加载纹理

找到纹理加载部分（通常在 `InitImGui` 函数中），添加加载代码：

```cpp
// 加载你的自定义logo
#ifdef USE_EMBEDDED_IMAGES
bool myLogoLoaded = LoadTextureFromMemory(
    g_pd3dDevice, 
    my_custom_logo,           // 数组名
    sizeof(my_custom_logo),   // 数组大小
    &image::my_custom_logo    // 输出纹理指针
);
if (myLogoLoaded)
{
    printf("[+] 自定义Logo从内嵌数据加载成功\n");
}
#endif

// 如果内嵌加载失败，尝试从文件加载（可选）
if (!myLogoLoaded)
{
    LoadTextureFromFileWIC(g_pd3dDevice, L".\\my_logo.png", &image::my_custom_logo);
}
```

## 步骤四：在UI中使用Logo

在 `src/UI/Menu/Menu.cpp` 中使用你的logo：

```cpp
// 绘制自定义logo
if (image::my_custom_logo)
{
    ImVec2 logoPos = pos + ImVec2(100, 100);  // 设置位置
    ImVec2 logoSize = ImVec2(64, 64);         // 设置大小
    
    ImGui::GetWindowDrawList()->AddImage(
        image::my_custom_logo,                // 纹理
        logoPos,                              // 左上角位置
        logoPos + logoSize,                   // 右下角位置
        ImVec2(0, 0),                         // UV 左上角
        ImVec2(1, 1),                         // UV 右下角
        ImColor(255, 255, 255, 255)           // 颜色（白色=原色）
    );
}
else
{
    // Logo加载失败时的降级方案
    ImVec2 logoCenter = pos + ImVec2(132, 132);
    ImGui::GetWindowDrawList()->AddCircleFilled(
        logoCenter, 
        32.0f, 
        ImColor(71, 226, 67, 255), 
        32
    );
}
```

## 步骤五：编译和测试

1. 确保 `USE_EMBEDDED_IMAGES` 宏已定义（在 `DX11Hook.cpp` 顶部）：

```cpp
#define USE_EMBEDDED_IMAGES
```

2. 编译项目：

```bash
# 使用VSCode任务
Ctrl+Shift+B

# 或使用PowerShell
.\.vscode\Invoke-MSBuild.ps1 -Target Build -Configuration Debug -Platform x64
```

3. 测试DLL，检查logo是否正确显示

## 高级技巧

### 1. 添加多个Logo

你可以为不同的场景准备多个logo：

```cpp
unsigned char logo_main[] = { /* ... */ };      // 主Logo
unsigned char logo_small[] = { /* ... */ };     // 小图标
unsigned char logo_loading[] = { /* ... */ };   // 加载动画
```

### 2. 动态切换Logo

```cpp
static int currentLogo = 0;
ID3D11ShaderResourceView* logos[] = {
    image::logo_main,
    image::logo_small,
    image::logo_loading
};

// 使用当前logo
if (logos[currentLogo])
{
    ImGui::GetWindowDrawList()->AddImage(logos[currentLogo], ...);
}
```

### 3. Logo着色

通过修改颜色参数来改变logo颜色：

```cpp
// 原色（白色）
ImColor(255, 255, 255, 255)

// 红色调
ImColor(255, 100, 100, 255)

// 半透明
ImColor(255, 255, 255, 128)

// 根据状态动态着色
ImColor color = isActive ? ImColor(0, 255, 0, 255) : ImColor(128, 128, 128, 255);
```

### 4. Logo动画

```cpp
static float logoAlpha = 0.0f;
logoAlpha += ImGui::GetIO().DeltaTime * 2.0f;  // 淡入速度
if (logoAlpha > 1.0f) logoAlpha = 1.0f;

ImColor(255, 255, 255, (int)(logoAlpha * 255))
```

## 常见问题

### Q: 转换后的数组太大，编译器报错？
A: 
1. 压缩图片尺寸（例如从512x512降到192x192）
2. 使用PNG格式并优化压缩
3. 将数组放在单独的.cpp文件中

### Q: Logo显示不出来？
A: 检查以下几点：
1. 确保 `USE_EMBEDDED_IMAGES` 宏已定义
2. 检查数组名称是否匹配
3. 查看控制台输出，确认加载成功
4. 确保纹理指针不为nullptr

### Q: Logo颜色不对？
A: 
1. 检查PNG是否使用了正确的颜色模式（RGBA）
2. 确保AddImage的颜色参数是白色 `ImColor(255, 255, 255, 255)`
3. 检查图片本身的颜色是否正确

### Q: 如何优化大小？
A: 
1. 使用PNG格式（比BMP小很多）
2. 减少图片尺寸
3. 使用在线PNG压缩工具（如TinyPNG）
4. 考虑使用JPG格式（但不支持透明）

## 示例：完整流程

假设你有一个 `my_awesome_logo.png` 文件：

```bash
# 1. 转换为C++数组
python image_to_cpp.py my_awesome_logo.png awesome_logo

# 2. 打开生成的 my_awesome_logo_array.h，复制内容

# 3. 粘贴到 src/Assets/image.h 末尾

# 4. 在 DX11Hook.cpp 中添加：
namespace image {
    ID3D11ShaderResourceView* awesome_logo = nullptr;
}

# 5. 在 InitImGui 中加载：
LoadTextureFromMemory(g_pd3dDevice, awesome_logo, sizeof(awesome_logo), &image::awesome_logo);

# 6. 在 Menu.cpp 中使用：
ImGui::GetWindowDrawList()->AddImage(image::awesome_logo, pos, pos + ImVec2(64, 64), ...);

# 7. 编译测试
```

## 参考资源

- 项目现有的 `src/Assets/image.h` - 查看现有logo的格式
- `src/Hook/DX11Hook.cpp` - 查看纹理加载代码
- `EMBEDDED_IMAGES_GUIDE.md` - 嵌入图片的详细说明
- `LOGO_FIX_GUIDE.md` - Logo问题排查指南

---

**提示**: 如果你只是想快速测试，可以直接使用项目中已有的 `logo[]` 数组，它已经包含了一个可用的logo图片数据。
