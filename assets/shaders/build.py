import subprocess
from pathlib import Path

src_dir = Path("src")
bin_dir = Path("bin")
bin_dir.mkdir(exist_ok=True)

for shader_path in src_dir.glob("*.*"):
    if shader_path.suffix in [".vert", ".frag"]:
        output_path = bin_dir / (shader_path.name + ".spv")
        subprocess.run(["glslc", str(shader_path), "-o", str(output_path)], check=True)
        print(f"Compiled {shader_path} -> {output_path}")