// main/game_monkey_config.h —— 《尖叫猴子》唯一调参入口。
// 纯常量与类型,不依赖 ESP-IDF/LVGL;逻辑层与宿主测试共用。
// 美术规格来源:.workbuddy/deliverables/尖叫猴子-美术设计.md(矩阵源 assets/art/sm_art_matrices.mjs)。
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ===========================================================================
// 音频采集与喊叫检测(16kHz/16bit/单声道,与 BSP 默认格式一致)
// ===========================================================================
#define SM_SAMPLE_RATE_HZ    16000
#define SM_CHUNK_SAMPLES     240      // 15ms @16kHz,与 I2S DMA 描述符对齐
#define SM_CALIB_CHUNKS      20       // 开局静默校准块数(约 300ms)

#define SM_DC_SHIFT          5        // 去直流:IIR 泄漏(右移位数)
#define SM_FLOOR_DOWN_SHIFT  4        // 噪声地板下探速度(快)
#define SM_FLOOR_UP_STEP     1        // 噪声地板上爬步长(极慢,每块)

#define SM_LEVEL_SPAN        9000     // RMS 超出噪声地板多少映射到满值(需真机标定)
#define SM_EMA_ATTACK_Q8     48       // 非对称 EMA 攻击系数(快,约 80ms)
#define SM_EMA_RELEASE_Q8    13       // 非对称 EMA 释放系数(慢)

// ===========================================================================
// 分贝→跳跃映射(连续;level 0..100 来自检测器)
//   v0 = V_MIN + (level - SHOUT_ON) * (V_MAX - V_MIN) / (SHOUT_HI - SHOUT_ON)
// ===========================================================================
#define SM_SHOUT_ON          50       // 触发阈值(柱状图刻度线位置)
#define SM_SHOUT_HI          95       // 满速映射点
#define SM_SHOUT_OFF         38       // 滞回下限:回落到此之下才重新武装

// ===========================================================================
// 猴子物理(屏幕 y 向下为正;Q16.16 定点,半隐式欧拉)
// ===========================================================================
#define SM_Q                 16
#define SM_Q_ONE             (1 << SM_Q)

#define SM_GRAVITY_Q         (1350 << SM_Q)   // 重力 px/s^2
#define SM_V_JUMP_MIN_Q      (270 << SM_Q)    // 最轻喊叫起跳初速 → 跳高约 27px
#define SM_V_JUMP_MAX_Q      (540 << SM_Q)    // 最响喊叫起跳初速 → 跳高约 108px
#define SM_V_FALL_MAX_Q      (700 << SM_Q)    // 下落速度上限

// ===========================================================================
// 布局(240x320 竖屏,美术设计 §4.1)
// ===========================================================================
#define SM_LCD_W             240
#define SM_LCD_H             320
#define SM_GROUND_Y          231      // 地面顶基准线(脚底站立位置)
#define SM_MONKEY_X          36       // 猴子精灵左缘(固定)
#define SM_MONKEY_W          24
#define SM_MONKEY_H          24

// 碰撞盒:相对精灵内缩(美术设计 §3.1)
#define SM_BOX_OFF_X         4
#define SM_BOX_OFF_Y         4
#define SM_BOX_W             16
#define SM_BOX_H             18

// ===========================================================================
// 跑速 / 里程 / 状态时长
// ===========================================================================
#define SM_SPEED_BASE_Q      (90 << SM_Q)     // 起始滚动速度 px/s
#define SM_SPEED_MAX_Q       (170 << SM_Q)    // 速度上限
#define SM_RAMP_MS           120000u          // 线性爬坡时长
#define SM_SLOW_PCT          55               // 被石头砸中后速度百分比
#define SM_SLOW_MS           15000u           // 减速持续时间
#define SM_INVULN_MS         1200u            // 受击无敌帧(防连撞)
#define SM_READY_MS          1800u            // 开跑倒计时
#define SM_WARMUP_MS         8000u            // 开跑后无障碍热身时长
#define SM_FALL_LIMIT_PX     40               // 坠崖超过地面线下多少像素判死
#define SM_PX_PER_METER      8                // 里程换算

// ===========================================================================
// 障碍
// ===========================================================================
typedef enum {
    SM_OBST_NONE = 0,
    SM_OBST_STONE,       // 石头:撞=疼(减速15s+掉血)
    SM_OBST_CACTUS,      // 仙人掌:撞=掉血
    SM_OBST_GAP,         // 悬崖:坠落即终局
} sm_obst_kind_t;

#define SM_STONE_W           16
#define SM_STONE_H           14       // 碰撞盒比精灵矮(16x16 精灵)
#define SM_CACTUS_W          12
#define SM_CACTUS_H          24
#define SM_GAP_MIN_PX        34
#define SM_GAP_MAX_PX        58
#define SM_OBST_MAX          6        // 最小间距 x 屏宽约束下的上限
#define SM_MIN_SPACING_PX    176      // 生成最小间距 = 最大滞空水平距离 + 40px 反应余量

// ===========================================================================
// 死亡原因 / 阶段
// ===========================================================================
typedef enum {
    SM_PHASE_CALIB = 0,  // 麦克风噪声校准(由检测器驱动,世界模型空转)
    SM_PHASE_READY,      // 倒计时
    SM_PHASE_RUN,        // 跑酷中
    SM_PHASE_OVER,       // 结束
} sm_phase_t;

#define SM_HP_MAX            3

typedef enum {
    SM_DEATH_HP = 0,     // 血量归零
    SM_DEATH_FALL,       // 坠崖
} sm_death_t;

// tick 事件位图(每 tick 重新清零,由 UI/音频消费)
#define SM_EVT_NONE       0u
#define SM_EVT_LAND       (1u << 0)
#define SM_EVT_HURT       (1u << 1)
#define SM_EVT_FALL_START (1u << 2)
#define SM_EVT_DEATH      (1u << 3)

// ===========================================================================
// SFX(运行时合成)
// ===========================================================================
#define SM_SFX_MAX_SAMPLES   6400     // 400ms @16kHz,最长音效(死亡 390ms)可容纳
#define SM_SFX_AMP           8000     // ≈25% 满幅,抑制扬声器回授
#define SM_SFX_FADE          80       // 整条音效首尾淡入淡出采样数(5ms)

// ===========================================================================
// NVS 榜单
// ===========================================================================
#define SM_BOARD_N           5
#define SM_GAME_NS           "sm_game"
#define SM_KEY_TOP5          "top5"
#define SM_KEY_COUNT         "top5_n"

// ===========================================================================
// UI
// ===========================================================================
#define SM_HUD_H             26       // 顶部信息条高度
#define SM_BAR_X             214      // dB 柱状图右缘竖条
#define SM_BAR_W             22
#define SM_BAR_SEGS          12
#define SM_UI_TICK_MS        33u      // 世界 tick ≈30fps
#define SM_UI_TICK_MS_SLOW   42u      // 真机掉帧时的降级档(≈24fps)
