"""Check that the maintained native projects still match the CMake C++ targets."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
NS = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}


def check() -> None:
    cmake = (ROOT / "Compiler/CMakeLists.txt").read_text(encoding="utf-8-sig")
    block = re.search(r"set\(GSPP_COMPILER_SOURCES\s+(.*?)\)", cmake, re.S)
    assert block, "Liste CMake des sources du bootstrap absente"
    expected = {
        "gspp_compiler": {"Compiler/" + p for p in block.group(1).split()},
    }
    for folder in ("Compiler", "Tests"):
        content = (ROOT / folder / "CMakeLists.txt").read_text(encoding="utf-8-sig")
        for name, files in re.findall(r"add_executable\(\s*(\w+)\s+([^)]*)\)", content):
            expected[name] = {f"{folder}/{p}" for p in files.split()}
    solution = ET.parse(ROOT / "GsPlusPlus.slnx").getroot()
    solution_projects = {p.attrib["Path"] for p in solution.iter("Project")}
    for name, files in expected.items():
        relative = f"VisualStudio/{name}.vcxproj"
        assert relative in solution_projects, f"Projet absent de la solution : {name}"
        path = ROOT / relative
        project = ET.parse(path).getroot()
        actual = {
            (path.parent / item.attrib["Include"].replace("\\", "/")).resolve().relative_to(ROOT).as_posix()
            for item in project.findall(".//m:ClCompile[@Include]", NS)
        }
        assert actual == files, f"Sources CMake/MSBuild divergentes pour {name}: {actual ^ files}"
        assert all((ROOT / p).is_file() for p in actual), f"Source absente : {name}"
        assert project.find(".//m:PlatformToolset", NS).text == "v145", name
        configs = {p.attrib["Include"] for p in project.findall(".//m:ProjectConfiguration", NS)}
        assert configs == {"Debug|x64", "Release|x64"}, name
    for path in (ROOT / "VisualStudio").glob("*.vcxproj"):
        project = ET.parse(path).getroot()
        for reference in project.findall(".//m:ProjectReference", NS):
            assert (path.parent / reference.attrib["Include"]).is_file(), path.name
        for command in project.findall(".//m:Exec", NS):
            assert not re.search(r"\b(cmake|ctest|ninja)\b", command.attrib["Command"], re.I), path.name
    print(f"SUCCES : {len(expected)} cibles C++ natives coherentes avec CMake, MSVC v145, Debug/Release x64.")


if __name__ == "__main__":
    check()
