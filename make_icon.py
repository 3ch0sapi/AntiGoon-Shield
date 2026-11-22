import zlib, struct, math, os

def make_png(width, height, rgba_data):
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw.extend(rgba_data[y * width * 4 : (y + 1) * width * 4])
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b'')

def render_shield(size):
    buf = bytearray(size * size * 4)
    cx, cy = size / 2.0, size / 2.0
    scale = size / 100.0

    def inside_shield(px, py, offset=0.0):
        x = (px - cx) / scale
        y = (py - cy) / scale - 3.0
        w = 38.0 - offset
        h = 42.0 - offset
        if y < -h or y > h or abs(x) > w:
            return False
        if y <= 0:
            return True
        ny = y / h
        allowed_w = w * (1.0 - (ny ** 1.8) * 0.95)
        return abs(x) <= allowed_w

    def inside_checkmark(px, py):
        x = (px - cx) / scale
        y = (py - cy) / scale
        p1 = (-14.0, 0.0)
        p2 = (-3.0, 11.0)
        p3 = (15.0, -9.0)
        thick = 4.2

        def dist_seg(x, y, x1, y1, x2, y2):
            dx, dy = x2 - x1, y2 - y1
            l2 = dx*dx + dy*dy
            if l2 == 0: return math.hypot(x-x1, y-y1)
            t = max(0.0, min(1.0, ((x - x1) * dx + (y - y1) * dy) / l2))
            projx = x1 + t * dx
            projy = y1 + t * dy
            return math.hypot(x - projx, y - projy)

        d = min(dist_seg(x, y, p1[0], p1[1], p2[0], p2[1]),
                dist_seg(x, y, p2[0], p2[1], p3[0], p3[1]))
        return d <= thick

    for y in range(size):
        for x in range(size):
            idx = (y * size + x) * 4
            sub_outer = 0
            sub_inner = 0
            sub_check = 0
            steps = 3
            for sy in range(steps):
                for sx in range(steps):
                    px = x + (sx + 0.5) / steps
                    py = y + (sy + 0.5) / steps
                    if inside_shield(px, py, 0.0):
                        sub_outer += 1
                        if inside_shield(px, py, 4.0):
                            sub_inner += 1
                            if inside_checkmark(px, py):
                                sub_check += 1

            total_sub = steps * steps
            if sub_outer == 0:
                buf[idx:idx+4] = [0, 0, 0, 0]
            else:
                alpha_outer = sub_outer / total_sub
                if sub_check > 0:
                    c_alpha = sub_check / total_sub
                    buf[idx:idx+4] = [0, 230, 118, int(c_alpha * 255)]
                elif sub_inner == 0:
                    norm_y = y / size
                    r = int(0 * (1 - norm_y) + 0 * norm_y)
                    g = int(230 * (1 - norm_y) + 200 * norm_y)
                    b = int(140 * (1 - norm_y) + 255 * norm_y)
                    buf[idx:idx+4] = [r, g, b, int(alpha_outer * 255)]
                else:
                    norm_y = y / size
                    r = int(20 + 15 * norm_y)
                    g = int(24 + 15 * norm_y)
                    b = int(38 + 20 * norm_y)
                    buf[idx:idx+4] = [r, g, b, int(alpha_outer * 255)]

    return bytes(buf)

def build_ico(sizes, output_path):
    pngs = []
    for s in sizes:
        rgba = render_shield(s)
        png = make_png(s, s, rgba)
        pngs.append((s, png))

    count = len(pngs)
    header = struct.pack('<HHH', 0, 1, count)
    entries = bytearray()
    images = bytearray()
    offset = 6 + count * 16

    for s, png in pngs:
        w_byte = 0 if s >= 256 else s
        h_byte = 0 if s >= 256 else s
        entry = struct.pack('<BBBBHHII', w_byte, h_byte, 0, 0, 1, 32, len(png), offset)
        entries.extend(entry)
        images.extend(png)
        offset += len(png)

    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(entries)
        f.write(images)

if __name__ == '__main__':
    base_dir = os.path.dirname(os.path.abspath(__file__))
    res_dir = os.path.join(base_dir, "res")
    os.makedirs(res_dir, exist_ok=True)
    ico_path = os.path.join(res_dir, "app.ico")
    build_ico([256, 64, 48, 32, 16], ico_path)
