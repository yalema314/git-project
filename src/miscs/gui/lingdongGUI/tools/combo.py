import os
import struct
import argparse
import re

def merge_bin_files(input_dir):
    script_dir = input_dir if input_dir else os.path.dirname(os.path.abspath(__file__))

    fonts_bin_path = os.path.join(script_dir, 'fonts', 'fonts.bin')
    images_bin_path = os.path.join(script_dir, 'images', 'images.bin')
    output_bin_path = os.path.join(script_dir, 'output', 'data.bin')

    fonts_data = b''
    if os.path.exists(fonts_bin_path):
        with open(fonts_bin_path, 'rb') as f:
            fonts_data = f.read()
    else:
        print(f"警告: {fonts_bin_path} 文件不存在，将使用空数据")

    images_data = b''
    if os.path.exists(images_bin_path):
        with open(images_bin_path, 'rb') as f:
            images_data = f.read()
    else:
        print(f"警告: {images_bin_path} 文件不存在，将使用空数据")

    with open(output_bin_path, 'wb') as f:
        f.write(fonts_data)
        f.write(images_data)


def process_image_name(name):
    suffix_mapping = {
        'ALPHA': 'MASK',
        'A1ALPHA': 'A1_MASK',
        'A2ALPHA': 'A2_MASK',
        'A4ALPHA': 'A4_MASK'
    }

    last_underscore_index = name.rfind('_')
    if last_underscore_index == -1:
        return name

    base_name = name[:last_underscore_index]
    suffix = name[last_underscore_index+1:]

    if suffix in suffix_mapping:
        return base_name + '_' + suffix_mapping[suffix]
    else:
        return name

def read_file_if_exists(file_path, file_type):
    lines = []
    if os.path.exists(file_path):
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    else:
        print(f"警告: {file_path} 文件不存在，将使用空数据")
    return lines

def process_name_with_suffix(name, name_bases, names_with_suffix, names_to_keep_suffix):
    final_name = name
    if name in names_with_suffix and name not in names_to_keep_suffix:
        last_underscore_index = name.rfind('_')
        final_name = name[:last_underscore_index]
    return final_name


def merge_txt_files(input_dir):
    script_dir = input_dir if input_dir else os.path.dirname(os.path.abspath(__file__))

    fonts_txt_path = os.path.join(script_dir, 'fonts', 'fonts.txt')
    images_txt_path = os.path.join(script_dir, 'images', 'images.txt')
    output_header_path = os.path.join(script_dir, 'binData.h')

    fonts_bin_path = os.path.join(script_dir, 'fonts', 'fonts.bin')
    fonts_bin_size = os.path.getsize(fonts_bin_path) if os.path.exists(fonts_bin_path) else 0

    fonts_lines = read_file_if_exists(fonts_txt_path, "fonts")
    images_lines = read_file_if_exists(images_txt_path, "images")

    processed_images_lines = []
    image_entries = []
    for line in images_lines:
        if line.strip():
            parts = line.strip().split(':')
            if len(parts) >= 2:
                try:
                    name = "IMAGE_" + parts[0]
                    offset_str = parts[1]
                    if offset_str.startswith('0x') or offset_str.startswith('0X'):
                        offset = int(offset_str, 16)
                    else:
                        offset = int(offset_str, 16) 

                    processed_name = process_image_name(name)
                    
                    new_offset = offset + fonts_bin_size
                    image_entries.append((processed_name, new_offset))
                except ValueError:
                    processed_images_lines.append(line)
            else:
                processed_images_lines.append(line)
        else:
            processed_images_lines.append(line)

    image_groups = {}
    for name, offset in image_entries:
        last_underscore_index = name.rfind('_')
        if last_underscore_index != -1:
            suffix = name[last_underscore_index+1:]
            if suffix in ['MASK', 'A1_MASK', 'A2_MASK', 'A4_MASK']:
                base_name = name
            else:
                base_name = name[:last_underscore_index]
            if base_name not in image_groups:
                image_groups[base_name] = {}
            image_groups[base_name][suffix] = offset
        else:
            image_groups[name] = {'DEFAULT': offset}

    for base_name, suffixes in image_groups.items():
        if len(suffixes) == 1 and 'DEFAULT' in suffixes:
            offset = suffixes['DEFAULT']
            processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
        elif len(suffixes) == 1:
            suffix = list(suffixes.keys())[0]
            offset = suffixes[suffix]
            if suffix in ['MASK', 'A1_MASK', 'A2_MASK', 'A4_MASK']:
                processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
            elif suffix == 'GRAY8':
                processed_images_lines.append(f"#if LD_CFG_COLOR_DEPTH == 8\n")
                processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                processed_images_lines.append(f"#endif\n")
            elif suffix == 'RGB565':
                processed_images_lines.append(f"#if LD_CFG_COLOR_DEPTH == 16\n")
                processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                processed_images_lines.append(f"#endif\n")
            else:
                processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
        else:
            has_mask_suffix = any(s in ['MASK', 'A1_MASK', 'A2_MASK', 'A4_MASK'] for s in suffixes.keys())
            color_suffixes = {k: v for k, v in suffixes.items() if k not in ['MASK', 'A1_MASK', 'A2_MASK', 'A4_MASK']}

            if has_mask_suffix:
                for suffix in ['MASK', 'A1_MASK', 'A2_MASK', 'A4_MASK']:
                    if suffix in suffixes:
                        offset = suffixes[suffix]
                        processed_images_lines.append(f"#define {base_name}_{suffix} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")

            if color_suffixes:
                processed_images_lines.append(f"#if LD_CFG_COLOR_DEPTH == 8\n")
                if 'GRAY8' in color_suffixes:
                    offset = color_suffixes['GRAY8']
                    processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                processed_images_lines.append(f"#elif LD_CFG_COLOR_DEPTH == 16\n")
                if 'RGB565' in color_suffixes:
                    offset = color_suffixes['RGB565']
                    processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                processed_images_lines.append(f"#else\n")
                if 'RGBA8888' in color_suffixes:
                    offset = color_suffixes['RGBA8888']
                    processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                else:
                    for suffix, offset in color_suffixes.items():
                        if suffix not in ['GRAY8', 'RGB565']:
                            processed_images_lines.append(f"#define {base_name} (arm_2d_tile_t*)ldBaseGetVresImage(0x{offset:08X})\n")
                            break
                processed_images_lines.append(f"#endif\n")

    with open(output_header_path, 'w', encoding='utf-8') as f:
        f.write("#ifndef _DATA_H_\n")
        f.write("#define _DATA_H_\n\n")
        f.write("#include \"arm_2d.h\"\n")
        f.write("#include \"ldConfig.h\"\n\n")

        font_entries = []
        for line in fonts_lines:
            if line.strip():
                parts = line.strip().split(':')
                if len(parts) >= 2:
                    font_entries.append((parts[0], parts[1]))

        name_bases = {}
        names_with_suffix = []
        
        for name, _ in font_entries:
            last_underscore_index = name.rfind('_')
            if last_underscore_index != -1:
                base_name = name[:last_underscore_index]
                suffix = name[last_underscore_index+1:]
                if re.match(r'^A\d+$', suffix):
                    if base_name not in name_bases:
                        name_bases[base_name] = []
                    name_bases[base_name].append(name)
                    names_with_suffix.append(name)

        names_to_keep_suffix = set()
        for base_name, names in name_bases.items():
            if len(names) > 1:
                names_to_keep_suffix.update(names)

        for name, offset_str in font_entries:
            final_name = process_name_with_suffix(name, name_bases, names_with_suffix, names_to_keep_suffix)
            
            if offset_str.startswith('0x') or offset_str.startswith('0X'):
                offset = int(offset_str, 16)
            else:
                offset = int(offset_str, 16)
            f.write(f"#define FONT_{final_name} (arm_2d_font_t*)ldBaseGetVresFont(0x{offset:08X})\n")

        if processed_images_lines:
            f.write('\n')
        for line in processed_images_lines:
            f.write(line)

        f.write("\n#endif // _DATA_H_\n")


def cleanup_process_files(input_dir):
    script_dir = input_dir if input_dir else os.path.dirname(os.path.abspath(__file__))

    directories = [
        os.path.join(script_dir, 'fonts'),
        os.path.join(script_dir, 'images')
    ]

    for directory in directories:
        if os.path.exists(directory):
            for filename in os.listdir(directory):
                if filename.endswith('.txt') or filename.endswith('.bin'):
                    file_path = os.path.join(directory, filename)
                    try:
                        os.remove(file_path)
                    except Exception as e:
                        print(f"删除 {file_path} 失败: {e}")
        else:
            print(f"目录不存在，跳过: {directory}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input_dir', nargs='?', default=None, help='输入目录路径')
    args = parser.parse_args()
    input_dir = args.input_dir
    script_dir = input_dir if input_dir else os.path.dirname(os.path.abspath(__file__))

    output_dir = os.path.join(script_dir, 'output')
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    fonts_dir_exists = os.path.exists(os.path.join(script_dir, 'fonts'))
    images_dir_exists = os.path.exists(os.path.join(script_dir, 'images'))
    
    if not fonts_dir_exists and not images_dir_exists:
        print("错误: fonts 和 images 文件夹都不存在")
        return
    
    if not fonts_dir_exists:
        print("警告: fonts 文件夹不存在")
    
    if not images_dir_exists:
        print("警告: images 文件夹不存在")

    merge_bin_files(input_dir)

    merge_txt_files(input_dir)

    cleanup_process_files(input_dir)

if __name__ == "__main__":
    main()

