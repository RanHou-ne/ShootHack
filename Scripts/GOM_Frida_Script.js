// === GOM实战脚本：实时找到我方和所有敌人 ===
// Frida 脚本 - 使用 GameObject Manager 直接内存偏移遍历游戏对象
// 
// 📌 使用说明：
//   1. 将此脚本保存为 .js 文件
//   2. 运行游戏并注入 Frida：frida -U -f <Game.apk> -l GOM_Frida_Script.js
//   3. 在 Frida 控制台查看实时输出
//
// ⚠️ 重要：需要根据 dump.cs 确认以下偏移值！

const UNITY_BASE = Module.findBaseAddress('GameAssembly.dll');
if (!UNITY_BASE) {
    console.log('[错误] GameAssembly.dll 未找到！');
    throw new Error('Failed to find GameAssembly.dll');
}

// ============================================================================
// 🔴 【必填】你需要从 dump.cs 中找到的关键偏移
// ============================================================================
// 
// 【如何找 GOM_OFFSET】
//   1. 在 dump.cs 中搜索 "GameObjectManager" 或 "ActiveObject"
//   2. 查看其静态成员或 s_Instance 字段的偏移
//   3. 例如：private static GameObjectManager s_Instance; // 0xABCDEF00
//   4. 如果没有导出，使用 CheatEngine 或其他工具动态找到 GOM 的全局地址
//
// 【如何找 TAG_OFFSET】
//   1. 在 dump.cs 中找到 GameObject 类的定义
//   2. 查看 "string tag" 字段或属性的偏移
//   3. 通常在 public string tag { get; } 注释中有 // 0xXX
//
// 【如何找 TRANSFORM_OFFSET】
//   1. 在 dump.cs 中找到 GameObject 的 Transform 字段偏移
//   2. 查找 "Transform transform" 或 "public Transform transform"
//   3. 通常在注释中显示，例如 // 0x28
//
// 【如何找 POS_OFFSET】
//   1. 在 dump.cs 中找 Transform 类的定义
//   2. 查找 "Vector3 position" 字段的偏移
//   3. 通常在 "public Vector3 position { get; }" 的注释中
//   4. Transform 对象的 position 字段通常在 0x30

const GOM_OFFSET = 0xABCDEF00;          // ←←← 替换为实际的 GOM 偏移 (dump.cs 确认)
const ACTIVE_OFFSET = 0x10;             // activeObjects vector 起始（通常固定）
const TAG_OFFSET = 0x5C;                // GameObject Tag 字符串偏移（dump.cs 确认）
const TRANSFORM_OFFSET = 0x28;          // GameObject Transform 指针偏移（dump.cs 确认）
const POS_OFFSET = 0x30;                // Transform position.x 起始地址（dump.cs 确认）

// 🔴 【必填】获取到的 Update 函数偏移（用于 Hook 的入口点）
// 通常在 dump.cs 搜索 "public void Update()" 的 RVA 值
const UPDATE_OFFSET = 0x12345678;       // ←←← 替换为实际的 Update 函数偏移

// 【可选】游戏特定的血量字段偏移（根据 PlayerController 或 Enemy 脚本修改）
const PLAYER_HEALTH_OFFSET = 0xYYYY;    // 玩家血量在 GameObject 中的偏移
const ENEMY_HEALTH_OFFSET = 0xZZZZ;     // 敌人血量在 GameObject 中的偏移

// ============================================================================
// 辅助工具函数
// ============================================================================

/**
 * 安全读取 UTF-8 字符串（防止因坏指针导致的崩溃）
 */
function safeReadString(addr, maxLen = 256) {
    try {
        if (addr.isNull()) return '<null>';
        const s = Memory.readUtf8String(addr, maxLen);
        return s || '<empty>';
    } catch (e) {
        return '<error: ' + e.message + '>';
    }
}

/**
 * 安全读取浮点数
 */
function safeReadFloat(addr, defaultVal = 0.0) {
    try {
        return Memory.readFloat(addr);
    } catch (e) {
        console.log('[警告] 读取浮点数失败 at ' + addr + ': ' + e.message);
        return defaultVal;
    }
}

/**
 * 安全读取指针
 */
function safeReadPointer(addr) {
    try {
        return Memory.readPointer(addr);
    } catch (e) {
        return null;
    }
}

// ============================================================================
// 【核心】Hook GameObject Manager 的 Update 函数
// ============================================================================

console.log('[GOM] 脚本初始化...');
console.log('[GOM] UNITY_BASE = ' + UNITY_BASE);

Interceptor.attach(UNITY_BASE.add(UPDATE_OFFSET), {
    onEnter: function(args) {
        try {
            // 获取 GOM 实例地址
            const gom = UNITY_BASE.add(GOM_OFFSET);
            
            // 读取 activeObjects vector
            const activeList = Memory.readPointer(gom.add(ACTIVE_OFFSET));
            if (!activeList || activeList.isNull()) {
                console.log('[GOM] activeObjects 列表为空');
                return;
            }

            // 读取 vector 的大小（通常在 +0x18 偏移处）
            const count = Memory.readInt(activeList.add(0x18));

            let myPlayerCount = 0;
            let enemyList = [];
            let objectsByTag = {};

            // ========== 【遍历所有 GameObject】 ==========
            for (let i = 0; i < Math.min(count, 2000); i++) {     // 限制 2000 个防止卡顿
                // vector 的数据通常从 +0x20 开始，每个元素 8 字节指针
                const goPtr = Memory.readPointer(activeList.add(0x20 + i * 8));
                if (!goPtr || goPtr.isNull()) continue;

                // ===== 【步骤1】读取 Tag =====
                try {
                    const tagPtr = Memory.readPointer(goPtr.add(TAG_OFFSET));
                    const tag = safeReadString(tagPtr);

                    // 统计标签出现次数
                    objectsByTag[tag] = (objectsByTag[tag] || 0) + 1;

                    // ===== 【步骤2】按 Tag 分类处理 =====
                    if (tag === "Player") {
                        myPlayerCount++;

                        // 读取 Transform 组件
                        const transform = Memory.readPointer(goPtr.add(TRANSFORM_OFFSET));
                        if (!transform || transform.isNull()) continue;

                        // 读取位置（Vector3 = float[3]）
                        const x = safeReadFloat(transform.add(POS_OFFSET));
                        const y = safeReadFloat(transform.add(POS_OFFSET + 4));
                        const z = safeReadFloat(transform.add(POS_OFFSET + 8));

                        // 【可选】读取血量（需要根据游戏脚本修改偏移）
                        let healthStr = '';
                        if (PLAYER_HEALTH_OFFSET !== 0xYYYY) {
                            const health = safeReadFloat(goPtr.add(PLAYER_HEALTH_OFFSET));
                            healthStr = ' | 血量: ' + health.toFixed(1);
                        }

                        console.log(`[我方] ★ 位置: (${x.toFixed(2)}, ${y.toFixed(2)}, ${z.toFixed(2)})${healthStr}`);
                    }
                    else if (tag === "Enemy") {
                        // 读取 Transform 组件
                        const transform = Memory.readPointer(goPtr.add(TRANSFORM_OFFSET));
                        if (!transform || transform.isNull()) continue;

                        // 读取位置
                        const x = safeReadFloat(transform.add(POS_OFFSET));
                        const y = safeReadFloat(transform.add(POS_OFFSET + 4));
                        const z = safeReadFloat(transform.add(POS_OFFSET + 8));

                        // 【可选】读取血量
                        let healthStr = '';
                        if (ENEMY_HEALTH_OFFSET !== 0xZZZZ) {
                            const health = safeReadFloat(goPtr.add(ENEMY_HEALTH_OFFSET));
                            healthStr = ' | 血量: ' + health.toFixed(1);
                        }

                        const enemyInfo = `敌人 → (${x.toFixed(2)}, ${y.toFixed(2)}, ${z.toFixed(2)})${healthStr}`;
                        enemyList.push(enemyInfo);
                    }
                    else if (tag === "Obstacle" || tag === "Building") {
                        // 其他重要对象可按需处理
                        objectsByTag[tag] = (objectsByTag[tag] || 0);
                    }
                } catch (e) {
                    console.log('[错误] 处理 GameObject #' + i + ' 失败: ' + e.message);
                }
            }

            // ========== 【输出统计信息】 ==========
            console.log('\n[GOM实战] ═══════════════════════════════════════');
            console.log(`[GOM实战] 本帧统计 → 我方: ${myPlayerCount}个 | 敌人: ${enemyList.length}个`);
            console.log('[GOM实战] 对象分类统计:');
            for (const [tag, count] of Object.entries(objectsByTag)) {
                console.log(`  - ${tag}: ${count} 个`);
            }
            
            // 取消注释下面这行可打印所有敌人位置
            // enemyList.forEach(msg => console.log('  ' + msg));
            
            console.log('[GOM实战] ═══════════════════════════════════════\n');

        } catch (e) {
            console.log('[GOM] Hook 执行出错: ' + e.message);
            console.log('[GOM] Stack: ' + e.stack);
        }
    }
});

console.log('[GOM实战] 脚本已注入！每帧 Update 时会输出游戏对象信息。');
console.log('[GOM实战] 移动你的 Player 看看控制台实时输出！');

// ============================================================================
// 【高级】动态偏移查找辅助函数（如果手工无法确定偏移，可用此函数）
// ============================================================================

/**
 * 在指定范围内搜索指针签名
 * 示例：findSignature(UNITY_BASE, '\x48\x89\x5C\x24\x08', 1000)
 */
function findSignature(baseAddr, pattern, searchLen) {
    const scan = Memory.scanSync(baseAddr, searchLen, pattern);
    if (scan.length > 0) {
        return scan[0].address;
    }
    return null;
}

/**
 * 帮助用户使用 CheatEngine 找到 GOM_OFFSET 的提示
 */
function helpFindGOMOffset() {
    console.log('\n【如何使用 CheatEngine 找 GOM_OFFSET】');
    console.log('  1. 在 CheatEngine 中附加到游戏进程');
    console.log('  2. 在任意一个 Player GameObject 上读取其地址');
    console.log('  3. 根据 activeObjects vector 的结构反推 GOM 地址');
    console.log('  4. GOM_OFFSET = (GOM_Address - GameAssembly_BaseAddress)');
    console.log('  5. 确认后填入本脚本顶部的常数定义中\n');
}

// ============================================================================
// 【测试】快速验证偏移是否正确的方法
// ============================================================================

/**
 * 验证函数：如果输出看起来合理（坐标在合理范围、敌人个数符合预期），
 * 说明偏移配置正确；如果输出都是 NaN 或数字明显不合理，需要调整偏移。
 */
console.log('[GOM] 偏移验证: 首次运行时请观察输出坐标是否合理');
console.log('[GOM] - 合理的坐标范围通常是 [-1000, 1000]（根据游戏地图）');
console.log('[GOM] - 如果全是 NaN 或 Infinity，说明偏移需要调整');

