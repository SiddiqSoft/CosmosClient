Set-Location "$PSScriptRoot\.."
python3 scripts/generate_dependencies_md.py
mkdocs build
