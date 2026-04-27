import sys
import os

def convert_obj_to_path1(obj_path, c_path, array_name):
    raw_verts = []
    raw_uvs = []
    faces = []

    try:
        with open(obj_path, 'r') as f:
            for line in f:
                parts = line.split()
                if not parts: continue
                
                if parts[0] == 'v':
                    raw_verts.append([float(parts[1]), float(parts[2]), float(parts[3])])
                elif parts[0] == 'vt':
                    raw_uvs.append([float(parts[1]), float(parts[2])])
                elif parts[0] == 'f':
                    faces.append(parts[1:]) 
                    
    except FileNotFoundError:
        print(f"Error: Could not find {obj_path}")
        return

    # 1. THE NAUGHTY DOG TRICK: Unroll everything into raw triangle strips
    pantheon_vertices = []
    
    for face in faces:
        # Triangulate polygons on the fly (fan method)
        for i in range(1, len(face) - 1):
            triangle = [face[0], face[i], face[i+1]]
            
            for tri_vert in triangle:
                v_data = tri_vert.split('/')
                v_idx = int(v_data[0]) - 1
                x, y, z = raw_verts[v_idx]
                
                # Handle UVs if the OBJ has them
                u, v_coord = 0.0, 0.0
                if len(v_data) > 1 and v_data[1] != '':
                    vt_idx = int(v_data[1]) - 1
                    u, v_coord = raw_uvs[vt_idx]
                    
                # ROCKSTAR TRICK: Baked Ambient Vertex Colors (Default to 128 gray)
                # You can modify this later to bake lighting directly into the array!
                r, g, b, a = 128, 128, 128, 128 

                pantheon_vertices.append({
                    'x': x, 'y': y, 'z': z, 'w': 1.0,
                    'u': u, 'v': v_coord, 'q': 1.0, 'pad': 0.0,
                    'r': r, 'g': g, 'b': b, 'a': a
                })

    # 2. VIF MATH: Calculate exactly how much VU1 memory we need
    # Every PantheonVertex is 3 Quadwords (48 bytes)
    vertex_qwc = len(pantheon_vertices) * 3
    
    # 3. Write the pre-compiled C file
    with open(c_path, 'w') as f:
        f.write(f"/* Auto-generated for PANTHEON Path 1 Engine from {obj_path} */\n")
        f.write(f"/* The EE CPU does ZERO math on this file. It streams via DMA directly to VU1. */\n\n")
        
        # Write the VIF/DMA Header
        f.write(f"PantheonVIFHeader {array_name}_header = {{\n")
        # DMA CNT Tag: Tell the DMA how many Quadwords to send total
        f.write(f"    {{ 0x10000000 | {7 + vertex_qwc}, 0, 0, 0 }}, // DMA CNT Tag\n")
        f.write(f"    {{ 0x01000101, 0, 0, 0 }}, // VIF STCYCL (Write 1, Skip 0)\n")
        # VIF UNPACK: 0x6C is V4_32 format. Unpack to VU1 Address 120.
        f.write(f"    {{ 0x6C000000 | ({vertex_qwc} << 16) | 120, 0, 0, 0 }}, // VIF UNPACK\n")
        f.write(f"    {{ 1.0f,0,0,0, 0,1.0f,0,0, 0,0,1.0f,0, 0,0,0,1.0f }}, // Local MVP Matrix\n")
        f.write(f"    {{ 0x14000000, 0, 0, 0 }}  // VIF MSCAL (Execute VU1 Microprogram at 0)\n")
        f.write(f"}};\n\n")

        # Write the 128-bit Aligned Vertices
        f.write(f"int {array_name}_vertex_count = {len(pantheon_vertices)};\n")
        f.write(f"PantheonVertex {array_name}_vertices[] = {{\n")
        
        for v in pantheon_vertices:
            f.write(f"    {{ {v['x']:.4f}f, {v['y']:.4f}f, {v['z']:.4f}f, {v['w']}f,  ")
            f.write(f"{v['u']:.4f}f, {v['v']:.4f}f, {v['q']}f, {v['pad']}f,  ")
            f.write(f"{v['r']}, {v['g']}, {v['b']}, {v['a']} }},\n")
            
        f.write(f"};\n")
        
    print(f"Path 1 Asset Compiled: {len(pantheon_vertices)} vertices ({vertex_qwc} QWORDS) ready for DMA.")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python obj2ps2.py <input.obj> <output.c> <array_name>")
    else:
        convert_obj_to_path1(sys.argv[1], sys.argv[2], sys.argv[3])