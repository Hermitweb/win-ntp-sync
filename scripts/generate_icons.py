#!/usr/bin/env python3
"""
生成 WinTimeSync 应用图标
输出 assets/ 目录下的多个 .ico 文件，分别表示不同状态
"""

from PIL import Image, ImageDraw, ImageFont
import os
import math

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# 颜色定义
COLOR_BG_DARK = (32, 80, 160, 255)       # 深蓝背景
COLOR_BG_NORMAL = (45, 125, 50, 255)     # 绿色 - 正常
COLOR_BG_WARNING = (220, 145, 30, 255)   # 黄色 - 警告
COLOR_BG_ERROR = (190, 40, 40, 255)      # 红色 - 错误
COLOR_BG_SYNCING = (60, 130, 200, 255)   # 蓝色 - 同步中
COLOR_WHITE = (255, 255, 255, 255)
COLOR_SHADOW = (0, 0, 0, 80)


def draw_clock_face(image, color_bg):
    """绘制时钟图标（圆形 + 指针）"""
    draw = ImageDraw.Draw(image, "RGBA")
    size = image.size[0]

    # 阴影
    margin = size // 12
    draw.ellipse(
        [margin + 2, margin + 4, size - margin + 2, size - margin + 4],
        fill=COLOR_SHADOW
    )

    # 外圈背景
    draw.ellipse(
        [margin, margin, size - margin, size - margin],
        fill=color_bg,
        outline=COLOR_WHITE,
        width=size // 32 + 1
    )

    # 内圈
    inner_margin = margin + size // 12
    draw.ellipse(
        [inner_margin, inner_margin, size - inner_margin, size - inner_margin],
        outline=COLOR_WHITE,
        width=size // 48 + 1
    )

    # 刻度（12 个）
    center = size // 2
    radius_clock = (size - 2 * margin) // 2 - size // 16
    for i in range(12):
        angle = math.radians(i * 30)
        # 主要刻度
        is_major = (i % 3 == 0)
        inner_r = radius_clock - (size // 20 if is_major else size // 32)
        outer_r = radius_clock
        x1 = center + inner_r * math.sin(angle)
        y1 = center - inner_r * math.cos(angle)
        x2 = center + outer_r * math.sin(angle)
        y2 = center - outer_r * math.cos(angle)
        line_w = size // 48 + 1 if is_major else max(1, size // 80)
        draw.line([(x1, y1), (x2, y2)], fill=COLOR_WHITE, width=line_w)

    # 时针 (10 点方向)
    hour_angle = math.radians(10 * 30 - 90)  # 10 点位置
    hour_len = radius_clock * 0.5
    hx = center + hour_len * math.cos(hour_angle)
    hy = center + hour_len * math.sin(hour_angle)
    draw.line([(center, center), (hx, hy)], fill=COLOR_WHITE, width=size // 24 + 1)

    # 分针 (2 点方向)
    min_angle = math.radians(2 * 6 - 90)  # 12 分位置
    min_len = radius_clock * 0.75
    mx = center + min_len * math.cos(min_angle)
    my = center + min_len * math.sin(min_angle)
    draw.line([(center, center), (mx, my)], fill=COLOR_WHITE, width=size // 40 + 1)

    # 中心点
    dot_r = size // 24
    draw.ellipse(
        [center - dot_r, center - dot_r, center + dot_r, center + dot_r],
        fill=COLOR_WHITE
    )


def draw_warning_badge(image):
    """在图标右下角添加警告感叹号"""
    draw = ImageDraw.Draw(image, "RGBA")
    size = image.size[0]
    badge_r = size // 5
    cx = size - badge_r - size // 16
    cy = size - badge_r - size // 16

    # 圆形黄色背景
    draw.ellipse(
        [cx - badge_r, cy - badge_r, cx + badge_r, cy + badge_r],
        fill=(255, 200, 0, 255),
        outline=COLOR_WHITE,
        width=size // 80 + 1
    )

    # 感叹号
    bang_w = size // 24 + 1
    bang_h = size // 8
    draw.rectangle(
        [cx - bang_w // 2, cy - bang_h, cx + bang_w // 2, cy],
        fill=(80, 60, 0, 255)
    )
    dot_r = bang_w
    draw.ellipse(
        [cx - dot_r, cy + size // 24, cx + dot_r, cy + size // 24 + 2 * dot_r],
        fill=(80, 60, 0, 255)
    )


def draw_error_badge(image):
    """在图标右下角添加错误 X 标记"""
    draw = ImageDraw.Draw(image, "RGBA")
    size = image.size[0]
    badge_r = size // 5
    cx = size - badge_r - size // 16
    cy = size - badge_r - size // 16

    # 红色圆形
    draw.ellipse(
        [cx - badge_r, cy - badge_r, cx + badge_r, cy + badge_r],
        fill=(220, 30, 30, 255),
        outline=COLOR_WHITE,
        width=size // 80 + 1
    )

    # X 标记
    line_w = size // 24 + 1
    x_len = badge_r * 0.6
    draw.line(
        [(cx - x_len, cy - x_len), (cx + x_len, cy + x_len)],
        fill=COLOR_WHITE, width=line_w
    )
    draw.line(
        [(cx - x_len, cy + x_len), (cx + x_len, cy - x_len)],
        fill=COLOR_WHITE, width=line_w
    )


def draw_sync_arc(image):
    """在时钟上绘制同步动画圆弧（箭头效果）"""
    draw = ImageDraw.Draw(image, "RGBA")
    size = image.size[0]
    center = size // 2
    radius = (size - 2 * (size // 12)) // 2 + size // 32

    # 绘制 270 度圆弧（表示循环）
    arc_box = [center - radius, center - radius, center + radius, center + radius]
    draw.arc(arc_box, start=30, end=300, fill=(255, 255, 255, 200), width=size // 24 + 1)

    # 箭头（端点处）
    arrow_size = size // 16
    # 计算起点
    angle_rad = math.radians(30)
    sx = center + radius * math.cos(angle_rad)
    sy = center + radius * math.sin(angle_rad)
    # 三角形箭头
    draw.polygon(
        [(sx, sy), (sx - arrow_size, sy - arrow_size // 2),
         (sx - arrow_size, sy + arrow_size // 2)],
        fill=(255, 255, 255, 220)
    )


def make_ico(images, filename):
    """保存为多尺寸 ICO"""
    filepath = os.path.join(OUTPUT_DIR, filename)
    images[0].save(
        filepath,
        format="ICO",
        sizes=[(img.width, img.height) for img in images],
        append_images=images[1:]
    )
    print(f"已生成: {filepath} ({len(images)} 个尺寸)")


def main():
    sizes = [16, 24, 32, 48, 64, 128, 256]

    # 1. 正常状态图标
    images_normal = []
    for s in sizes:
        img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        draw_clock_face(img, COLOR_BG_NORMAL)
        images_normal.append(img)
    make_ico(images_normal, "icon_normal.ico")

    # 2. 警告状态图标
    images_warning = []
    for s in sizes:
        img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        draw_clock_face(img, COLOR_BG_WARNING)
        draw_warning_badge(img)
        images_warning.append(img)
    make_ico(images_warning, "icon_warning.ico")

    # 3. 错误状态图标
    images_error = []
    for s in sizes:
        img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        draw_clock_face(img, COLOR_BG_ERROR)
        draw_error_badge(img)
        images_error.append(img)
    make_ico(images_error, "icon_error.ico")

    # 4. 同步中状态图标
    images_syncing = []
    for s in sizes:
        img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        draw_clock_face(img, COLOR_BG_SYNCING)
        draw_sync_arc(img)
        images_syncing.append(img)
    make_ico(images_syncing, "icon_syncing.ico")

    # 5. 应用主图标（256x256 PNG，用于其他用途）
    img_main = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    draw_clock_face(img_main, COLOR_BG_NORMAL)
    main_png = os.path.join(OUTPUT_DIR, "icon_main.png")
    img_main.save(main_png, "PNG")
    print(f"已生成: {main_png}")

    print("所有图标生成完成！")


if __name__ == "__main__":
    main()
