import cv2
import numpy as np

# 打开摄像头，参数0表示第一个摄像头
cap = cv2.VideoCapture(1)

# 获取帧率
fps = cap.get(cv2.CAP_PROP_FPS)
# 获取分辨率
cols, rows = (int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)),
        int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)))
print(f'h:{cols} v:{rows} fps:{fps}')
#默认转换为RGB，关掉
cap.set(cv2.CAP_PROP_CONVERT_RGB, 0)

zoom = 4
colormap = cv2.COLORMAP_BONE
#colormap = cv2.COLORMAP_JET
while True:
    # 读取一帧
    ret, frame = cap.read()
    if isinstance(frame, np.ndarray):
        frame = frame.reshape(rows, cols*2)
        frame = frame.astype(np.uint16)               
        frame = (frame[:, 0::2] << 0) + (frame[:, 1::2] << 8)
        frame = frame[0:120, :]
        min_temp = np.min(frame) / 10 - 273
        max_temp = np.max(frame) / 10 - 273
        frame = cv2.normalize(frame, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U)
        normed = cv2.applyColorMap(frame, colormap)

        bar_height, bar_width = frame.shape[0]*zoom, 40
        # 生成从最低到最高温度的渐变序列（垂直条的温度分布）
        temp_range = np.linspace(min_temp, max_temp, bar_height)

        # 归一化温度序列用于伪彩
        if max_temp != min_temp:
            norm_bar = ((temp_range - min_temp) / (max_temp - min_temp) * 255).astype(np.uint8)
        else:
            norm_bar = np.ones(bar_height, dtype=np.uint8) * 127

        # 转换为伪彩并扩展为条带宽度
        color_bar = cv2.applyColorMap(np.flip(norm_bar), colormap)
        color_bar = np.repeat(color_bar, bar_width, axis=1)
        
        # 8. 在垂直条上标注最高/最低温度
        # 最高温度（位于条带顶部）
        cv2.putText(
            color_bar, 
            f"{max_temp:.1f}C", 
            (2, 15),  # 位置（x,y）
            cv2.FONT_HERSHEY_SIMPLEX, 
            0.4,  # 字体大小
            (0, 0, 0),
            1  # 线宽
        )
        # 最低温度（位于条带底部）
        cv2.putText(
            color_bar, 
            f"{min_temp:.1f}C", 
            (2, bar_height - 5), 
            cv2.FONT_HERSHEY_SIMPLEX, 
            0.4, 
            (255, 255, 255), 
            1
        )

        normed = cv2.resize(normed, (int(
            normed.shape[1]*zoom), int(normed.shape[0]*zoom)), interpolation=cv2.INTER_NEAREST)#
        combined_img = cv2.hconcat([normed, color_bar])
        cv2.imshow("DALI TECH SD16", combined_img)
        # 按下ESC键退出
        c = cv2.waitKey(1)
        if c == 27:
            # 释放摄像头
            cap.release()
            cv2.destroyAllWindows()
            break