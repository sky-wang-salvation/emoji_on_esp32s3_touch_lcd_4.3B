# RDK__X5 Session Handoff

## 1. 项目目标

这个工程当前的目标是：

- 在 ESP32-S3 + 4.3 寸触摸屏上显示一个机器人脸部界面。
- 使用 LVGL 绘制表情。
- 使用 GT911 触摸控制器响应点击/长按。
- 通过 `robot_link -> emotion_engine -> display_service` 这条链路驱动最终表情。
- 参考 `emoji/5.html` 中的 8 个表情设计，完成触摸交互和自动行为。

当前已经实现的核心方向是：先让触摸和基础显示稳定，再把图片参考中的 8 个表情行为映射到显示服务中。

## 2. 当前工作区结构

项目主要目录如下：

- `main/`
  - `app_main.c`
- `components/bsp/`
  - 板级初始化
- `components/display/`
  - `display_service.c`
  - `include/display_service.h`
- `components/emotion/`
  - `emotion_engine.c`
  - `include/emotion_engine.h`
- `components/robot_link/`
  - `robot_link.c`
  - `include/robot_link.h`
- `emoji/`
  - `5.html`，本轮表情设计参考文件

另外，工作区内还包含大量第三方示例和库目录，例如：

- `ESP32-S3-Touch-LCD-4.3B-BOX-Demo/`
- `managed_components/`

这些目录主要作为参考、依赖或示例，不是这轮主要改动点。

## 3. 系统运行链路

系统主循环在 `main/app_main.c` 中，启动顺序如下：

1. `bsp_board_init()`
2. `display_service_init()`
3. `emotion_engine_init()`
4. `robot_link_init()`
5. 主循环中每 50ms 执行一次：
   - `robot_link_poll()`
   - `emotion_engine_request_state()`
   - `emotion_engine_update()`
   - `display_service_render()`

对应文件：

- `main/app_main.c`
- `components/robot_link/robot_link.c`
- `components/emotion/emotion_engine.c`
- `components/display/display_service.c`

## 4. 本轮对话前已完成的修复

### 4.1 GT911 触摸不响应

问题现象：

- 屏幕触摸没有反应。
- 初始化日志出现 `ESP_ERR_INVALID_ARG`。

定位结果：

- GT911 的 I2C panel IO 配置里使用了不兼容的 `scl_speed_hz` 赋值，导致初始化失败。

修复方式：

- 在 `components/display/display_service.c` 的 `display_service_touch_init()` 中，保留 `ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG()` 的默认结构，不再手动覆盖 `scl_speed_hz`。

结果：

- 触摸初始化路径恢复正常。

### 4.2 表情偶发变红、抖动

问题现象：

- 脸部会周期性变成红色错误脸。
- 用户观察到“有时候表情变红，会抖动”。

定位结果：

- `robot_link_poll()` 之前返回了轮换状态，其中周期性会触发 `ROBOT_STATE_ERROR`。

修复方式：

- 将 `components/robot_link/robot_link.c` 中的 `robot_link_poll()` 简化为稳定返回 `ROBOT_STATE_IDLE`。

结果：

- 取消了周期性错误状态注入。
- 显示不再无故进入红脸错误态。

## 5. 本轮新增实现

本轮核心改动集中在：

- `components/display/display_service.c`

目标是把 `emoji/5.html` 中的 8 个表情逻辑映射到固件里。

### 5.1 已接入的表情状态

当前显示层已加入这些状态：

- `DISPLAY_FACE_HAPPY`
- `DISPLAY_FACE_SHY`
- `DISPLAY_FACE_EXCITED`
- `DISPLAY_FACE_LOOK_LEFT`
- `DISPLAY_FACE_LOOK_RIGHT`
- `DISPLAY_FACE_BLINK`
- `DISPLAY_FACE_SLEEP`
- `DISPLAY_FACE_ERROR`

另外新增了内部辅助状态：

- `DISPLAY_FACE_NONE`

用于表示“当前没有临时触发表情”。

### 5.2 触摸行为

已实现的触摸行为：

- 点击上方区域：
  - 进入 `shy`
  - 数秒后自动回到 `happy`
- 点击中间区域：
  - 立即恢复 `happy`
- 长按 `1.5s`：
  - 进入 `excited`
  - 短暂停留后自动回到 `happy`
- 点击下方区域：
  - 不切换成指定表情
  - 但会记录一次交互，用于阻止立刻休眠或立刻触发自动行为

触摸分区逻辑为纵向三分区：

- `top`
- `middle`
- `bottom`

实现点：

- `display_service_touch_band_from_point()`
- `display_service_update_touch_locked()`

### 5.3 自动行为

已实现的自动行为：

- `look left`
  - 空闲时随机触发
  - 触发间隔 `3~6 秒`
- `look right`
  - 空闲时随机触发
  - 触发间隔 `3~6 秒`
- `blink`
  - 空闲时随机触发
  - 触发间隔 `10~20 秒`
- `sleep`
  - `3 分钟`无交互进入休眠

自动行为约束：

- 自动行为不会覆盖触摸产生的临时表情。
- 自动行为在触摸开始时会被清空并重新排期。
- 休眠优先级高于普通默认表情，但错误态仍优先于休眠。

实现点：

- `display_service_random_range_us()`
- `display_service_schedule_gaze_locked()`
- `display_service_schedule_blink_locked()`
- `display_service_visual_face_from_state()`

### 5.4 表情绘制实现方式

本轮没有重做整套 UI，而是在已有 LVGL 对象基础上扩展：

- 左眼、右眼
- 左右高光
- 左右腮红
- 嘴线对象
- 睡眠标签

新增对象：

- `s_left_shade`
- `s_right_shade`
  - 用于实现 `look left / look right` 时的遮罩感
- `s_mouth_dot`
  - 用于实现 `excited` 的椭圆嘴型

实现策略：

- 通过切换对象尺寸、位置、透明度、阴影和隐藏状态，模拟 `emoji/5.html` 中的 SVG 效果。
- 不是逐像素还原 SVG，但行为和视觉意图已经对齐。

## 6. `emoji/5.html` 的作用

`emoji/5.html` 是本轮最关键的视觉参考文件。

它定义了：

- 8 个目标表情
- 触摸与自动行为分组
- 眼睛宽高与圆角参数
- 腮红、嘴型、眨眼、左右看等视觉规则

关键参考参数包括：

- `EYE_W = 120`
- `EYE_H = 160`
- `EYE_R = 52`
- `look left / right` 的高光偏移逻辑
- `blink` 的单眼闭合效果
- `sleeping` 的 Z 字提示

当前固件实现已经把这份 HTML 的主要交互和表情语义映射到 C 代码中。

## 7. 当前各模块状态

### 7.1 `robot_link`

当前状态：

- 仍是占位 stub。
- `robot_link_poll()` 现在固定返回 `ROBOT_STATE_IDLE`。

意义：

- 保证显示层稳定，不被上层随机错误状态扰动。

后续如果接入 UART、Wi-Fi、MQTT 或语音链路，可以再把真实状态输送回去。

### 7.2 `emotion_engine`

当前状态：

- 仍是轻量状态机。
- 上电先进入 `BOOTING`。
- 大约 `1.5s` 后自动切到 `IDLE`。
- 如果收到新请求状态，就切到请求值。

意义：

- 提供一个稳定的过渡层，让 `display_service` 不直接依赖 `robot_link` 的原始输入。

### 7.3 `display_service`

当前状态：

- 是本轮开发重点。
- 已承担：
  - LCD 初始化
  - GT911 初始化
  - LVGL 初始化
  - 表情对象创建
  - 触摸读取
  - 表情决策
  - 自动行为调度
  - 最终界面渲染

## 8. 当前实现中的优先级关系

显示层当前采用的是“基础状态 + 触摸临时状态 + 自动行为”的组合思路。

优先级大致如下：

1. `ROBOT_STATE_ERROR`
2. 长时间无操作触发的 `sleep`
3. 触摸临时表情
4. 自动行为
5. 默认 `happy`

这样做的目的：

- 保证错误态不会被普通表情覆盖。
- 保证手势表情不会被自动行为抢走。
- 保证休眠能够在真正长时间无交互时生效。

## 9. 已知限制和注意事项

### 9.1 当前还没有在本机完成完整编译验证

我在本轮尝试执行：

```bash
idf.py build
```

但当前 Trae 终端环境里没有 `idf.py`，报错为：

```bash
zsh: command not found: idf.py
```

这说明当前 IDE 终端尚未加载 ESP-IDF 环境，而不是代码本身一定有问题。

### 9.2 诊断器中的剩余报错属于环境问题

VS Code/clang 诊断里仍存在一些报错，例如：

- 未识别的 Xtensa/ESP 编译参数
- 某些 ESP 头文件间接包含路径缺失

这些属于本机 clang 语言服务没有拿到完整 ESP-IDF 工具链配置，不属于本轮新增逻辑导致的 C 语法错误。

### 9.3 自动行为是短时插入，不是复杂动画

当前 `look left / look right / blink` 实现为“短暂切换到对应脸型”，而不是多帧插值动画。

这已经满足“自动行为存在”的需求，但如果后续希望更丝滑，可以继续扩展成：

- 连续帧动画
- 插值偏移
- 更真实的双眼眨动节奏

## 10. 启动与开发细节

### 10.1 运行时启动顺序

设备上电后：

1. 板级初始化
2. 显示初始化
3. 触摸初始化
4. LVGL 初始化
5. 创建脸部 UI 对象
6. 情绪引擎初始化
7. 机器人链路初始化
8. 主循环每 50ms 刷新一次状态与画面

### 10.2 开发时建议的命令

在正确加载 ESP-IDF 环境后，常用命令通常是：

```bash
idf.py build
idf.py flash
idf.py monitor
```

如果终端里没有 `idf.py`，需要先加载 ESP-IDF 环境，例如执行你的本机 ESP-IDF 导出脚本。由于当前工作区里没有明确记录本机的 `export.sh` 绝对路径，所以需要按你的开发机实际安装位置来执行。

### 10.3 启动调试时建议重点关注的日志

建议重点看这些日志是否正常：

- `Display I2C init failed`
- `LCD init failed`
- `Touch init failed`
- `LVGL init failed`
- `Touch controller GT911 initialized`
- `Render face state: ...`

## 11. 本轮对话进展总结

本轮对话的推进过程如下：

1. 用户先反馈：
   - 触摸没有反应
   - 有时表情会变红并抖动

2. 已完成修复：
   - 修掉 GT911 初始化失败
   - 修掉 `robot_link` 周期性错误态

3. 用户再提出新需求：
   - `top` 点击后进入害羞表情
   - 几秒后自动恢复到初始表情
   - 参考图片下方 8 个表情增加触摸点和表情
   - 增加 `look left / look right / blink` 自动行为

4. 已完成实现：
   - 新的表情状态枚举
   - 触摸临时表情超时回退
   - 长按触发兴奋表情
   - 自动左右看
   - 自动眨眼
   - 休眠状态保留

5. 当前正处于可继续迭代阶段：
   - 可以进一步增加更多细分触摸区
   - 可以把自动行为改成更连贯的动画
   - 可以把真实机器人状态接入 `robot_link`

## 12. 下一轮对话建议从哪里继续

下一轮最适合继续做的方向有这几类：

### 方向 A：细分触摸点位

例如把当前三段分区升级成：

- 左上
- 中上
- 右上
- 左中
- 中中
- 右中
- 左下
- 中下
- 右下

然后分别绑定不同表情或动作。

### 方向 B：把自动行为做成更自然的动画

例如：

- 左看前先居中
- 右看有缓动
- 双眼都可眨
- 随机双眨或单眨

### 方向 C：接入真实上层状态

例如：

- 语音监听时显示 `listening`
- 处理中显示 `processing`
- 播报时显示 `speaking`
- 真正的异常状态才进入 `error`

### 方向 D：继续对齐 `emoji/5.html` 视觉细节

例如进一步细调：

- 眼睛位置
- 嘴型弧度
- 腮红尺寸
- 左右看的遮罩宽度
- blink 的闭眼高度和嘴角曲线

## 13. 建议的下一步验证清单

建议上板后按下面顺序验证：

1. 上电后是否先正常显示，约 1.5 秒后进入默认 idle/happy 视觉
2. 点击屏幕上方，是否进入害羞脸，并自动恢复
3. 点击屏幕中间，是否恢复默认开心
4. 长按 1.5 秒，是否进入兴奋脸
5. 放置几秒，是否会随机左右看
6. 放置更久，是否会随机眨眼
7. 放置 3 分钟不操作，是否进入睡眠
8. 睡眠时任意触摸，是否唤醒

## 14. 交接结论

当前工程已经从“基础显示能亮 + 触摸无效 + 状态不稳定”推进到：

- GT911 触摸初始化已修复
- 红脸抖动问题已修复
- 8 个参考表情已基本映射到固件状态机
- 触摸、长按、自动行为、睡眠逻辑已接通

如果下一轮继续开发，建议优先做：

1. 上板验证当前行为
2. 根据实际视觉效果微调参数
3. 细分更多触摸点位
4. 把自动行为做成更自然的动画

