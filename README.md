# emoji_on_esp32s3_touch_lcd_4.3B

`v1.0.0` 是一个基于 ESP32-S3 4.3 寸触摸屏的机器人表情显示原型，聚焦“表情状态切换 + 触摸交互 + 真实状态接入”的最小可用版本。

## 产品功能

- 基于 LVGL 的机器人表情显示界面
- 支持 `booting`、`idle`、`listening`、`processing`、`speaking`、`happy`、`sleep`、`error` 等基础状态
- 支持 9 宫格触摸交互，不同区域触发不同视觉反馈
- 支持左右看、害羞、说话、睡眠、兴奋等表情切换
- 支持更自然的 gaze 与 blink 动画，左右看与眨眼过渡更加平滑
- 支持通过 USB Serial JTAG 接入真实 `robot_link` 状态源

## 版本详情

### v1.0.0

发布日期：2026-04-05

本版本完成了项目第一阶段可运行闭环：

- 打通 `robot_link -> emotion_engine -> display_service` 状态链路
- 完成触摸驱动接入与 9 区域交互映射
- 完成基础表情渲染与自动行为调度
- 优化左右看动画，减少瞬时跳变
- 加入眨眼功能，并增强 look/blink 的自然感
- 参考 `emoji/5.html` 持续对齐眼睛比例、视线偏移和整体观感

## 状态输入示例

设备运行后，可通过 USB Serial JTAG 发送一行文本切换状态：

```text
idle
listening
processing
speaking
happy
sleep
error
```

也兼容如下格式：

```text
state=speaking
{"state":"processing"}
```

## 目录说明

- `components/display/`：显示、动画、触摸交互
- `components/emotion/`：情绪状态机
- `components/robot_link/`：外部状态输入接入
- `main/`：主程序入口
- `emoji/`：表情视觉参考稿

## 当前定位

`v1.0.0` 为可演示、可继续迭代的原型版本。后续将继续优化动画流畅度、修复运行时稳定性问题，并补充更完整的状态接入与视觉细节。
