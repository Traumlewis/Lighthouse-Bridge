"""Explicit rs-matter build profiles; no automatic chip substitution."""
BOARDS = {
    'esp32': dict(label='ESP-WROOM-32 / ESP-32S', family='ESP32', chip=0, boot=0x1000,
                  target='xtensa-esp32-none-elf', arduino='esp32', options=''),
    'weact-c3': dict(label='WeAct ESP32-C3FH4', family='ESP32-C3', chip=5, boot=0,
                     target='riscv32imc-unknown-none-elf', arduino='esp32c3', options=',CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M'),
    'c3-supermini': dict(label='ESP32 C3 SuperMini', family='ESP32-C3', chip=5, boot=0,
                        target='riscv32imc-unknown-none-elf', arduino='esp32c3', options=',CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M'),
    's3-supermini': dict(label='ESP32 S3 SuperMini (4 MB flash / 2 MB PSRAM)', family='ESP32-S3', chip=9, boot=0,
                        target='xtensa-esp32s3-none-elf', arduino='esp32s3', options=',USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M,PSRAM=enabled'),
    'xiao-c3': dict(label='Seeed Studio XIAO ESP32-C3', family='ESP32-C3', chip=5, boot=0,
                    target='riscv32imc-unknown-none-elf', arduino='XIAO_ESP32C3',
                    options=',CDCOnBoot=default,FlashMode=qio,FlashSize=4M'),
    'xiao-s3': dict(label='Seeed Studio XIAO ESP32-S3', family='ESP32-S3', chip=9, boot=0,
                    target='xtensa-esp32s3-none-elf', arduino='XIAO_ESP32S3',
                    partition='default_8MB', flash_mib=8, psram_type='opi',
                    options=',USBMode=hwcdc,CDCOnBoot=default,FlashMode=qio,FlashSize=8M,PSRAM=opi'),
}
