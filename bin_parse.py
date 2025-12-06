import numpy as np
import struct

bs = []
with open(r'e:\sd16\mysd16.bin', 'rb') as f:
    bs = f.read()

DEAD_PIXEL_MASK = np.array(list(bs[0x40000:0x40000+120*160]))
DEAD_PIXEL_MASK = DEAD_PIXEL_MASK.reshape(120, 160)

FLAT_FIELD_CORRECTION = np.array(list(bs[0x120024:0x120024+120*160*2]))
FLAT_FIELD_CORRECTION = FLAT_FIELD_CORRECTION.reshape(120, 160*2)
FLAT_FIELD_CORRECTION = FLAT_FIELD_CORRECTION.astype(np.uint16)
FLAT_FIELD_CORRECTION = (
    FLAT_FIELD_CORRECTION[:, 0::2] << 0) + (FLAT_FIELD_CORRECTION[:, 1::2] << 8)


coeff_bs = bs[0x1040F4:0x1040F4+204]

offset = [
    0xc, 0x10,
    0x34, 0x38,
    0x5c, 0x60,
    0x84, 0x88,
    0xac, 0xb0,
]
coeffs = []

for i in offset:
    f1, = struct.unpack('f', coeff_bs[i:i+4])
    coeffs.append(f1)

BIAS_VOLTAGE = []
BIAS_VOLTAGE.append(int(bs[0x100028:0x100028 + 4].decode(), base=16))
BIAS_VOLTAGE.append(int(bs[0x100032:0x100032 + 4].decode(), base=16))
BIAS_VOLTAGE.append(int(bs[0x10003C:0x10003C + 4].decode(), base=16))
BIAS_VOLTAGE.append(int(bs[0x100046:0x100046 + 4].decode(), base=16))
HORIZONTAL_SYNC_DELAY = int(bs[0x100050:0x100050 + 3].strip(b'\0').decode(), base=10)

print(BIAS_VOLTAGE, HORIZONTAL_SYNC_DELAY)


with open('uvc_app/calibration_data.h', 'w+') as f:
    f.write('#ifndef __CALIBRATION_DATA_H__\n')
    f.write('#define __CALIBRATION_DATA_H__\n')
    f.write('#include <stdio.h>\n')
    f.write('#include <stdlib.h>\n')

    f.write('const uint8_t DEAD_PIXEL_MASK[120][160] =\n{\n')
    for i in range(120):
        f.write(' '*4+'{\n')
        for j in range(160):
            f.write(' '*8+f'{DEAD_PIXEL_MASK[i][j]},\n')
        f.write(' '*4+'},\n')
    f.write('};\n\n')

    f.write('const uint16_t FLAT_FIELD_CORRECTION[120][160] =\n{\n')
    for i in range(120):
        f.write(' '*4+'{\n')
        for j in range(160):
            f.write(' '*8+f'{FLAT_FIELD_CORRECTION[i][j]},\n')
        f.write(' '*4+'},\n')
    f.write('};\n\n')

    for i in range(0, len(coeffs), 2):
        f.write(f'#define COEFF_K{i >> 1} ({coeffs[i]}f)\n')
        f.write(f'#define COEFF_B{i >> 1} ({coeffs[i+1]}f)\n')

    f.write('#endif /*__CALIBRATION_DATA_H__*/\n')


with open('uvc_app/init_calibration_data.h', 'w+') as f:
    f.write('#ifndef __INIT_CALIBRATION_DATA_H__\n')
    f.write('#define __INIT_CALIBRATION_DATA_H__\n')
    f.write('#include <stdio.h>\n')
    f.write('#include <stdlib.h>\n')

    f.write(f'#define BIAS_VOLTAGE0 ({BIAS_VOLTAGE[0]})\n')
    f.write(f'#define BIAS_VOLTAGE1 ({BIAS_VOLTAGE[1]})\n')
    f.write(f'#define BIAS_VOLTAGE2 ({BIAS_VOLTAGE[2]})\n')
    f.write(f'#define BIAS_VOLTAGE3 ({BIAS_VOLTAGE[3]})\n')
    f.write(f'#define HORIZONTAL_SYNC_DELAY ({HORIZONTAL_SYNC_DELAY})\n')
    
    f.write('#endif /*__INIT_CALIBRATION_DATA_H__*/\n')