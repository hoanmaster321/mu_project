import os
import glob
import subprocess
import shutil

def compile_shaders_if_needed(shaders_dir):
    glslc = shutil.which("glslc")
    glslang = shutil.which("glslangValidator")

    glsl_files = glob.glob(os.path.join(shaders_dir, "*.glsl"))
    for glsl_path in glsl_files:
        filename = os.path.basename(glsl_path)
        base_name = filename[:-5] if filename.endswith(".glsl") else filename
        spv_name = base_name + ".spv"
        spv_path = os.path.join(shaders_dir, spv_name)

        stage = None
        glslc_stage = None
        if ".vert" in filename:
            stage = "vert"
            glslc_stage = "vertex"
        elif ".frag" in filename:
            stage = "frag"
            glslc_stage = "fragment"
        elif ".comp" in filename:
            stage = "comp"
            glslc_stage = "compute"

        if glslc:
            cmd = [glslc]
            if glslc_stage:
                cmd.append(f"-fshader-stage={glslc_stage}")
            cmd.extend([glsl_path, "-o", spv_path])
            try:
                subprocess.run(cmd, check=True)
                print(f"[glslc] Compiled {filename} -> {spv_name}")
            except Exception as e:
                print(f"[glslc] Error compiling {filename}: {e}")
        elif glslang and stage:
            cmd = [glslang, "-V", "-S", stage, glsl_path, "-o", spv_path]
            try:
                subprocess.run(cmd, check=True)
                print(f"[glslang] Compiled {filename} -> {spv_name}")
            except Exception as e:
                print(f"[glslang] Error compiling {filename}: {e}")

def generate_embedded_shaders():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    shaders_dir = os.path.join(script_dir, "Shaders")
    output_file = os.path.join(script_dir, "source", "EmbeddedShaders.h")

    compile_shaders_if_needed(shaders_dir)

    spv_files = sorted(glob.glob(os.path.join(shaders_dir, "*.spv")))
    if not spv_files:
        print("Warning: No .spv files found in", shaders_dir)
        return

    out = []
    out.append("#pragma once")
    out.append("#include <cstdint>")
    out.append("#include <cstddef>")
    out.append("#include <string>")
    out.append("#include <vector>")
    out.append("")
    out.append("namespace EmbeddedShaders {")

    mapping = []

    for spv_path in spv_files:
        filename = os.path.basename(spv_path)
        var_name = "g_spv_" + filename.replace(".", "_")

        with open(spv_path, "rb") as f:
            data = f.read()

        out.append(f"    // {filename} ({len(data)} bytes)")
        out.append(f"    alignas(4) inline const uint8_t {var_name}[] = {{")

        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_str = ",".join(f"0x{b:02X}" for b in chunk) + ","
            out.append(f"        {hex_str}")

        out.append("    };")
        out.append(f"    inline const size_t {var_name}_size = sizeof({var_name});")
        out.append("")

        mapping.append((filename, var_name))

    out.append("    inline std::vector<char> GetEmbeddedSPV(const std::string& name) {")
    for filename, var_name in mapping:
        out.append(f'        if (name == "{filename}") return std::vector<char>(reinterpret_cast<const char*>({var_name}), reinterpret_cast<const char*>({var_name}) + {var_name}_size);')
    out.append("        return std::vector<char>();")
    out.append("    }")
    out.append("}")
    out.append("")

    with open(output_file, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

    print(f"Generated {output_file} successfully with {len(spv_files)} embedded shaders.")

if __name__ == "__main__":
    generate_embedded_shaders()
