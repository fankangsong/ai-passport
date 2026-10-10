// 《尖叫猴子》SFX 合成 host 测试:长度/包络/无削顶/确定性/容量拒绝。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game_monkey_sfx.h"

static int32_t max_abs(const int16_t *buf, size_t n) {
    int32_t m = 0;
    for (size_t i = 0; i < n; i++) {
        const int32_t v = buf[i] < 0 ? -buf[i] : buf[i];
        if (v > m) m = v;
    }
    return m;
}

int main(void) {
    static int16_t buf1[SM_SFX_MAX_SAMPLES];
    static int16_t buf2[SM_SFX_MAX_SAMPLES];

    const size_t expected[SM_SFX_COUNT] = {
        [SM_SFX_JUMP]   = 1440,   // 90ms  @16k
        [SM_SFX_LAND]   = 960,    // 60ms
        [SM_SFX_HURT]   = 3200,   // 60+140ms
        [SM_SFX_DEATH]  = 6240,   // 130*3ms
        [SM_SFX_RECORD] = 5760,   // 90*4ms
    };

    for (int fx = 0; fx < SM_SFX_COUNT; fx++) {
        const size_t n = sm_sfx_render((sm_sfx_t)fx, buf1, SM_SFX_MAX_SAMPLES, 16000u);
        assert(n == expected[fx]);
        assert(n <= SM_SFX_MAX_SAMPLES);
        assert(max_abs(buf1, n) <= SM_SFX_AMP);          // 无削顶
        assert(buf1[0] == 0);                            // 淡入起点归零
        const int32_t tail = buf1[n - 1] < 0 ? -buf1[n - 1] : buf1[n - 1];
        assert(tail <= SM_SFX_AMP * 4 / 80);             // 淡出收尾接近零
        // 确定性
        const size_t n2 = sm_sfx_render((sm_sfx_t)fx, buf2, SM_SFX_MAX_SAMPLES, 16000u);
        assert(n2 == n);
        assert(memcmp(buf1, buf2, n * sizeof(int16_t)) == 0);
    }

    // 容量不足:拒绝渲染且不写缓冲。
    memset(buf2, 0x5a, sizeof(buf2));
    assert(sm_sfx_render(SM_SFX_DEATH, buf2, expected[SM_SFX_DEATH] - 1, 16000u) == 0);
    assert((uint8_t)buf2[0] == 0x5a);

    // 非法参数。
    assert(sm_sfx_render((sm_sfx_t)SM_SFX_COUNT, buf1, SM_SFX_MAX_SAMPLES, 16000u) == 0);
    assert(sm_sfx_render(SM_SFX_JUMP, NULL, SM_SFX_MAX_SAMPLES, 16000u) == 0);
    assert(sm_sfx_render(SM_SFX_JUMP, buf1, SM_SFX_MAX_SAMPLES, 0u) == 0);

    puts("scream monkey sfx tests: PASS");
    return 0;
}
