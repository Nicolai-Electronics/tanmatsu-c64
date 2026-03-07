#include "i2s.hpp"
#include <cstdint>
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/i2s_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/i2s_types.h"
#include "sid.hpp"
#include "sid/sid.hpp"

extern "C" {
#include "bsp/audio.h"
}

static const char* TAG = "I2S";

esp_err_t I2S::init()
{
    esp_err_t res = bsp_audio_set_volume(0);
    if (res != ESP_OK) return res;
    res = bsp_audio_get_i2s_handle(&i2s_handle);
    if (res != ESP_OK) return res;
    res = i2s_channel_disable(i2s_handle);
    if (res != ESP_OK) return res;
    res = bsp_audio_set_rate(DEFAULT_SAMPLERATE);
    if (res != ESP_OK) return res;
    res = i2s_channel_enable(i2s_handle);
    if (res != ESP_OK) return res;
    res = bsp_audio_set_volume(60);
    if (res != ESP_OK) return res;
    res = bsp_audio_set_amplifier(false);
    if (res != ESP_OK) return res;

    return ESP_OK;
}

// Union for left and right audio channel to uint32_t
union MonoToStereo {
    uint32_t val;
    struct {
        int16_t l;
        int16_t r;
    };
};

esp_err_t I2S::write(const int16_t* data, size_t size)
{
    size_t          bytes_written;
    static uint32_t stereo_sample;
    assert(sizeof(i2s_stereo_out) >= size * 2 * 2);  // 2 channels * 2 bytes per sample
    static int16_t swapped;

    for (size_t i = 0; i < size; i++) {
        // Convert the union to use int16_t to match the data type
        // swapped = ((uint16_t(data[i]) << 8) & 0xFF00) | ((uint16_t(data[i]) >> 8) & 0x00FF);
        swapped = data[i];

        stereo_sample = MonoToStereo{.l = swapped, .r = swapped}.val;

        reinterpret_cast<uint32_t*>(i2s_stereo_out)[i] = stereo_sample;
    }

    // size * 2 * 2 because of 2 channels (left and right) and 2 bytes per sample (int16_t)
    i2s_channel_write(i2s_handle, (uint8_t const*)i2s_stereo_out, size * 2 * 2, &bytes_written, 12);
    if (bytes_written < size * 2 * 2) {
        ESP_LOGE(TAG, "Failed to write to I2S buffer %d != %d", bytes_written, size * 2 * 2);
        return ESP_FAIL;
    }
    return ESP_OK;
}